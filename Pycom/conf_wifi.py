import network
import time

# Conectar el Wipy a la Wifi
#
yourWifiSSID = "MOVISTAR_4149"
yourWifiPassword = "s8JNj2DehX2DwpiGSCzx"
wlan = network.WLAN(mode=network.WLAN.STA)
wlan.connect(yourWifiSSID,auth=(network.WLAN.WPA2, yourWifiPassword))
time.sleep(1)

while not wlan.isconnected():
  print('.')
  time.sleep(1)

print("Connected to wifi %s" %yourWifiSSID)
print('IP: %s' %wlan.ifconfig()[0])
