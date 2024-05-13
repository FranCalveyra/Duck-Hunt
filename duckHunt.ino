//Declaro las constantes, como los pines a usar y el tiempo total de juego
const int finDeCarrera = 34;
const int led = 26;
const unsigned long tiempoTotalDeJuego = 30000; //En milisegundos
//Declaro las variables
unsigned long antesDePegar = 0;
unsigned long tiempoAlPegar = 0;
int puntaje;
int dificultad;

/*FALTA: 
- Modularizar y escribir las cosas en funciones más chicas
- Conectar con MQTT
- Apagar el led si no le pegó en 5 segundos y restar 5 puntos
*/
void setup() {
  //Inicializo las variables y los dispositivos
  Serial.begin(9600);
  pinMode(finDeCarrera, INPUT);
  pinMode(led, OUTPUT);
  antesDePegar = millis();
  puntaje = 0;
  //Acá iría el código de MQTT relacionado con obtener la dificultad del próximo juego, validando al jugador, etc.
  //Por ahora, la dificultad estará hardcodeada en 0
  dificultad = 0;
}

void loop() {  
  ejecutarJuego(dificultad);

}

void ejecutarJuego(int nivelDeDificultad){
  int FDC = digitalRead(finDeCarrera);
  bool patoGolpeado = FDC == LOW;
  bool activoParaPegar = true;
  int intervaloMaximo = obtenerIntervalo(nivelDeDificultad);

  if(activoParaPegar){
    digitalWrite(led, HIGH);
    if(patoGolpeado){
      tiempoAlPegar = millis();
      if(tiempoAlPegar - antesDePegar> intervaloMaximo ){ //Delay parametrizado con la dificultad
        Serial.println("No le pegaste a tiempo, -5 puntos"); //Alternar puntaje 
        cambiarPuntaje(puntaje,-5);
      }
      else{
        Serial.println("Golpeaste al pato! +5 puntos");
        cambiarPuntaje(puntaje,5);
      }
      activoParaPegar = false;
      digitalWrite(led, LOW);
      antesDePegar = millis();
      delay(1000);
    }
  }
  unsigned long tiempoDeJuego = millis();
  if(tiempoDeJuego - antesDePegar >= tiempoTotalDeJuego){ //Después de 30 segundos, terminaría
    Serial.println("Tu puntaje fue: ");
    Serial.println(puntaje);
    antesDePegar = millis(); //Reinicio el tiempo de juego
    puntaje = 0;
    //Enviar el puntaje por MQTT al server
  }
}

int obtenerIntervalo(int nivelDeDificultad){
  return 5000 - 1000*nivelDeDificultad; //Máximo nivel: 3
}

void cambiarPuntaje(int puntos, int delta){
  puntos +=delta;
}



