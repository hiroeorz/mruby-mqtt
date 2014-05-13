#mruby-mqtt

mrbgem that wrap the Phaho MQTT protocol library, a lightweight M2M,IoT protocol publish/subscribe messaging.

#Installing

#Examples

##Publish

```ruby
MQTTClient.connect("tcp://test.mosquitto.org:1883", "mruby") do |c|
  c.publish("/tmp/shimane", "hello", 0)
end
```

##Subscribe

```ruby
MQTTClient.connect("tcp://test.mosquitto.org:1883", "mruby") do |c|
  c.subscribe("/tmp/shimane", 0)
  m = c.receive(300)
  puts "topic:#{m.topic} paylaod:#{m.payload}"
end
```

Block receive

```ruby
MQTTClient.connect("tcp://test.mosquitto.org:1883", "mruby") do |c|
   c.receive do |m|
     puts "topic:#{m.topic} paylaod:#{m.payload}"
   end
 end
```

```ruby
 MQTTClient.connect("tcp://test.mosquitto.org:1883", "mruby") do |c|
   loop do
     c.receive do |m|
       puts "topic:#{m.topic} paylaod:#{m.payload}"
     end
   end
 end
```
