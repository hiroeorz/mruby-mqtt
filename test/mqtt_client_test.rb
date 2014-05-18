MQTT_TOPIC_1= "/mqtttest/test1"
MQTT_TOPIC_2 = "/mqtttest/test2"
MQTT_PAYLOAD_1 = "hello1"
MQTT_PAYLOAD_2 = "hello2"

assert("MQTTClient.connect") do
  subscribe_count = 0
  publish_count = 0
  get_message_count = 0

  MQTTClient.connect("tcp://127.0.0.1:1883", "mruby-client") do |c|
    c.on_subscribe = -> { subscribe_count += 1 }
    c.on_publish   = -> { publish_count += 1}

    c.on_connect   = -> {
      c.subscribe(MQTT_TOPIC_1, qos:0)
      c.publish(MQTT_TOPIC_2, "world")
      c.publish(MQTT_TOPIC_2, "this is mruby-mqtt libraries")
      c.publish(MQTT_TOPIC_1, MQTT_PAYLOAD_1) # subscribed
      c.publish(MQTT_TOPIC_2, "neko")
      c.publish(MQTT_TOPIC_2, "inu")
      c.publish(MQTT_TOPIC_1, MQTT_PAYLOAD_2) # subscribed
    }

    c.on_message = -> (message) {
      if get_message_count == 0
        assert_equal MQTT_TOPIC_1, message.topic
        assert_equal MQTT_PAYLOAD_1, message.payload
      end

      if get_message_count == 1
        assert_equal MQTT_TOPIC_1, message.topic
        assert_equal MQTT_PAYLOAD_2, message.payload
      end

      get_message_count += 1
    }
  end


  Sleep.sleep 1
  assert_equal 1, subscribe_count
  assert_equal 6, publish_count
  m = MQTTClient.instance
  assert_equal true, m.disconnect
  Sleep.sleep 5 # wait disconnect callback.
end

assert("MQTTClient.instance.clean_session = false") do

  mqtt = MQTTClient.instance
  assert_equal true, mqtt.clean_session

  mqtt.clean_session = false
  assert_equal false, mqtt.clean_session

  mqtt.clean_session = true
  assert_equal true, mqtt.clean_session

  assert_raise(ArgumentError) { mqtt.clean_session = 1 }
  assert_raise(ArgumentError) { mqtt.clean_session = 0 }
  assert_raise(ArgumentError) { mqtt.clean_session = "true" }
  assert_raise(ArgumentError) { mqtt.clean_session = nil }

end

assert("MQTTClient.instance.reconnect_interval") do

  mqtt = MQTTClient.instance
  assert_equal 5, mqtt.reconnect_interval

  mqtt.reconnect_interval = 10
  assert_equal 10, mqtt.reconnect_interval

  assert_raise(ArgumentError) { mqtt.reconnect_interval = true }
  assert_raise(ArgumentError) { mqtt.reconnect_interval = "true" }
  assert_raise(ArgumentError) { mqtt.reconnect_interval = nil }

end
