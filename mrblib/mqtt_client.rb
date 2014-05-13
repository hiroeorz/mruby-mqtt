class MQTTClient

  #
  # # Publish
  #
  # MQTTClient.connect("tcp://test.mosquitto.org:1883", "mruby") do |c|
  #   c.publish("/tmp/shimane", "hello", 0)
  # end
  #
  # # Subscribe
  #
  # MQTTClient.connect("tcp://test.mosquitto.org:1883", "mruby") do |c|
  #   c.subscribe("/tmp/shimane", 0)
  #   m = c.receive(300)
  #   p m.topic
  #   p m.payload
  # end
  #
  def self.connect(address, client_id, &block)
    client = self.new(address, client_id)
    client.connect
    return client unless block_given?

    begin
      block.call(client)
    ensure
      client.disconnect
    end
  end

end
