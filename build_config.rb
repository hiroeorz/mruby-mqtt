MRuby::Build.new do |conf|

  toolchain :gcc
  conf.gembox 'default'
  conf.gem :github => 'ksss/mruby-singleton', :branch => 'master'
  conf.gem :github => 'matsumoto-r/mruby-sleep', :branch => 'master'

  conf.gem '../mruby-mqtt'

  conf.linker do |linker|
    linker.link_options = "%{flags} -o %{outfile} %{objs} %{libs} -lpthread -Wl -lm"
  end

end
