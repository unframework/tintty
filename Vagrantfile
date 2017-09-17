# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure("2") do |config|
  config.vm.box = 'bento/ubuntu-16.04'
  config.vm.network 'private_network', ip: '192.168.60.60'

  # per https://gist.github.com/garyachy/c1ee689c73bde019a44078edc5b8ed02
  config.vm.provider :virtualbox do |vb|
    vb.customize ["modifyvm", :id, "--uart1", "0x3f8", "4"]
    vb.customize ["modifyvm", :id, "--uartmode1", "COM3:"]
  end
end
