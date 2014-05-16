# Copyright (c) 2014 Shin Hiroe
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.


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
