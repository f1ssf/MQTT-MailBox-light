// GNU Franck Dubuis - 01/2025
// Version boite aux lettres connectées avec ESP32
// plusieurs sensors : colis et lettre et presence, rssi, batterie, ip
// telemetrie a chaque publication et toutes les 10 minutes
// reveil pour RAZ des topics toutes les 5 minutes


#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <time.h>



// Configuration Wi-Fi et MQTT
const char* ssid = "your_ssid";
const char* password = "your_password";
const char* mqtt_server = "brooker_ip";
const char* mqtt_user = "brooker_user";
const char* mqtt_password = "brooker_password";
const char* topic_lettre = "boite/lettres";
const char* topic_colis = "boite/colis";
const char* topic_presence = "boite/presence";
const char* topic_batterie = "boite/batterie";
const char* topic_rssi = "boite/rssi";
const char* topic_ip = "boite/ip";

// GPIO pour les capteurs et mesure de batterie
const int PIN_LETTRES = 27;
const int PIN_COLIS = 15;
const int PIN_PRESENCE = 4;
const int PIN_BATTERIE = 33;  // ADC pour la batterie

// Pont diviseur
const float FACTEUR_PONT_DIVISEUR = 3.8;  // Ajuster en fonction de vos résistances, 3.8 dans mon cas
const float REF_ADC = 3.3;  // Référence ADC (tension maximale)
const int RESOLUTION_ADC = 4096;  // Résolution de l'ADC

// Durée du Timer RTC en microsecondes (5 minutes)
const uint64_t TIMER_WAKEUP_US = 300000000;  // 5 minutes en microsecondes

WiFiClient espClient;
PubSubClient client(espClient);

// Definitions des fonctions
void envoyerMessageMQTT(const char* topic, const char* message);
void envoyerTensionBatterie();
void effacerMessageMQTT(const char* topic);
void connecterWiFi();
void publishNetworkData();
float mesurerBatterie();
void entrerEnVeille();
void verifierEtatWiFi();

// ici le setup
void setup() {
    Serial.begin(115200);

    // Configurer les GPIO comme entrées
    pinMode(PIN_LETTRES, INPUT);
    pinMode(PIN_COLIS, INPUT);
    pinMode(PIN_PRESENCE, INPUT);
    pinMode(PIN_BATTERIE, INPUT);

    uint64_t wakeupPins = 0;

    // Vérification de la cause du réveil
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("Réveil par capteur (EXT1).");
            wakeupPins = esp_sleep_get_ext1_wakeup_status();

            if (wakeupPins & (1ULL << PIN_LETTRES) && digitalRead(PIN_LETTRES) == HIGH) {
                envoyerMessageMQTT(topic_lettre, "Lettre détectée");
                envoyerTensionBatterie();
                publishNetworkData();  // Publier RSSI et IP après détection
            }
            if (wakeupPins & (1ULL << PIN_COLIS) && digitalRead(PIN_COLIS) == HIGH) {
                envoyerMessageMQTT(topic_colis, "Colis détecté");
                envoyerTensionBatterie();
                publishNetworkData();  // Publier RSSI et IP après détection
            }
            if (wakeupPins & (1ULL << PIN_PRESENCE) && digitalRead(PIN_PRESENCE) == HIGH) {
                envoyerMessageMQTT(topic_presence, "Présence détectée");
                envoyerTensionBatterie();
                publishNetworkData();  // Publier RSSI et IP après détection
            }
            break;

        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Réveil par Timer RTC. Effacement des messages...");
            effacerMessageMQTT(topic_lettre);
            effacerMessageMQTT(topic_colis);
            effacerMessageMQTT(topic_presence);
            break;

        default:
            Serial.println("Réveil pour une autre raison.");
            break;
    }

    // Configurer les GPIO pour réveil par capteurs
    esp_sleep_enable_ext1_wakeup(
        (1ULL << PIN_LETTRES) | (1ULL << PIN_COLIS) | (1ULL << PIN_PRESENCE),
        ESP_EXT1_WAKEUP_ANY_HIGH
    );

    // Configurer le Timer RTC pour réveil dans 5 minutes
    esp_sleep_enable_timer_wakeup(TIMER_WAKEUP_US);

    entrerEnVeille();
}

void loop() {
    // Rien à exécuter grâce au mode veille profonde
}

// ici les fonctions avec differents parametres de log et securisation.

void connecterWiFi() {
    Serial.println("Connexion WiFi...");
    WiFi.begin(ssid, password);

    int essais = 0;
    while (WiFi.status() != WL_CONNECTED && essais < 20) {  // Timeout après 10 secondes
        delay(500);
        Serial.print(".");
        essais++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi connecté.");
        delay(1000);  // Stabilisation de la connexion
    } else {
        Serial.println("\nErreur : connexion Wi-Fi échouée.");
        verifierEtatWiFi();  // Diagnostic si connexion échouée
    }
}

void verifierEtatWiFi() {
    wl_status_t status = WiFi.status();
    Serial.print("Wi-Fi Status: ");
    switch (status) {
        case WL_IDLE_STATUS:
            Serial.println("IDLE");
            break;
        case WL_NO_SSID_AVAIL:
            Serial.println("SSID non disponible");
            break;
        case WL_SCAN_COMPLETED:
            Serial.println("Scan terminé");
            break;
        case WL_CONNECTED:
            Serial.println("Connecté");
            break;
        case WL_CONNECT_FAILED:
            Serial.println("Connexion échouée");
            break;
        case WL_CONNECTION_LOST:
            Serial.println("Connexion perdue");
            break;
        case WL_DISCONNECTED:
            Serial.println("Déconnecté");
            break;
        default:
            Serial.println("État inconnu");
            break;
    }
}


void publishNetworkData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Erreur : Wi-Fi non connecté. Impossible de publier RSSI et IP.");
        verifierEtatWiFi();
        return;
    }

    // Obtenir le RSSI
    int rssi = WiFi.RSSI();
    char rssiMessage[50];
    snprintf(rssiMessage, sizeof(rssiMessage), "RSSI: %d dBm", rssi);

    // Obtenir l'adresse IP
    String ipAddress = WiFi.localIP().toString();
    char ipMessage[50];
    snprintf(ipMessage, sizeof(ipMessage), "IP: %s", ipAddress.c_str());

    // Publier le RSSI
    Serial.print("Publishing RSSI: ");
    Serial.println(rssi);
    envoyerMessageMQTT(topic_rssi, rssiMessage);

    // Publier l'adresse IP
    Serial.print("Publishing IP: ");
    Serial.println(ipAddress);
    envoyerMessageMQTT(topic_ip, ipMessage);
}


void envoyerMessageMQTT(const char* topic, const char* message) {
    connecterWiFi();
    client.setServer(mqtt_server, 1883);

    if (!client.connected()) {
        client.connect("ESP32_Client", mqtt_user, mqtt_password);
    }

    if (client.publish(topic, message, true)) {
        Serial.println("Message publié avec persistance : " + String(message));
    } else {
        Serial.println("Échec de publication MQTT.");
    }

    client.disconnect();
}

void envoyerTensionBatterie() {
    float tension = mesurerBatterie();
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "Tension batterie : %.2fV", tension);
    envoyerMessageMQTT(topic_batterie, buffer);
    Serial.println(buffer);
}

float mesurerBatterie() {
    int valeurADC = analogRead(PIN_BATTERIE);
    float tensionADC = (valeurADC * REF_ADC) / RESOLUTION_ADC;
    return tensionADC * FACTEUR_PONT_DIVISEUR;
}

void effacerMessageMQTT(const char* topic) {
    connecterWiFi();
    client.setServer(mqtt_server, 1883);

    if (!client.connected()) {
        client.connect("ESP32_Client", mqtt_user, mqtt_password);
    }

    if (client.publish(topic, "", true)) {  // Message vide avec retain=true
        Serial.println("Message effacé : " + String(topic));
    } else {
        Serial.println("Échec de suppression du message.");
    }

    client.disconnect();
    WiFi.disconnect(true);
}

void entrerEnVeille() {
    Serial.println("Déconnexion Wi-Fi...");
    WiFi.disconnect(true);
    delay(100);  // Petit délai pour garantir une déconnexion propre

    Serial.println("Entrée en veille profonde...");
    esp_deep_sleep_start();
}
