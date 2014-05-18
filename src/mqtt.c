/*
Copyright (c) 2014 Shin Hiroe

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "mruby.h"
#include "mruby/class.h"
#include "mruby/data.h"
#include "mruby/numeric.h"
#include "mruby/string.h"
#include "mruby/variable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"

#define E_MQTT_ALREADY_CONNECTED_ERROR  (mrb_class_get(mrb, "MQTTAlreadyConnectedError"))
#define E_MQTT_CONNECTION_FAILURE_ERROR (mrb_class_get(mrb, "MQTTConnectionFailureError"))
#define E_MQTT_SUBSCRIBE_ERROR          (mrb_class_get(mrb, "MQTTSubscribeFailureError"))
#define E_MQTT_PUBLISH_ERROR            (mrb_class_get(mrb, "MQTTPublishFailureError"))
#define E_MQTT_DISCONNECT_ERROR         (mrb_class_get(mrb, "MQTTDisconnectFailureError"))

typedef struct _mqtt_state {
  const char *struct_name;
  void (*dfree)(mrb_state *mrb, void*);
  mrb_state *mrb;
  mrb_value self;
  MQTTAsync client;
} mqtt_state;

static mrb_value _self;
static int mqtt_connected;

/*******************************************************************
  MQTTMessage Class
 *******************************************************************/

// exp: message = MQTTMessage.new
mrb_value
mqtt_msg_new(mrb_state *mrb)
{
  struct RClass *_class_message;
  _class_message = mrb_class_get(mrb, "MQTTMessage");
  return mrb_obj_new(mrb, _class_message, 0, NULL);
}

// exp: message.topic #=> "/temp/shimane"
mrb_value
mqtt_msg_topic(mrb_state *mrb, mrb_value message)
{
  return mrb_iv_get(mrb, message, mrb_intern_lit(mrb, "topic"));
}

// exp: message.topic = "/tmp/shimane"
mrb_value
mqtt_set_msg_topic(mrb_state *mrb, mrb_value message)
{
  mrb_value topic;
  mrb_get_args(mrb, "o", &topic);
  mrb_iv_set(mrb, message, mrb_intern_lit(mrb, "topic"), topic);
  return topic;
}

// exp: message.payload #=> "/temp/shimane"
mrb_value
mqtt_msg_payload(mrb_state *mrb, mrb_value message)
{
  return mrb_iv_get(mrb, message, mrb_intern_lit(mrb, "payload"));
}

// exp: message.payload = "10"
mrb_value
mqtt_set_msg_payload(mrb_state *mrb, mrb_value message)
{
  mrb_value payload;
  mrb_get_args(mrb, "o", &payload);
  mrb_iv_set(mrb, message, mrb_intern_lit(mrb, "payload"), payload);
  return payload;
}

/*******************************************************************
  MQTT Call backs
 *******************************************************************/

int
mqtt_msgarrvd(void *context, char *topicName, int topicLen,
	      MQTTAsync_message *message)
{
  mqtt_state *m = DATA_PTR(_self);
  char *ptr;

  ptr = topicName;
  for(int i = 0; i < topicLen; i++) ptr++;
  *ptr = '\0';

  ptr = message->payload;
  for(int i = 0; i < message->payloadlen; i++) ptr++;
  *ptr = '\0';

  mrb_value mrb_topic = mrb_str_new_cstr(m->mrb, topicName);
  mrb_value mrb_payload = mrb_str_new_cstr(m->mrb, message->payload);
  mrb_value mrb_message = mqtt_msg_new(m->mrb);
  mrb_funcall(m->mrb, mrb_message, "topic=", 1, mrb_topic);
  mrb_funcall(m->mrb, mrb_message, "payload=", 1, mrb_payload);
  mrb_funcall(m->mrb, m->self, "on_message_callback", 1, mrb_message);

  MQTTAsync_freeMessage(&message);
  MQTTAsync_free(topicName);
  return 1;
}

void
mqtt_connlost(void *context, char *cause)
{
  mqtt_state *m = DATA_PTR(_self);
  mrb_sym cause_sym = mrb_intern_cstr(m->mrb, cause);
  mqtt_connected = FALSE;
  mrb_funcall(m->mrb, m->self, "connlost_callback", 1, cause_sym);
}

void
mqtt_on_disconnect(void* context, MQTTAsync_successData* response)
{
  mqtt_state *m = DATA_PTR(_self);
  mqtt_connected = FALSE;
  mrb_funcall(m->mrb, m->self, "on_disconnect_callback", 0);
}

void
mqtt_on_subscribe(void* context, MQTTAsync_successData* response)
{
  mqtt_state *m = DATA_PTR(_self);
  mrb_funcall(m->mrb, m->self, "on_subscribe_callback", 0);
}

void
mqtt_on_subscribe_failure(void* context, MQTTAsync_failureData* response)
{
  mqtt_state *m = DATA_PTR(_self);
  mrb_funcall(m->mrb, m->self, "on_subscribe_failure_callback", 0);
}

void
mqtt_on_publish(void* context, MQTTAsync_successData* response)
{
  mqtt_state *m = DATA_PTR(_self);
  mrb_funcall(m->mrb, m->self, "on_publish_callback", 0);
}

void
mqtt_on_connect_failure(void* context, MQTTAsync_failureData* response)
{
  mqtt_state *m = DATA_PTR(_self);
  mrb_funcall(m->mrb, m->self, "on_connect_failure_callback", 0);
}


void
mqtt_on_connect(void* context, MQTTAsync_successData* response)
{
  mqtt_state *m = DATA_PTR(_self);
  mqtt_connected = TRUE;
  mrb_funcall(m->mrb, m->self, "on_connect_callback", 0);
}

/*******************************************************************
  MQTTClient Class
 *******************************************************************/

static mrb_value
mqtt_init(mrb_state *mrb, mrb_value self)
{
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "address"), mrb_nil_value());
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "client_id"), mrb_nil_value());
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "keep_alive"), 
	     mrb_fixnum_value(20));
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "request_timeout"), 
	     mrb_fixnum_value(1000));

  DATA_PTR(self) = NULL;
  return self;
}

// exp: self.address  #=> "tcp://test.mosquitto.org:1883"
mrb_value
mqtt_address(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "address"));
}

// exp: self.address = "tcp://test.mosquitto.org:1883"
mrb_value
mqtt_set_address(mrb_state *mrb, mrb_value self)
{
  mrb_value address;
  mrb_get_args(mrb, "o", &address);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "address"), address);
  return address;
}

// exp: self.client_id  #=> "my_client_id"
mrb_value
mqtt_client_id(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "client_id"));
}

// exp: self.client_id = "my-clilent-id"
mrb_value
mqtt_set_client_id(mrb_state *mrb, mrb_value self)
{
  mrb_value client_id;
  mrb_get_args(mrb, "o", &client_id);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "client_id"), client_id);
  return client_id;
}

// exp: self.keep_alive #=> 20
mrb_value
mqtt_keep_alive(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "keep_alive"));
}

// exp: self.keep_alive = 20
mrb_value
mqtt_set_keep_alive(mrb_state *mrb, mrb_value self)
{
  mrb_value keep_alive;
  mrb_get_args(mrb, "o", &keep_alive);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "keep_alive"), keep_alive);
  return keep_alive;
}

// exp: self.request_timeout #=> 10
mrb_value
mqtt_request_timeout(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "request_timeout"));
}

// exp: self.request_timeout = 60
mrb_value
mqtt_set_request_timeout(mrb_state *mrb, mrb_value self) 
{
  mrb_value timeout_sec;
  mrb_get_args(mrb, "o", &timeout_sec);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "request_timeout"), timeout_sec);
  return timeout_sec;
}

/*******************************************************************
  MQTT Client Class API
 *******************************************************************/

mrb_value
mqtt_is_connected(mrb_state *mrb, mrb_value self)
{
  return mqtt_connected ? mrb_true_value() : mrb_false_value();
}

mrb_bool
clean_session(mrb_state* mrb, mrb_value self)
{
  return mrb_obj_eq(mrb, mrb_true_value(),
		    mrb_funcall(mrb, self, "clean_session", 0));
}

// exp: self.connect  #=> true | false
mrb_value
mqtt_connect(mrb_state *mrb, mrb_value self)
{
  if (mqtt_connected) {
    mrb_raise(mrb, E_MQTT_ALREADY_CONNECTED_ERROR, "MQTT Already connected");
  }

  MQTTAsync client;
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
  mrb_value m_address = mqtt_address(mrb, self);
  mrb_value m_client_id = mqtt_client_id(mrb, self);
  mrb_value m_keep_alive = mqtt_keep_alive(mrb, self);
  mrb_int c_keep_alive = (mrb_int)mrb_fixnum(m_keep_alive);
  char *c_address = mrb_str_to_cstr(mrb, m_address);
  char *c_client_id = mrb_str_to_cstr(mrb, m_client_id);
  int rc;

  MQTTAsync_create(&client, c_address, c_client_id, 
		   MQTTCLIENT_PERSISTENCE_NONE, NULL);

  MQTTAsync_setCallbacks(client, NULL, mqtt_connlost, mqtt_msgarrvd, NULL);

  conn_opts.keepAliveInterval = c_keep_alive;
  conn_opts.cleansession = clean_session(mrb, self);
  conn_opts.onSuccess = mqtt_on_connect;
  conn_opts.onFailure = mqtt_on_connect_failure;
  conn_opts.context = client;

  if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
    mrb_raise(mrb, E_MQTT_CONNECTION_FAILURE_ERROR, "connection failure");
  }
 
  mqtt_state *mqtt_state_p;
  mqtt_state_p = mrb_malloc(mrb, sizeof(mqtt_state));
  mqtt_state_p->mrb = mrb;
  mqtt_state_p->self = self;
  mqtt_state_p->client = client;
  DATA_PTR(self) = mqtt_state_p;
  _self = self;

  return mrb_bool_value(TRUE);
}

mrb_value
mqtt_disconnect(mrb_state *mrb, mrb_value self)
{
  mqtt_state *m = DATA_PTR(_self);
  MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
  int rc;
  
  opts.onSuccess = mqtt_on_disconnect;
  opts.context = m->client;
  
  if ((rc = MQTTAsync_disconnect(m->client, &opts)) != MQTTASYNC_SUCCESS) {
    mrb_raise(mrb, E_MQTT_DISCONNECT_ERROR, "disconnect failure");
  }

  return mrb_bool_value(TRUE);
}

mrb_value
mqtt_publish(mrb_state *mrb, mrb_value self)
{
  int rc;
  mqtt_state *m = DATA_PTR(_self);
  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
  MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

  mrb_value topic;
  mrb_value payload;
  mrb_int qos;
  mrb_bool retain;
  mrb_get_args(mrb, "ooib", &topic, &payload, &qos, &retain);
  char *topic_p = mrb_str_to_cstr(mrb, topic);
  char *payload_p = mrb_str_to_cstr(mrb, payload);

  opts.onSuccess = mqtt_on_publish;
  opts.context = m->client;

  pubmsg.payload = payload_p;
  pubmsg.payloadlen = strlen(payload_p);
  pubmsg.qos = qos;
  pubmsg.retained = retain;

  if ((rc = MQTTAsync_sendMessage(m->client, topic_p, &pubmsg, &opts)) != MQTTASYNC_SUCCESS) {
    mrb_raise(mrb, E_MQTT_PUBLISH_ERROR, "publish failure");
  }

  return mrb_bool_value(TRUE);
}

mrb_value
mqtt_subscribe(mrb_state *mrb, mrb_value self)
{
  int rc;
  mqtt_state *m = DATA_PTR(_self);
  mrb_value topic;
  mrb_int qos;

  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
  opts.onSuccess = mqtt_on_subscribe;
  opts.onFailure = mqtt_on_subscribe_failure;
  opts.context = m->client;

  mrb_get_args(mrb, "oi", &topic, &qos);
  char *topic_p = mrb_str_to_cstr(mrb, topic);
  
  if ((rc = MQTTAsync_subscribe(m->client, topic_p, qos, &opts)) != MQTTASYNC_SUCCESS) {
    mrb_raise(mrb, E_MQTT_SUBSCRIBE_ERROR, "subscribe failure");
  }

  return mrb_bool_value(TRUE);
}

void
mrb_mruby_mqtt_gem_init(mrb_state* mrb)
{
  mrb_define_class(mrb, "MQTTConnectionFailureError", mrb->eStandardError_class);
  mrb_define_class(mrb, "MQTTSubscribeFailureError",  mrb->eStandardError_class);
  mrb_define_class(mrb, "MQTTPublishFailureError",    mrb->eStandardError_class);
  mrb_define_class(mrb, "MQTTDisconnectFailureError", mrb->eStandardError_class);
  mrb_define_class(mrb, "MQTTNullClientError",        mrb->eStandardError_class);
  mrb_define_class(mrb, "MQTTAlreadyConnectedError",  mrb->eStandardError_class);

  struct RClass *d;
  d = mrb_define_class(mrb, "MQTTMessage", mrb->object_class);
  MRB_SET_INSTANCE_TT(d, MRB_TT_DATA);
  mrb_define_method(mrb, d, "topic", mqtt_msg_topic, MRB_ARGS_NONE());
  mrb_define_method(mrb, d, "topic=", mqtt_set_msg_topic, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, d, "payload", mqtt_msg_payload, MRB_ARGS_NONE());
  mrb_define_method(mrb, d, "payload=", mqtt_set_msg_payload, MRB_ARGS_REQ(1));

  struct RClass *c;
  c = mrb_define_class(mrb, "MQTTClient", mrb->object_class);
  MRB_SET_INSTANCE_TT(c, MRB_TT_DATA);

  mrb_define_method(mrb, c, "initialize", mqtt_init, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "address", mqtt_address, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "address=", mqtt_set_address, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, c, "client_id", mqtt_client_id, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "client_id=", mqtt_set_client_id, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, c, "keep_alive", mqtt_keep_alive, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "keep_alive=", mqtt_set_keep_alive, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, c, "request_timeout", mqtt_request_timeout, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "request_timeout=", mqtt_set_request_timeout, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "connect", mqtt_connect, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "connected?", mqtt_is_connected, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "publish_internal", mqtt_publish, MRB_ARGS_REQ(4));
  mrb_define_method(mrb, c, "subscribe_internal", mqtt_subscribe, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, c, "disconnect", mqtt_disconnect, MRB_ARGS_NONE());
}

void
mrb_mruby_mqtt_gem_final(mrb_state* mrb)
{
}
