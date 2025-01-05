# MQTT-MailBox

An MQTT notify system with an ESP32 and deepsleep

compared to the other project Box-211224, same logic.

This code is more simply, only:

presence, letter, parcel, V batterie, rssi, IP

each evenements publish on mqtt brooker

each 5 minutes, mqqt topics are resetting

each 10 minutes, batterie, rssi, ip  are plublied.

no sensors currents, temp, RH
