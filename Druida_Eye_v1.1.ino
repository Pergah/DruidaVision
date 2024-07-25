#include "TSL2561.h"
#include <EEPROM.h>
#include "Adafruit_TCS34725.h"
#define uvSensorPin A2
#define TCS34725_INTEGRATIONTIME TCS34725_INTEGRATIONTIME_50MS
#define TCS34725_GAIN TCS34725_GAIN_1X

TSL2561 tsl(TSL2561_ADDR_FLOAT); 
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME, TCS34725_GAIN);

struct DatosElemento {
  int color;
  float PPF;
  int Cantidad;
  float CantidadPorcentual;
};

DatosElemento datos;
int totalElementos, totalWatts;
float superficie, distanciaSensor, anguloApertura;
int totalChips = 0;
int totalPPF = 0;
float wattChip;

int valorUV;


void setup() {
  Serial.begin(9600);
  Serial.println("Programa de almacenamiento en EEPROM");

  // Cargar totalElementos desde la EEPROM
  EEPROM.get(0, totalElementos);
  if (totalElementos <= 0 || totalElementos > 6) {
    totalElementos = 6;
    EEPROM.put(0, totalElementos);
  }

  // Cargar totalWatts desde la EEPROM
  EEPROM.get(sizeof(int), totalWatts);

  // Cargar superficie, distancia Sensor y anguloApertura desde la EEPROM
  int address = sizeof(int) * 2 + totalElementos * sizeof(DatosElemento);
  EEPROM.get(address, superficie);
  EEPROM.get(address + sizeof(superficie), distanciaSensor);
  EEPROM.get(address + sizeof(superficie) + sizeof(distanciaSensor), anguloApertura);

  // Configuración del sensor TSL2561
 if (tsl.begin()) {
    Serial.println("TSL 2561 Found sensor");
  } else {
    Serial.println("No sensor?");
    while (1);
  }

    // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //tsl.setGain(TSL2561_GAIN_0X);         // set no gain (for bright situtations)
  tsl.setGain(TSL2561_GAIN_16X);      // set 16x gain (for dim situations)
  
  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_402MS);  // longest integration time (dim light)

  // Configuración del sensor AS7262
  /*if (!ams.begin()) {
    Serial.println("¡No se pudo conectar con el sensor AS7262! Por favor, verifica tu conexión.");
    while (1);
    delay(500);
  }*/

  delay(500);

  // Configuración del sensor TCS34725
  if (tcs.begin()) {
    Serial.println("TCS34725 Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }
  tcs.setInterrupt(false);

  // Espera para la inicialización y calibración
  delay(500);

  Serial.println("1. Modificar valores de configuracion luminica.");
  Serial.println("2. Mostrar los valores de configuracion luminica.");
  Serial.println("3. Tomar medicion y mostrar valores");
}

void loop() {
  char trash;
  if (Serial.available() > 0) {
    char entrada = Serial.read();
    if (entrada == '1') {
      while (Serial.available() > 0) {
        trash = Serial.read();
      }
      modificarValores();
    }

    if (entrada == '2') {
      while (Serial.available() > 0) {
        trash = Serial.read();
      }
      mostrarValores();
    }

    if (entrada == '3'){
      while (Serial.available() > 0) {
        trash = Serial.read();
      }
      tomarMedicion();
    }
  }
}

void mostrarValores() {
  // Reiniciar totalChips a cero
  totalChips = 0;
  totalPPF = 0;

  Serial.println("Valores almacenados en EEPROM:");
  for (int i = 0; i < totalElementos; i++) {
    EEPROM.get(i * sizeof(DatosElemento) + sizeof(int) * 2, datos); // Actualizado el desplazamiento

    Serial.print("Chip ");
    Serial.print(i + 1);
    Serial.print(": Color ");
    Serial.print(datos.color);
    if (datos.color > 1000) {
      Serial.print("°K");
    } else {
      Serial.print("nm");
    }
    Serial.print(", PPF/W ");
    Serial.print(datos.PPF);
    Serial.print(", Cantidad ");
    Serial.print(datos.Cantidad);
    Serial.print(", Cantidad Porcentual ");
    Serial.print(datos.CantidadPorcentual);
    Serial.println(" %");

    // Calcular la cantidad total de chips
    totalChips += datos.Cantidad;
    totalPPF += datos.Cantidad * datos.PPF;
  }

  // Calcular wattChip
  wattChip = totalWatts / (float)totalChips;
  int panelPPF = totalPPF * wattChip;

  Serial.print("Watts: ");
  Serial.println(totalWatts);
  Serial.print("Cantidad total de chips: ");
  Serial.println(totalChips);
  Serial.print("Watt/Chip: ");
  Serial.println(wattChip);
  Serial.print("Superficie (m^2): ");
  Serial.println(superficie);
  Serial.print("Distancia al sensor (m): ");
  Serial.println(distanciaSensor);
  Serial.print("Ángulo de apertura (grados): ");
  Serial.println(anguloApertura);
  Serial.print("PPF del panel: ");
  Serial.println(panelPPF);
  Serial.println("******************************");
}

void modificarValores() {
  int cantidadElementos = 6;
  int cantidadWatts;
  String input;

  while (Serial.available() > 0) {
    char trash = Serial.read();
  }

  while (!Serial.available()) {}
  
  while (true) {
    Serial.print("Ingrese la cantidad de variables de chips (entre 1 y 6): ");
    
    while (Serial.available() > 0) {
      char trash = Serial.read();
    }

    while (!Serial.available()) {}

    input = Serial.readStringUntil('\n');
    if (input.length() > 0) {
      cantidadElementos = input.toInt();
      if (cantidadElementos >= 1 && cantidadElementos <= 6) {
        totalElementos = cantidadElementos;
        Serial.println(totalElementos);
        EEPROM.put(0, totalElementos);
        break; // Salir del bucle while si la cantidad es válida
      } else {
        Serial.println("Cantidad inválida. Debe ser entre 1 y 6.");
      }
    }
  }

  Serial.print("Ingrese la potencia del panel (Watts): ");
  
  while (Serial.available() > 0) {
    char trash = Serial.read();
  }

  while (!Serial.available()) {}
  
  input = Serial.readStringUntil('\n');
  if (input.length() > 0) {
    cantidadWatts = input.toInt();
    totalWatts = cantidadWatts;
    Serial.println(totalWatts);
    EEPROM.put(sizeof(int), totalWatts); // Actualizado el desplazamiento
  }

  Serial.print("Ingrese la superficie (m^2): ");
  
  while (Serial.available() > 0) {
    char trash = Serial.read();
  }

  while (!Serial.available()) {}
  
  input = Serial.readStringUntil('\n');
  if (input.length() > 0) {
    superficie = input.toFloat();
    Serial.println(superficie);

    EEPROM.put(sizeof(int) * 2 + totalElementos * sizeof(DatosElemento), superficie);
  }

  Serial.print("Ingrese la distancia al sensor (m): ");
  
  while (Serial.available() > 0) {
    char trash = Serial.read();
  }

  while (!Serial.available()) {}
  
  input = Serial.readStringUntil('\n');
  if (input.length() > 0) {
    distanciaSensor = input.toFloat();
    Serial.println(distanciaSensor);

    EEPROM.put(sizeof(int) * 2 + totalElementos * sizeof(DatosElemento) + sizeof(superficie), distanciaSensor);
  }

  Serial.print("Ingrese el ángulo de apertura (grados): ");
  
  while (Serial.available() > 0) {
    char trash = Serial.read();
  }

  while (!Serial.available()) {}
  
  input = Serial.readStringUntil('\n');
  if (input.length() > 0) {
    anguloApertura = input.toFloat();
    Serial.print(anguloApertura);
    Serial.println("°");

    EEPROM.put(sizeof(int) * 2 + totalElementos * sizeof(DatosElemento) + sizeof(superficie) + sizeof(distanciaSensor), anguloApertura);
  }

  if (cantidadElementos > 0 && cantidadElementos <= 6) {
    int totalChips = 0;
    for (int i = 0; i < cantidadElementos; i++) {
      DatosElemento datos;

      Serial.print("Chip ");
      Serial.println(i + 1);
      
      Serial.print("Color(nm) o Temperatura(°K): ");

      while (Serial.available() > 0) {
    char trash = Serial.read();
  }

  while (!Serial.available()) {}
  
  input = Serial.readStringUntil('\n');
  if (input.length() > 0) {
    datos.color = input.toInt();
    Serial.println(datos.color);

  }


      Serial.print("Ingrese PPF/Watt: ");
      while (Serial.available() > 0) {
    char trash = Serial.read();
  }

  while (!Serial.available()) {}
  
  input = Serial.readStringUntil('\n');
  if (input.length() > 0) {
    datos.PPF = input.toFloat();
    Serial.println(datos.PPF);

  }

      Serial.print("Cantidad de chips: ");
      while (Serial.available() > 0) {
    char trash = Serial.read();
  }

  while (!Serial.available()) {}
  
  input = Serial.readStringUntil('\n');
  if (input.length() > 0) {
    datos.Cantidad = input.toInt();
    Serial.println(datos.color);

  }

      totalChips += datos.Cantidad;
      EEPROM.put(i * sizeof(DatosElemento) + sizeof(int) * 2, datos); // Actualizado el desplazamiento
    }

    // Calcular la cantidad porcentual después de haber obtenido el total de chips
    for (int i = 0; i < cantidadElementos; i++) {
      DatosElemento datos;
      EEPROM.get(i * sizeof(DatosElemento) + sizeof(int) * 2, datos);
      float cantidadPorcentual = (datos.Cantidad / (float)totalChips) * 100;
      datos.CantidadPorcentual = cantidadPorcentual;
      EEPROM.put(i * sizeof(DatosElemento) + sizeof(int) * 2, datos); // Actualizar la EEPROM con la cantidad porcentual
    }

    Serial.println("Valores modificados y almacenados en EEPROM.");
    mostrarValores();
  } else {
    Serial.println("Cantidad de elementos no válida. Debe ser entre 1 y 6.");
  }
}

void tomarMedicion() {
  uint16_t r, g, b, c, colorTemp, lux;
  uint16_t x;
  uint32_t lum;
  uint16_t ir, full;
  double PPFD, PFDR, PFDG, PFDB;

  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature_dn40(r, g, b, c);
  lux = tcs.calculateLux(r, g, b);

  // Leer el valor del sensor UV
  valorUV = analogRead(uvSensorPin); 

  // Leer luminosidad infrarroja del sensor TSL2561
  x = tsl.getLuminosity(TSL2561_INFRARED);

  // Leer luminosidad completa del sensor TSL2561
  lum = tsl.getFullLuminosity();
  ir = lum >> 16;
  full = lum & 0xFFFF;

  // Calcular PPFD
  PPFD = 0.00032056 * (c * c) + 0.685359 * c + 62.6962;

  // Calcular el total de R, G, B
  double total = r + g + b;

  // Calcular los porcentajes de cada color
  double percentageRed = r / total;
  double percentageGreen = g / total;
  double percentageBlue = b / total;

  // Calcular PFD-Red, PFD-Green y PFD-Blue
  PFDR = percentageRed * PPFD;
  PFDG = percentageGreen * PPFD;
  PFDB = percentageBlue * PPFD;

  delay(100); 
  Serial.println("****************************************");
  Serial.print("Lux: "); Serial.print(tsl.calculateLux(full, ir)); Serial.println("   Lumen / m2 ");
  Serial.print("PPFD: "); Serial.print(PPFD); Serial.println("    uMol / m2 s");
  Serial.print("PFD-UV: "); Serial.print(valorUV); Serial.println("   uMol / m2 s");
  Serial.print("PFD-B: "); Serial.print(PFDB); Serial.println("   uMol / m2 s");
  Serial.print("PFD-G: "); Serial.print(PFDG); Serial.println("   uMol / m2 s");
  Serial.print("PFD-R: "); Serial.print(PFDR); Serial.println("   uMol / m2 s");
  Serial.print("PFD-IR: "); Serial.print(x, DEC);  Serial.println("  uMol / m2 s");
  Serial.println("****************************************");

  delay(900);
}
