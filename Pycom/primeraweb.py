import network
import socket
import machine
import json

from machine import Pin
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
i=0;
while True:
    conn, addr = s.accept()
    print("Got a connection from %s" % str(addr))
    request = conn.recv(1024)

    print(request)
    if 'datos.txt' in request:
        response='{"vel":"%s"}'%i
    else:
        response = html1

    conn.send(response)
    conn.close()
    i=i+1
    if Boton() == 0:
        break
s.close()
