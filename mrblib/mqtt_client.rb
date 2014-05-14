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
  #   puts "topic:#{m.topic} paylaod:#{m.payload}"
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

  #
  # MQTTClient.connect("tcp://test.mosquitto.org:1883", "mruby") do |c|
  #   c.receive do |m|
  #     puts "topic:#{m.topic} paylaod:#{m.payload}"
  #   end
  # end
  #
  # MQTTClient.connect("tcp://test.mosquitto.org:1883", "mruby") do |c|
  #   loop do
  #     c.receive do |m|
  #       puts "topic:#{m.topic} paylaod:#{m.payload}"
  #     end
  #   end
  # end
  #
  def receive(timeout_sec = 60, &block)
    msg = block_receive(timeout_sec)
    return msg unless block_given?
    block.call(msg)
  end

end
