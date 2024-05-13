const int finDeCarrera = 34;
const int led = 26;
bool patoFueGolpeado = false;
unsigned long antesDePegar = 0;
unsigned long tiempoAlPegar = 0;
int puntaje;
/*FALTA: 
- Modularizar y escribir las cosas en funciones más chicas
- Conectar con MQTT
- Apagar el led si no le pegó en 5 segundos y restar 5 puntos
*/
void setup() {
  //Inicializo
  Serial.begin(9600);
  pinMode(finDeCarrera, INPUT);
  pinMode(led, OUTPUT);
  antesDePegar = millis();
  puntaje = 0;
}

void loop() {  
  // Lectura del botón
  int FDC = digitalRead(finDeCarrera);
  bool patoGolpeado = FDC == LOW;
  bool activoParaPegar = true;
  if(activoParaPegar){
    digitalWrite(led, HIGH);
    if(patoGolpeado){
      tiempoAlPegar = millis();
      if(tiempoAlPegar - antesDePegar> 5000 ){ //Delay parametrizado con la dificultad
        Serial.println("No le pegaste a tiempo, -5 puntos"); //Alternar puntaje 
        puntaje-=5;
      }
      else{
        Serial.println("Golpeaste al pato! +5 puntos");
        puntaje+=5;
      }
      activoParaPegar = false;
      digitalWrite(led, LOW);
      antesDePegar = millis();
      delay(1000);
    }
  }
  unsigned long tiempoDeJuego = millis();
  if(tiempoDeJuego - antesDePegar >= 20000){ //Después de 10 segundos, terminaría
    Serial.println("Tu puntaje fue: ");
    Serial.println(puntaje);
    antesDePegar = millis(); //Reinicio el tiempo de juego
    puntaje = 0;
    //Enviar el puntaje por MQTT al server
  }

  
  
  /*
  - el led está prendido
  - le pego al pato
  - si le pego antes de los 5 segundos, sumo 5 puntos. Sino, resto 5 puntos
  */


}



