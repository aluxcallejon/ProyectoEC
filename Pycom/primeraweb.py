import network
import socket
import machine
import _thread
import time

from machine import Pin
from machine import UART
#Para la pantalla Nokia
from machine import SPI
import pcd8544
import framebuf

spi = SPI(0, mode=SPI.MASTER, baudrate=2000000, polarity=0, phase=0, pins=('P23','P22','P17'))
cs = Pin('P20')
dc = Pin('P21')
rst = Pin('P19')

# backlight on
bl = Pin('P11', Pin.OUT, value=1)

lcd = pcd8544.PCD8544(spi, cs, dc, rst)

buffer = bytearray((lcd.height // 8) * lcd.width)
framebuf = framebuf.FrameBuffer1(buffer, lcd.width, lcd.height)
framebuf.text('192.168.', 0, 0, 1)
lcd.data(buffer)
    #TX(PIN 3) RX(PIN 4)
uart = UART(1, 9600)                         # init with given baudrate
uart.init(9600, bits=8, parity=None, stop=1) # init with given parameters




#Fichero HTML a mandar
html1 = """
<html>
<script>
var lastX=128;
var lastY=128;
var myVar = setInterval(loadDoc, 200);
function loadDoc() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
     document.getElementById("demo").innerHTML = this.responseText;
     var info=JSON.parse(this.responseText);

     ctx.clearRect(0,0,512,512);
     drawLine(ctx,256,0,256,512,"#1EB4C8");
     drawLine(ctx,0,256,512,256,"#1EB4C8");

     drawCircle(info.posx*2,info.posy*2);

    }
  };
  xhttp.open("GET", "datos.txt", true);
  xhttp.send();
}
</script>

<head>
<title>Mi primera página web </title>
</head>
<body>
<h1 align="center" >Mi Primera pagina web </h1>
<hr>
<p>Esto tan sencillo es una verdadera página web, aunque le falta el contenido,
pero todo llegará.</p>
<p id="demo"></p>

<h2> FRASE DEL DÍA: BUENOS DIAS </h2>
<canvas id="myCanvas" width="512" height="512" style="border:1px solid #d3d3d3;">
Your browser does not support the HTML canvas tag.</canvas>
<p> Cambia </p>
<script>
var c = document.getElementById("myCanvas");
var ctx = c.getContext("2d");


function drawLine(ctx, startX, startY, endX, endY,color){
    ctx.beginPath();
    ctx.strokeStyle = color;
    ctx.moveTo(startX,startY);
    ctx.lineTo(endX,endY);
    ctx.stroke();
}

drawLine(ctx,256,0,256,512,"#1EB4C8");
drawLine(ctx,0,256,512,256,"#1EB4C8");

function drawCircle(movx,movy){
    ctx.beginPath();
    ctx.fillStyle ="#D81515";
    ctx.arc(movx,512-movy,30,0,2*Math.PI);
    ctx.fill();
    ctx.stroke();
}

</script>

</body>
</html>
"""
input()
Boton=Pin('P10',Pin.IN)

#Setup Socket WebServer
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind(('', 80))
s.listen(5)

while True:

    conn, addr = s.accept()
    print("Got a connection from %s" % str(addr))
    request = conn.recv(1024)
    framebuf.text('%s'% str(addr), 0, 12, 1)
    lcd.data(buffer)

    print(request)
    if 'datos.txt' in request:


        uart.write('r')
        time.sleep(0.05)

        modo=int.from_bytes(uart.read(1),False) # read up to 1 bytes
        print("%d",modo)
        posx=int.from_bytes(uart.read(1),False) # read up to 1 bytes
        print("%d",posx)
        posy=int.from_bytes(uart.read(1),False) # read up to 1 bytes
        print("%d",posy)

        response='{"vel":"%d","posx":"%d","posy":"%d"}'%(modo,posx,posy)
    else:
        response = html1

    conn.send(response)
    conn.close()
    if Boton() == 0:
        break
s.close()
