#include "mruby.h"
#include "mruby/class.h"
#include "mruby/data.h"
#include "mruby/numeric.h"
#include "mruby/string.h"
#include "mruby/variable.h"
#include <stdio.h>
#include <string.h>
#include "MQTTClient.h"

#define E_MQTT_RECEIVE_TIMEOUT_ERROR (mrb_class_get(mrb, "MQTTReceiveTimeout"))
#define E_MQTT_NULL_CLIENT_ERROR (mrb_class_get(mrb, "MQTTNullClient"))

/*******************************************************************
  MQTTMessage Class
 *******************************************************************/

// exp: message = MQTTMessage.new
mrb_value
mqtt_msg_new(mrb_state *mrb, mrb_value self)
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
  MQTTClient Class
 *******************************************************************/

static mrb_value
mqtt_init(mrb_state *mrb, mrb_value self)
{
  mrb_value address;
  mrb_value client_id;
  mrb_get_args(mrb, "oo", &address, &client_id);

  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "address"), address);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "client_id"), client_id);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "keep_alive"), 
	     mrb_fixnum_value(20));
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "timeout"), 
	     mrb_fixnum_value(10));

  DATA_PTR(self) = NULL;
  return self;
}

// exp: self.address  #=> "tcp://test.mosquitto.org:1883"
mrb_value
mqtt_address(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "address"));
}

// exp: self.client_id  #=> "my_client_id"
mrb_value
mqtt_client_id(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "client_id"));
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

// exp: self.timeout #=> 10
mrb_value
mqtt_timeout(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "timeout"));
}

// exp: self.timeout = 60
mrb_value
mqtt_set_timeout(mrb_state *mrb, mrb_value self) 
{
  mrb_value timeout_sec;
  mrb_get_args(mrb, "o", &timeout_sec);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "timeout"), timeout_sec);
  return timeout_sec;
}

/*******************************************************************
  MQTT Client Class API
 *******************************************************************/

// exp: self.connect  #=> true | false
mrb_value
mqtt_connect(mrb_state *mrb, mrb_value self)
{
  MQTTClient client;
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  mrb_int rc = -1;
  mrb_int result;

  mrb_value m_address    = mqtt_address(mrb, self);
  char     *c_address    = RSTRING_PTR(m_address);
  mrb_value m_client_id  = mqtt_client_id(mrb, self);
  char     *c_client_id  = RSTRING_PTR(m_client_id);
  mrb_value m_keep_alive = mqtt_keep_alive(mrb, self);
  mrb_int   c_keep_alive = (mrb_int)mrb_to_flo(mrb, m_keep_alive);

  MQTTClient_create(&client, c_address, c_client_id,
		    MQTTCLIENT_PERSISTENCE_NONE, NULL);

  conn_opts.keepAliveInterval = c_keep_alive;
  conn_opts.cleansession = 1;
  
  if ((rc = MQTTClient_connect(client, &conn_opts)) == MQTTCLIENT_SUCCESS) {
    DATA_PTR(self) = client;
    result = 1;
  }
  else {
    result = 0;
  }

  return mrb_bool_value(result);
}

mrb_value
mqtt_disconnect(mrb_state *mrb, mrb_value self)
{
  MQTTClient client = (MQTTClient)DATA_PTR(self);
  if (client == NULL) { return mrb_bool_value(0); }

  MQTTClient_disconnect(client, 10000);
  MQTTClient_destroy(&client);
  DATA_PTR(self) = NULL;
  return mrb_bool_value(1);
}

mrb_value
mqtt_publish(mrb_state *mrb, mrb_value self)
{
  MQTTClient client = (MQTTClient)DATA_PTR(self);

  if (client == NULL) {
    mrb_raise(mrb, E_MQTT_NULL_CLIENT_ERROR, "empty client");
  }

  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  MQTTClient_deliveryToken token;
  mrb_int rc;

  mrb_value topic;
  mrb_value payload;
  mrb_int qos;
  mrb_get_args(mrb, "ooi", &topic, &payload, &qos);

  char *topic_p = RSTRING_PTR(topic);
  char *payload_p = RSTRING_PTR(payload);

  mrb_value m_timeout_sec = mqtt_timeout(mrb, self);
  mrb_int c_timeout_sec = (mrb_int)mrb_to_flo(mrb, m_timeout_sec);

  pubmsg.payload = payload_p;
  pubmsg.payloadlen = strlen(payload_p);
  pubmsg.qos = qos;
  pubmsg.retained = 0;
  MQTTClient_publishMessage(client, topic_p, &pubmsg, &token);

  rc = MQTTClient_waitForCompletion(client, token, (c_timeout_sec * 1000));
  return mrb_fixnum_value(rc);
}

mrb_value
mqtt_subscribe(mrb_state *mrb, mrb_value self)
{
  MQTTClient client = (MQTTClient)DATA_PTR(self);

  if (client == NULL) {
    mrb_raise(mrb, E_MQTT_NULL_CLIENT_ERROR, "empty client");
  }

  mrb_value topic;
  mrb_int qos;
  mrb_int rc = -1;
  mrb_get_args(mrb, "oi", &topic, &qos);

  char *topic_p = RSTRING_PTR(topic);
  rc = MQTTClient_subscribe(client, topic_p, qos);

  if (rc == MQTTCLIENT_SUCCESS) {
    return mrb_bool_value(1);
  }
  else {
    return mrb_bool_value(0);
  }
}

mrb_value
mqtt_block_receive(mrb_state *mrb, mrb_value self)
{
  MQTTClient client = (MQTTClient)DATA_PTR(self);

  if (client == NULL) {
    mrb_raise(mrb, E_MQTT_NULL_CLIENT_ERROR, "empty client");
  }

  mrb_int timeout_sec;
  mrb_get_args(mrb, "i", &timeout_sec);

  char *topic = NULL;
  mrb_int topic_len;
  MQTTClient_message *pubmsg = NULL;

  mrb_int result = MQTTClient_receive(client, &topic, &topic_len, &pubmsg,
				      timeout_sec * 1000);
  if (result != MQTTCLIENT_SUCCESS) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "message receive failure");
  }
  if (pubmsg->payload == NULL) {
    mrb_raise(mrb, E_MQTT_RECEIVE_TIMEOUT_ERROR, "message receive timeout");
  }

  mrb_value message = mqtt_msg_new(mrb, self);
  mrb_funcall(mrb, message, "topic=", 1, mrb_str_new_cstr(mrb, topic));
  mrb_funcall(mrb, message, "payload=", 1, mrb_str_new_cstr(mrb, pubmsg->payload));
  return message;
}

void
mrb_mruby_mqtt_gem_init(mrb_state* mrb)
{
  mrb_define_class(mrb, "MQTTReceiveTimeout", mrb->eStandardError_class);
  mrb_define_class(mrb, "MQTTNullClient", mrb->eStandardError_class);

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

  mrb_define_method(mrb, c, "initialize", mqtt_init, MRB_ARGS_ANY());
  mrb_define_method(mrb, c, "address", mqtt_address, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "client_id", mqtt_address, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "keep_alive", mqtt_keep_alive, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "keep_alive=", mqtt_set_keep_alive, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, c, "timeout", mqtt_timeout, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "timeout=", mqtt_set_timeout, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "connect", mqtt_connect, MRB_ARGS_NONE());
  mrb_define_method(mrb, c, "publish", mqtt_publish, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, c, "subscribe", mqtt_subscribe, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, c, "block_receive", mqtt_block_receive, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, c, "disconnect", mqtt_disconnect, MRB_ARGS_NONE());
}

void
mrb_mruby_mqtt_gem_final(mrb_state* mrb)
{
}
