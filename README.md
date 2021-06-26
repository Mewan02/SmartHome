# SmartHome
Projet maison connectée codé en arduino pour une Arduido Uno et un ESP8266 et en C++ pour la rasperry pi

A smart home coded in Arduino for Arduino Uno and ESP8266 and C++ for Raspberry Pi


-------------------English Below-------------------

Bonjour,
J'ai eu du mal à finir ce projet dans les temps, et j'ai été beaucoup aidé par d'autre sur internet donc j'apporte ma pierre à l'édifice en publiant mon code et mes résultats.

Le projet est constitué de trois parties : 
- Partie récolte de données et affichage avec une Arduino Uno qui utilise : un écran LCD, un capteur de température, une LED et une photorésistance. Le LCD affiche le nombre de personne dans la maison + la température. L'intensité de la LED change ne fonction de la luminosité ambiante. Les données sont envoyés à l'ESP8266 via SPI.
- Partie sécurité avec un ESP8266 qui utilise : un servomoteur qui ouvre la porte de la maison, un capteur RFID pour les cartes d'entrées, un bouton qui ouvre la porte de l'intérieur. Les données sont envoyés au broker MQTT par wifi.
- Partie server avec une Raspberry pi 3 qui utilise un broker MQTT et ifttt


-------------------English Part-------------------

Hello,
I just want to help for guys like me who hardly found answers on the internet. I'm sorry for my English in advance.
The project is in three parts:
- The first part is for display input and output of the house (Arduino UNO): screen LCD, temperature sensor, an LED and a photodiode. The LCD display the number of user inside of the house + the temperature. The intensity of the LED is proportional to the luminosity of the house. Data are sent to the ESP8266 via SPI.
- The second part is for security with an ESP8266: a servomotor who open the house's door, an RFID captor for id's users, a button who open from inside. Data are sent to MQTT server via wifi.
- The last part is the server part with a raspberry pi 3 who use MQTT and ifttt

