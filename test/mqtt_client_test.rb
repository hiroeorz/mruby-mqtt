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
  Sleep.sleep 1 # wait disconnect callback.
end
