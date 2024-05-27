#include <WiFi.h> // Para ESP32
WiFiClient WIFI_CLIENT;
#include <PubSubClient.h>
PubSubClient MQTT_CLIENT;
#include <stdio.h>
#include <ArduinoJson.h>


//Declaro las constantes, como los pines a usar y el tiempo total de juego
const int finDeCarrera = 34;
const int led = 26;
const unsigned long tiempoTotalDeJuego = 10000; //En milisegundos
//Declaro las variables
unsigned long antesDePegar = 0;
unsigned long momentoDeInicio = 0;
unsigned long tiempoAlPegar = 0;

int puntaje;
bool conectado = false;

// Nombre y contraseña red WiFi.
const char* ssid = "UA-Alumnos";
const char* password = "41umn05WLC";
const int ledPin = 26;
//Cosas del server
const char* serverIp = "44.205.248.202"; //A modificar según sea necesario
char* subscribeTopicName ="game/init/1";
char* publishTopicName = "game/result/1";


//JSON
StaticJsonDocument<200> doc;
const char* nombreJugador = "";
int dificultad;


/*FALTA: 
- Conectar con MQTT
*/
void setup() {
  //Inicializo las variables y los dispositivos
  Serial.begin(115200);
  pinMode(finDeCarrera, INPUT);
  pinMode(led, OUTPUT);
  antesDePegar = millis();
  momentoDeInicio = millis();
  puntaje = 0;
  //Acá iría el código de MQTT relacionado con obtener la dificultad del próximo juego, validando al jugador, etc.
  inicializarConexion();
  //Necesito hacer un "fetch" de la dificultad, el nombre del jugador, etc. Pero para eso, necesito parsear un JSON
  //Por ahora, la dificultad estará hardcodeada en 1

}

void loop() {  

//Descomentar para comentar con el server
  conectarConMQTT();
  while(conectado) 
  ejecutarJuego(dificultad);
  
}


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
    MQTT_CLIENT.subscribe(subscribeTopicName);

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
  
  for (int i = 0; i < length; i++) {
    json[i] = (char)payload[i];
  }
  json[length] = '\0';
  Serial.println(json);

  DeserializationError error = deserializeJson(doc,json);
  if(error){
    Serial.print(F("deserializeJson() falló con el código "));
    Serial.println(error.c_str());
    return;
  }
  setearValores();
  conectado = true;
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
  return 5000 +1000- 1000*nivelDeDificultad; //Máximo nivel: 3
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

    char mensaje[50];
    snprintf(mensaje,sizeof(mensaje),"{\"player\":\"%s\",\"points\":%d,\"difficulty\":%d}",nombreJugador, puntaje, dificultad);
    char* mensajeReal = mensaje;
    MQTT_CLIENT.publish(publishTopicName, mensajeReal);
    Serial.println(mensaje);
    puntaje = 0;
    conectado = false;
    //TODO: Enviar el puntaje por MQTT al server
  }
}

void setearValores(){
  nombreJugador = doc["player"];
  dificultad = doc["difficulty"];
}