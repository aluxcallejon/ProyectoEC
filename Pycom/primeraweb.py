import network
import socket
import machine
import _thread
import time

from machine import Pin
from machine import UART


    #TX(PIN 3) RX(PIN 4)
uart = UART(1, 9600)                         # init with given baudrate
uart.init(9600, bits=8, parity=None, stop=1) # init with given parameters


#Fichero HTML a mandar
html1 = """
<html>
<script>
var myVar = setInterval(loadDoc, 500);
function loadDoc() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
     document.getElementById("demo").innerHTML = this.responseText;
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

<h2> FRASE DEL DÍA: BUENOS DIAS <hr>
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

    print(request)
    if 'datos.txt' in request:

        while(uart.any()==0):
            uart.write('r')
        datos=uart.read(1) # read up to 1 bytes
        print(datos)
        response='{"vel":"%s"}'%datos
    else:
        response = html1

    conn.send(response)
    conn.close()
    if Boton() == 0:
        break
s.close()
