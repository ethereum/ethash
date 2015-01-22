# -*- mode: ruby -*-
# vi: set ft=ruby :

VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.box = "ubuntu/trusty64"
  config.vm.box_url = "https://cloud-images.ubuntu.com/vagrant/trusty/current/trusty-server-cloudimg-amd64-vagrant-disk1.box"
  config.vm.provider "virtualbox" do |v|
    v.memory = 1024
    v.cpus = 4
  end
  config.vm.provision "shell", inline: <<-EOF
  	apt-get update
	apt-get install -y emscripten cmake libcrypto++-dev openmpi-bin libopenmpi-dev libboost1.55-dev libboost-test1.55-dev
  EOF
end
