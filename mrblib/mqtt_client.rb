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
#   c.clean_session = false
#   c.on_connect   = -> { c.subscribe("/temp/shimane")}
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

  attr_accessor :on_connect, :on_subscribe, :on_publish, :on_disconnect
  attr_accessor :on_connect_failure, :on_subscribe_failure, :on_connlost
  attr_accessor :on_message
  attr_accessor :debug

  class << self
    def connect(address, client_id, &block)
      client = self.instance
      client.address = address
      client.client_id = client_id
      block.call(client) if block_given?
      client.connect
      return client
    end
  end

  def reconnect_interval
    @reconnect_interval ||= 5
  end

  def reconnect_interval=(val)
    unless val.kind_of? Numeric
      raise ArgumentError.new("invalid reconnect_interval:#{val}")
    end

    @reconnect_interval = val
  end

  def clean_session
    return true if @clean_session.nil? # defualt true
    @clean_session
  end

  def clean_session=(val)
    unless [true, false].include?(val)
      raise ArgumentError.new("invalid clean_session:#{val}")
    end

    if connected?
      raise ArgumentError.new("Can't set clean_session after connected")
    end

    @clean_session = val
  end

  def publish(topic, payload, opts = {})
    qos = opts[:qos] || 0
    retain = opts[:retain] || false

    unless [0,1,2].include?(qos)
      raise ArgumentError.new("invalid qos:#{qos}")
    end

    unless [true, false].include?(retain)
      raise ArgumentError.new("invalid retain:#{retain}")
    end

    publish_internal(topic, payload, qos, retain)
  end

  def subscribe(topic, opts = {})
    qos = opts[:qos] || 0

    unless [0,1,2].include?(qos)
      raise ArgumentError.new("invalid qos:#{qos}")
    end

    subscribe_internal(topic, qos)
  end

  def on_connect_callback
    debug_out "on_connect_callback"
    @on_connect.call if @on_connect
  end

  def on_connect_failure_callback
    debug_out "connection lost"

    if @on_connect_failure
      @on_connect_failure.call
    else
      sleep reconnect_interval
      connect
    end    
  end

  def on_subscribe_callback
    debug_out "on_subscribe_callback"
    @on_subscribe.call if @on_subscribe
  end

  def on_subscribe_failure_callback
    debug_out "on_subscribe_failure_callback"
    @on_subscribe_failure.call if @on_subscribe_failure
  end

  def on_message_callback(message)
    debug_out "received: #{message.topic} : #{message.payload}"
    @on_message.call(message) if @on_message
  end

  def on_publish_callback
    debug_out "on_publish_callback"
    @on_publish.call if @on_publish
  end

  def on_disconnect_callback
    debug_out "on_disconnect_callback"
    @on_disconnect.call if @on_disconnect
  end

  def connlost_callback(cause)
    debug_out "connection lost"; p cause

    if @on_connlost
      @on_connlost.call
    else
      sleep reconnect_interval
      connect
    end
  end

  private

  def debug_out(str, *args)
    return unless @debug
    printf(str + "\n", *args)
  end

end
