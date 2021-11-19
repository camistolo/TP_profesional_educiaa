# Código para la comunicación con el módulo WiFi (ESP-12)

La EDU-CIAA recive un string del tipo >n<, donde n es un número del 1 al 4. Cada uno de estos valores corresponde a una medición distinta. 
Entonces se parsea este valor y dependiendo del valor recibido se realiza la correspondiente medición. Luego se envia el dato al módulo WiFi y lo publica. 