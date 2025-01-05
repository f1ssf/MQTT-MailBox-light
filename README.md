# MQTT-MailBox

An MQTT notify system with an ESP32 and deepsleep

compared to the other project Box-211224, same logic.

This code is more simply, only:

presence, letter, parcel, V batterie, rssi, IP

each evenements publish on mqtt brooker

each 5 minutes, mqqt topics are resetting

each 10 minutes, batterie, rssi, ip  are plublied.

no sensors currents, temp, RH


# CONFIG.YAML in Home Assistant
    - name: "Tension Batterie"
      state_topic: "boite/batterie"
      unit_of_measurement: "V"
      value_template: "{{ value.split(': ')[1].split('V')[0] | float }}"

      unique_id: "mailbox_battery_voltage"

    - name: "RSSI"
      state_topic: "boite/rssi"
      unit_of_measurement: "dBm"
      value_template: "{{ value.split(': ')[1].split(' ')[0] | int }}"

      unique_id: "mailbox_rssi"

    - name: "Adresse IP"
      state_topic: "boite/ip"
      value_template: "{{ value.split(': ')[1] }}"
      unique_id: "mailbox_ip_address"

    - name: "Détection Lettres"
      state_topic: "boite/lettres"
      unique_id: "mailbox_letters_detected"

    - name: "Détection Colis"
      state_topic: "boite/colis"
      unique_id: "mailbox_parcel_detected"

    - name: "Détection Présence"
      state_topic: "boite/presence"
      unique_id: "mailbox_presence_detected"
