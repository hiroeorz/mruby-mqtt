MQTT_TOPIC_1= "/mqtttest/test1"
MQTT_TOPIC_2 = "/mqtttest/test2"
MQTT_PAYLOAD = "hello"

assert("MQTTClient.connect") do
  subscribe_count = 0
  publish_count = 0

  MQTTClient.connect("tcp://test.mosquitto.org:1883", "mruby") do |c|
    c.on_subscribe = -> { subscribe_count += 1 }
    c.on_publish   = -> { publish_count += 1}

    c.on_connect   = -> {
      c.subscribe(MQTT_TOPIC_1, 0)
      c.publish(MQTT_TOPIC_1, MQTT_PAYLOAD)
      c.publish(MQTT_TOPIC_2, MQTT_PAYLOAD)
    }

    c.on_message = -> (message) {
      assert_equal MQTT_TOPIC_1, message.topic
      assert_equal MQTT_PAYLOAD, message.payload
    }
  end


  Sleep.sleep 5
  assert_equal 1, subscribe_count
  assert_equal 2, publish_count
end
