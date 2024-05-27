#include <WiFi.h> // Para ESP32
WiFiClient WIFI_CLIENT;
#include <PubSubClient.h>
PubSubClient MQTT_CLIENT;
#include <stdio.h>
#include <ArduinoJson.h> //Instalarla en caso de no tenerla


//Declaro las constantes, como los pines a usar y el tiempo total de juego
const int finDeCarrera = 34;
const int led = 26;
const unsigned long tiempoTotalDeJuego = 10000; //En milisegundos (en este caso son 10 segundos)
//Declaro las variables de tiempo para el juego en cuestión
unsigned long antesDePegar = 0;
unsigned long momentoDeInicio = 0;
unsigned long tiempoAlPegar = 0;

int puntaje;
bool conectado;

// Nombre y contraseña red WiFi.
const char* ssid = "UA-Alumnos"; //A modificar según sea necesario
const char* password = "41umn05WLC"; //A modificar según sea necesario
const int ledPin = 26;

//Cosas del server
const char* serverIp = "44.205.248.202"; //A modificar según sea necesario
char* topicDeSuscripcion ="game/init/1";
char* topicDePublicacion = "game/result/1";


//JSON
StaticJsonDocument<200> doc;
const char* nombreJugador = "";
int dificultad;

//Lo que va a usar la ESP32
void setup() {
  //Inicializo las variables y los dispositivos
  Serial.begin(115200);
  pinMode(finDeCarrera, INPUT);
  pinMode(led, OUTPUT);
  antesDePegar = millis();
  puntaje = 5; //Hardcode por la diferencia de tiempo entre la solicitud del server y el inicio del juego
  conectado = false;
  //Inicializo la conexión con el server y me traigo los datos del jugador
  inicializarConexion();
}

void loop() {
  conectarConMQTT();
  momentoDeInicio = millis();
  while(conectado) {
    ejecutarJuego(dificultad);
    }
}

//A partir de acá son sólo funciones auxiliares y el cómo funciona por abajo

//MQTT
void conectarConMQTT(){
  if (!MQTT_CLIENT.connected()) {
    reconnect();
  }
  MQTT_CLIENT.loop();
}


// Reconecta con MQTT broker
void reconnect() {
   MQTT_CLIENT.setServer(serverIp, 1883);  // si se usa un servidor en la red local, verificar IP
     //MQTT_CLIENT.setServer("broker.hivemq.com", 1883);

  MQTT_CLIENT.setClient(WIFI_CLIENT);

  // Intentando conectar con el broker.
  while (!MQTT_CLIENT.connected()) {
    Serial.println("Intentando conectar con MQTT.");
    MQTT_CLIENT.connect("server");   // usar un nombre aleatorio
    //                      topic /  valor
    MQTT_CLIENT.subscribe(topicDeSuscripcion);

    // Espera antes de volver a intentarlo.
    delay(3000);
  }

  Serial.println("Conectado a MQTT.");
}
// Aquí configuramos lo que debe hacer cuando recibe un valor.
void callback(char* recibido, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido: ");
  Serial.println(recibido);

  char* json = new char[length+1] ;
  parsearJson(json, payload, length);
  setearValores();
}

void inicializarConexion(){
    // Conectar con WiFi.
  Serial.println();
  Serial.print("Conectando con ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Configuración de la respuesta.
  MQTT_CLIENT.setCallback(callback);
}

void setearValores(){
  nombreJugador = doc["player"];
  dificultad = doc["difficulty"];
  conectado = true;
}

void parsearJson(char* json, byte* payload, unsigned int length){
  //Copiar lo que llega de payload
  for (int i = 0; i < length; i++) {
    json[i] = (char)payload[i];
  }

  //Dejar el último en NULL
  json[length] = '\0';
  Serial.println(json);
  
  //Catchear errores
  DeserializationError error = deserializeJson(doc,json);
  if(error){
    Serial.print(F("deserializeJson() falló con el código "));
    Serial.println(error.c_str());
    return;
  }
}

//JUEGO
void ejecutarJuego(int nivelDeDificultad){

  int FDC = digitalRead(finDeCarrera);
  bool patoGolpeado = FDC == LOW;
  bool activoParaPegar = true;
  int intervaloMaximo = obtenerIntervalo(nivelDeDificultad);
  
  if(activoParaPegar){
    digitalWrite(led, HIGH);
    if(patoGolpeado){
      tiempoAlPegar = millis();
      if(tiempoAlPegar - antesDePegar<= intervaloMaximo){ //Delay parametrizado con la dificultad
        Serial.println("Golpeaste al pato! +5 puntos");
        cambiarPuntaje(5);
      }
      else{
        Serial.println("No le pegaste a tiempo, -5 puntos"); //Alternar puntaje 
        cambiarPuntaje(-5);
      }
      reiniciar(activoParaPegar);
    } else if (millis() - antesDePegar >intervaloMaximo){
      Serial.println("No le pegaste a tiempo, -5 puntos");
      cambiarPuntaje(-5);
      reiniciar(activoParaPegar);
    }
  }
  verificarFin();
}

int obtenerIntervalo(int nivelDeDificultad){
  return 6000 - 1000*nivelDeDificultad; //Máximo nivel: 3, Mínimo: 1
}

void cambiarPuntaje(int delta){
  puntaje += delta;
  Serial.println(puntaje);
}

void reiniciar(bool activoParaPegar){
  activoParaPegar = false;
  digitalWrite(led, LOW);
  antesDePegar = millis();
  delay(1000);
}

void verificarFin(){
  unsigned long tiempoDeJuego = millis();
  if(tiempoDeJuego - momentoDeInicio >= tiempoTotalDeJuego){ //Después de 30 segundos, terminaría
    Serial.println("Tu puntaje fue: ");
    Serial.println(puntaje);
    momentoDeInicio = millis(); //Reinicio el tiempo de juego
    //Parseo el mensaje
    char mensaje[50];
    snprintf(mensaje,sizeof(mensaje),"{\"player\":\"%s\",\"points\":%d,\"difficulty\":%d}", nombreJugador, puntaje, dificultad);
    
    //Publico el mensaje
    MQTT_CLIENT.publish(topicDePublicacion, mensaje);
    
    Serial.println(mensaje);
    //Reinicio las variables
    puntaje = 5; //Hardcode por la diferencia de tiempo entre la solicitud del server y el inicio del juego
    conectado = false;
  }
}