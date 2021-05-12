# boot.py -- run on boot-up

import network
import time
from machine import RTC
meses=["Enero","Febrero","Marzo","Abril","Mayo","Junio","Julio","Agosto", "Septiembre","Octubre","Noviembre", "Diciembre"]
rtc=RTC()
wlan = network.WLAN(mode=network.WLAN.STA)
#wlan.connect('MiFibra-3683',auth=(network.WLAN.WPA2, 'zvMrbC6X'))

#MATI PON AQUI TU RED PARA NO ESTAR ELIMINANDO EL DEL OTRO PARA HACER PRUEBAS
wlan.connect('MOVISTAR_4149',auth=(network.WLAN.WPA2, 's8JNj2DehX2DwpiGSCzx'))
time.sleep(1)
for i in range(10):
    if wlan.isconnected() == True:
        break
    print('.')
    time.sleep_ms(500)
if wlan.isconnected() == True:
    print(wlan.ifconfig())
    rtc.ntp_sync("hora.rediris.es")
    time.sleep(1)
    act=rtc.now()
    rtc.init((act[0],act[1],act[2],act[3]+2,act[4],act[5],0,0))
    time.sleep(0.2)
    act=rtc.now()
    print("son las %d:%d del %d de %s de %d" %(act[3],act[4],act[2],meses[act[1]-1], act[0] ))
    print("Sistema arrancado y conectado")
else:
    print("Sistema arrancado sin conexión")
