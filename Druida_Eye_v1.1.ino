#include "TSL2561.h"
#include "Adafruit_AS726x.h"
#include <EEPROM.h>
#include "Adafruit_TCS34725.h"
#define uvSensorPin A2
#define TCS34725_INTEGRATIONTIME TCS34725_INTEGRATIONTIME_50MS
#define TCS34725_GAIN TCS34725_GAIN_1X

Adafruit_AS726x ams;
uint16_t sensorValues[AS726x_NUM_CHANNELS];
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
    Serial.println("Found sensor");
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
  if (!ams.begin()) {
    Serial.println("¡No se pudo conectar con el sensor AS7262! Por favor, verifica tu conexión.");
    while (1);
    delay(500);
  }

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
    Serial.println(anguloApertura);

    EEPROM.put(sizeof(int) * 2 + totalElementos * sizeof(DatosElemento) + sizeof(superficie) + sizeof(distanciaSensor), anguloApertura);
  }

  Serial.println("Ingrese los valores de los chips:");
  for (int i = 0; i < totalElementos; i++) {
    Serial.print("Ingrese el valor del color del Chip ");
    Serial.print(i + 1);
    Serial.println(" (en nanómetros o grados Kelvin): ");

    while (Serial.available() > 0) {
      char trash = Serial.read();
    }

    while (!Serial.available()) {}

    input = Serial.readStringUntil('\n');
    if (input.length() > 0) {
      datos.color = input.toInt();
    }

    Serial.print("Ingrese el valor del PPF por Watt del Chip ");
    Serial.print(i + 1);
    Serial.println(": ");

    while (Serial.available() > 0) {
      char trash = Serial.read();
    }

    while (!Serial.available()) {}

    input = Serial.readStringUntil('\n');
    if (input.length() > 0) {
      datos.PPF = input.toFloat();
    }

    Serial.print("Ingrese la cantidad de chips ");
    Serial.print(i + 1);
    Serial.println(": ");

    while (Serial.available() > 0) {
      char trash = Serial.read();
    }

    while (!Serial.available()) {}

    input = Serial.readStringUntil('\n');
    if (input.length() > 0) {
      datos.Cantidad = input.toInt();
    }

    Serial.print("Ingrese la cantidad porcentual de chips ");
    Serial.print(i + 1);
    Serial.println(": ");

    while (Serial.available() > 0) {
      char trash = Serial.read();
    }

    while (!Serial.available()) {}

    input = Serial.readStringUntil('\n');
    if (input.length() > 0) {
      datos.CantidadPorcentual = input.toFloat();
    }

    EEPROM.put(i * sizeof(DatosElemento) + sizeof(int) * 2, datos); // Actualizado el desplazamiento
  }

  Serial.println("Valores almacenados correctamente en EEPROM.");
}

void tomarMedicion() {
  // Realizar la medición del sensor UV
  valorUV = analogRead(uvSensorPin);
  float voltaje = valorUV * (5.0 / 1023.0);
  float intensidadUV = voltaje * 307; // Sensibilidad del sensor UV

  // Mostrar los valores medidos
  Serial.print("Valor UV: ");
  Serial.println(valorUV);
  Serial.print("Voltaje: ");
  Serial.print(voltaje, 2);
  Serial.println(" V");
  Serial.print("Intensidad UV: ");
  Serial.print(intensidadUV, 2);
  Serial.println(" mW/cm^2");

 // Simple data read example. Just read the infrared, fullspecrtrum diode 
  // or 'visible' (difference between the two) channels.
  // This can take 13-402 milliseconds! Uncomment whichever of the following you want to read
  uint16_t x = tsl.getLuminosity(TSL2561_VISIBLE);     
  //uint16_t x = tsl.getLuminosity(TSL2561_FULLSPECTRUM);
  //uint16_t x = tsl.getLuminosity(TSL2561_INFRARED);
  
  Serial.println(x, DEC);

  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparisons you want!
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  Serial.print("IR: "); Serial.print(ir);   Serial.print("\t\t");
  Serial.print("Full: "); Serial.print(full);   Serial.print("\t");
  Serial.print("Visible: "); Serial.print(full - ir);   Serial.print("\t");
  Serial.print("Lux: "); Serial.println(tsl.calculateLux(full, ir));

  // Leer valores de RGB del sensor TCS34725
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  Serial.print("Rojo: "); Serial.println(r);
  Serial.print("Verde: "); Serial.println(g);
  Serial.print("Azul: "); Serial.println(b);
  Serial.print("Claro: "); Serial.println(c);

  // Leer valores del sensor AS7262
  ams.readRawValues(sensorValues);
  for (int i = 0; i < AS726x_NUM_CHANNELS; i++) {
    Serial.print("Valor del canal ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(sensorValues[i]);
  }
}
