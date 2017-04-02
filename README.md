# ESP-SwitchNode

This firmeware for the esp8266 (ESP-12) is made to switch 2x8 outputs in a network.
You can:
	- set a output on
	- set a output off
	- set the whole 16-bit register 
	
	
Also, it opens a captive portal to setup the wifi network. A route /set is there to set:
	- the hostname
	- the remote url host and port
	
	
## manipulate the whole state

turn on switch on:

```
http://switch-1/switch?v=1024
```

turn the two first switches on:

```
http://switch-1/switch?v=3
```

turn the first 8 switches on:

```
http://switch-1/switch?v=255
```

turn all off

```
http://switch-1/switch?v=0
```

## manipulate a single switch

tbd

	
## Schematics


tbd