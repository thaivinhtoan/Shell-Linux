UBUNTU 18.4
Login root
	$sudo -s
Make file
	$make
Load
	$insmod RNG_module.ko
Run
	$./RNG_user_space
Unload
	$rmmod RNG_module.ko