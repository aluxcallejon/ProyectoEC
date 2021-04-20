import network
import socket
import machine

from machine import Pin
#Fichero HTML a mandar
html1 = """
<html>
<head>
<title>Mi primera página web </title>
</head>
<body>
<h1 align="center" >Mi Primera pagina web </h1>
<hr>
<p>Esto tan sencillo es una verdadera página web, aunque le falta el contenido,
pero todo llegará.</p>
<h2> FRASE DEL DÍA: %s <hr>
</body>
</html>
"""
input()
Boton=Pin('P10',Pin.IN)
frase=input("Mete frase del día:")

#Setup Socket WebServer
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind(('', 80))
s.listen(5)
while True:
    conn, addr = s.accept()
    print("Got a connection from %s" % str(addr))
    request = conn.recv(1024)
    print(request)
    response = html1 %frase
    conn.send(response)
    conn.close()
    if Boton() == 0:
        break
s.close()
