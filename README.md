#mruby-mqtt

MQTT protocol library.

MQTT is a lightweight M2M,IoT protocol publish/subscribe messaging.
The mruby-mqtt is implemented as wrapper of [Paho C library](http://www.eclipse.org/paho/)

##Installing

Write in /mruby/build_config.rb

```ruby
MRuby::Build.new do |conf|
  conf.gem :github => 'hiroeorz/mruby-mqtt', :branch => 'master'
end
```

##Examples

###Setup

```ruby
MQTTClient.connect("tcp://test.mosquitto.org:1883", "mruby") do |c|
  c.on_connect   = -> { c.subscribe("/temp/shimane", 0)}
  c.on_subscribe = -> { puts "subscribe success"}
  c.on_publish   = -> { puts "publish success"}
end
```

###Publish.

```ruby
mqtt = MQTTClient.instance
mqtt.publish("/mytopic", "mydata", 1)
```

##Limitations

Only one MQTTClient instance created per one os process. This means that only one connection created to the broker per os process.

##License
The mruby-mqtt mrbgem is licensed under the terms of the MIT license. See the file LICENSE for details. 
