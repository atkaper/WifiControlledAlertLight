## Wifi Controlled Alert Light

#### Shopping List

- (second hand) flash light.
- WeMos D1 Mini - ESP8266 Wifi micro controller.
- Bridge rectifier (or 4 diodes, 1N4001) for example KBP307, to make some DC from AC - (my flash light uses a 12V AC adapter).
- Big Electrolytic Capacitor to flatten the AC/DC converted voltage ripple, for example 470 microfarad.
- DC-DC converter module, to transform the high DC (15 volt in my case) to 5 volt DC.
- LED (5 mm).
- Resistor 330 Ohm.
- Small relay with 5 volt coil.
- Small diode, for example 1N4148.
- NPN transistor, for example BC547.
- Resistor 1.6 Kilo Ohm.
- Some hot glue to mount everything in the light housing.

#### Building the circuit

- TODO; Add a nice circuit drawing + instructions.

Fow now, just see the photo's. Note that the shown breadboard power-supply has been replaced by a DC-DC converter (the other
one got too warm).

#### Compile source

Compile WifiControlledAlertLight.ino using Arduino IDE, and install on the ESP.

#### Testing

The code contains a wifi-manager, so connect to it using your phone or laptop to connect it to your WIFI.
Put the long-poll.php code on a server, and make sure it's url is in your sketch.

Some (shell - curl) test commands, to use the flash light:
```
# Note: you can find the user (mac address) to use, by looking for the data_*.poll files in your folder,
# or, look in your server access log to find the user-agent which contains the mac address.
 
# enable light for 4 seconds:
curl -v "http://YOUR-SERVER.SOMEWERE/SOMEPATH/long-poll.php?user=12_34_56_78_90_AB&code=4000"
 
# flash the LED due to unknown command code:
curl -v "http://YOUR-SERVER.SOMEWERE/SOMEPATH/long-poll.php?user=12_34_56_78_90_AB&code=flash-led"
 
# ask the server for online-status of the flash light:
curl -s "http://YOUR-SERVER.SOMEWERE/SOMEPATH/long-poll.php?user=12_34_56_78_90_AB&status"
 
# response will be like this:
ONLINE November 13 2018 22:41:26.
 
 
# If your flash light is on your local wireless network, and you CAN reach it, and know it's IP, you can also
# enable the light with a call like this (replace IP with correct IP):
curl -v "http://192.168.0.123/light/pulse"
 
# See the ESP source code above for other command's you can send to it ;-)
```

I'll put a bit more descriptive story on my website's blog...

Thijs.

