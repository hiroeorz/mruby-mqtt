# The MQTTClient class implements the MQTT Client.
#
# == Usage
#
# MQTTClient.connect("tcp://test.mosquitto.org:1883", "mruby") do |c|
#   c.on_connect   = -> { c.subscribe("/temp/shimane", 0)}
#   c.on_subscribe = -> { puts "subscribe success"}
#   c.on_publish   = -> { puts "publish success"}
# end
#
# Publish other scope
#
# mqtt = MQTTClient.instance
# mqtt.publish("/mytopic", "mydata", 1)
#

class MQTTClient
  include Singleton

  attr_reader :connecting
  attr_accessor :on_connect, :on_subscribe, :on_publish, :on_disconnect
  attr_accessor :on_connect_failure, :on_subscribe_failure, :on_connlost

  class << self
    def connect(address, client_id, &block)
      client = self.instance
      client.address = address
      client.client_id = client_id
      return client unless block_given?

      block.call(client)
      client.connect
    end
  end

  def initialize
    @connecting = nil
  end

  def connect
    if @connecting
      raise MQTTAlreadyConnectedError.new("already connected")
    end

    connect!
  end

  def on_connect_callback
    puts "on_connect_callback"
    @connecting = true
    @on_connect.call if @on_connect
  end

  def on_connect_failure_callback
    puts "connection lost"; p cause

    @connecting = nil

    if @on_connect_failure
      @on_connect_failure.call
    else
      sleep 3
      connect
    end    
  end

  def on_subscribe_callback
    puts "on_subscribe_callback"
    @on_subscribe.call if @on_subscribe
  end

  def on_subscribe_failure_callback
    puts "on_subscribe_failure_callback"
    @on_subscribe_failure.call if @on_subscribe_failure
  end

  def on_message_callback(message)
    puts "received: #{message.topic} : #{message.payload}"
  end

  def on_publish_callback
    puts "on_publish_callback"
    @on_publish.call if @on_publish
  end

  def on_disconnect_callback
    puts "on_disconnect_callback"
    @connecting = nil
    @on_disconnect.call if @on_disconnect
  end

  def connlost_callback(cause)
    puts "connection lost"; p cause
    @connecting = nil

    if @on_connlost
      @on_connlost.call
    else
      sleep 3
      connect
    end
  end

end

class MQTTAlreadyConnectedError < StandardError
end
