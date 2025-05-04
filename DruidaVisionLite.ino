#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TCS34725.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#define EEPROM_SIZE 12
#include <vector>
#include <cmath>

// Configuración de la red WiFi en modo AP
const char* ssid = "DruidaVision";
const char* password = "";  // Deja la contraseña vacía para una red abierta

float coefA0, coefA1, coefA2;

WebServer server(80);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Valores de color
uint16_t clear = 0, red = 0, green = 0, blue = 0;


// Función para mostrar valores en la pantalla OLED
void mostrarEnPantallaOLED() {
    display.clearDisplay();
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 15);
    display.print("PPFD: "); display.print(clear); display.print(" uMol");
    display.setCursor(20, 25);
    display.print("Red: "); display.print(red); display.print(" uMol");
    display.setCursor(20, 35);
    display.print("Green: "); display.print(green); display.print(" uMol");
    display.setCursor(20, 45);
    display.print("Blue: "); display.print(blue); display.print(" uMol");
    display.display();
    Serial.println("Pantalla OLED actualizada con valores.");
}

// Escaneo de dispositivos I2C (opcional para depuración)
void escanearI2C() {
    Serial.println("Escaneando dispositivos I2C...");
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.print("Dispositivo encontrado en dirección: 0x");
            Serial.println(address, HEX);
        }
    }
}

// Manejo de la raíz del servidor (WebApp)
void handleRoot() {
    String html = "<!DOCTYPE html><html><head><title>Mediciones TCS34725</title>";
    html += "<style>";
    html += "body { background-color: #001F3F; color: white; font-family: Arial, sans-serif; display: flex; flex-direction: column; justify-content: space-between; align-items: center; height: 100vh; margin: 0; }";
    html += ".container { display: flex; flex-direction: column; justify-content: center; align-items: center; width: 100%; text-align: center; flex-grow: 1; }";
    html += ".logo { font-size: 7em; font-weight: bold; color: #39FF14; background-color: black; padding: 20px; border-radius: 50%; box-shadow: 0 0 50px rgba(0, 255, 0, 0.7); position: relative; animation: float 3s infinite ease-in-out; text-align: center; font-family: 'Courier New', Courier, monospace; margin-top: 120px; margin-bottom: 30px; }";
    html += "@keyframes float { 0%, 100% { transform: translateY(-10px); } 50% { transform: translateY(10px); } }";
    html += ".box { background-color: #0074D9; padding: 25px 40px; border-radius: 10px; margin-bottom: 20px; font-size: 2.5em; font-weight: bold; display: inline-block; text-align: center; }";
    html += ".bar-container { display: flex; justify-content: space-between; align-items: flex-end; height: 500px; border: 3px solid white; border-radius: 15px; padding: 20px; margin: 20px 0; width: 90%; background-color: #001F4F; position: relative; }";
    html += ".bar { width: 30%; border-radius: 10px; position: relative; transition: height 0.3s; }";
    html += ".red-bar { background-color: red; }";
    html += ".green-bar { background-color: green; }";
    html += ".blue-bar { background-color: blue; }";
    html += ".percent { position: absolute; bottom: 10px; width: 100%; text-align: center; font-size: 1.8em; font-weight: bold; color: white; }";
    html += ".btn { background-color: #FF851B; border: none; color: white; padding: 30px 70px; text-align: center; text-decoration: none; font-size: 2em; margin: 20px; cursor: pointer; border-radius: 25px; }";
    html += ".input { padding: 20px; font-size: 2em; border-radius: 10px; border: 2px solid #0074D9; margin: 15px 0; text-align: center; width: 150px; }";
    html += ".result { font-size: 2.5em; font-weight: bold; color: #39FF14; margin: 15px 0; text-align: center; }";
    html += ".footer { font-size: 1.8em; color: white; margin: 20px 0; text-align: center; }";
    html += ".spacer { margin-top: 40px; }"; // Separador agregado
    html += "</style></head><body>";
    html += "<div class='logo'>Data<br>Druida</div>";
    html += "<div class='container'>";
    html += "  <div class='box'>PPFD: <span id='ppfd'>0</span></div>";
    html += "  <div class='bar-container'>";
    html += "    <div id='redBar' class='bar red-bar' style='height: 0%;'><div id='redPercent' class='percent'>0%</div></div>";
    html += "    <div id='greenBar' class='bar green-bar' style='height: 0%;'><div id='greenPercent' class='percent'>0%</div></div>";
    html += "    <div id='blueBar' class='bar blue-bar' style='height: 0%;'><div id='bluePercent' class='percent'>0%</div></div>";
    html += "  </div>";
    html += "  <button class='btn' onclick='medir()'>MEDIR</button>";
    html += "  <div class='spacer'></div>"; // Separación entre MEDIR y el cálculo de DLI
    html += "  <button class='btn' onclick='calcularDLI()'>DLI</button>";
    html += "  <input id='hoursInput' class='input' type='number' placeholder='Horas de luz' />";
    html += "  <div id='dliResult' class='result'>DLI: -</div>";
    html += "</div>";
    html += "<div class='footer'>druidadata@gmail.com<br>IG: @datadruida</div>";
    html += "<script>";
    html += "function medir() {";
    html += "  fetch('/medir').then(response => response.text()).then(data => {";
    html += "    const lines = data.split('\\n');";
    html += "    const ppfd = parseFloat(lines[0].split(': ')[1]) || 0;";
    html += "    const red = parseFloat(lines[1].split(': ')[1]) || 0;";
    html += "    const green = parseFloat(lines[2].split(': ')[1]) || 0;";
    html += "    const blue = parseFloat(lines[3].split(': ')[1]) || 0;";
    html += "    document.getElementById('ppfd').innerText = ppfd.toFixed(2);";
    html += "    document.getElementById('redBar').style.height = `${red}%`;";
    html += "    document.getElementById('greenBar').style.height = `${green}%`;";
    html += "    document.getElementById('blueBar').style.height = `${blue}%`;";
    html += "    document.getElementById('redPercent').innerText = `${red.toFixed(2)}%`;";
    html += "    document.getElementById('greenPercent').innerText = `${green.toFixed(2)}%`;";
    html += "    document.getElementById('bluePercent').innerText = `${blue.toFixed(2)}%`;";
    html += "  }).catch(error => console.error('Error:', error));";
    html += "}";
    html += "function calcularDLI() {";
    html += "  const ppfd = parseInt(document.getElementById('ppfd').innerText || 0);";
    html += "  const hours = parseFloat(document.getElementById('hoursInput').value || 0);";
    html += "  if (ppfd > 0 && hours > 0) {";
    html += "    const dli = (ppfd * hours * (3600 / 1000000)).toFixed(2);"; // Fórmula DLI
    html += "    document.getElementById('dliResult').innerText = `DLI: ${dli}`;";
    html += "  } else {";
    html += "    document.getElementById('dliResult').innerText = 'DLI: -';";
    html += "  }";
    html += "}";
    html += "</script>";
    html += "</body></html>";

    server.send(200, "text/html", html);
}



// Manejo de la ruta /medir
void handleMedir() {
    // Obtener valores del sensor
    tcs.getRawData(&red, &green, &blue, &clear);

    float coefA0 = -29.9406;
    float coefA1 = 0.1769;
    float coefA2 = -6.0306e-6;
    float coefA3 = 3.6102e-10;

    // Calcular PPFD utilizando los coeficientes calibrados
    //float ppfdActual = coefA0 + coefA1 * clear + coefA2 * (clear * clear);
    //float ppfdActual = 165.2 + 0.1139 * clear + 0.00001318 * (clear * clear);
    float ppfdActual = coefA0 + coefA1 * clear + coefA2 * (clear * clear) + coefA3 * (clear * clear * clear);
    float redPercentage, greenPercentage, bluePercentage;

    if (ppfdActual < 0){
      ppfdActual = 0;
    }

    // Calcular los porcentajes RGB
    if (ppfdActual != 0){
    uint32_t totalRGB = red + green + blue;
    redPercentage = (totalRGB > 0) ? (float(red) / totalRGB) * 100 : 0;
    greenPercentage = (totalRGB > 0) ? (float(green) / totalRGB) * 100 : 0;
    bluePercentage = (totalRGB > 0) ? (float(blue) / totalRGB) * 100 : 0;
    } else {
    redPercentage = 0;
    greenPercentage = 0;
    bluePercentage = 0;  
    }

    // Mensajes de depuración
    Serial.printf("Clear: %d, Red: %d, Green: %d, Blue: %d\n", clear, red, green, blue);
    Serial.printf("PPFD calculado: %.2f uMol\n", ppfdActual);
    Serial.printf("Red: %.2f%%, Green: %.2f%%, Blue: %.2f%%\n", redPercentage, greenPercentage, bluePercentage);

    // Crear mensaje de respuesta para la WebApp
    String mensaje = "PPFD: " + String(ppfdActual, 1) + "\nRed: " + String(redPercentage, 1) +
                     "%\nGreen: " + String(greenPercentage, 1) + "%\nBlue: " + String(bluePercentage, 1) + "%";
    server.send(200, "text/plain", mensaje);

    // Mostrar los valores en la pantalla OLED
    display.clearDisplay();
    //display.setRotation(2); // Invertir la orientación de la pantalla
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(16, 12);
    display.print("PPFD: "); display.print(ppfdActual, 1); display.print(" uMol");

    display.setCursor(31, 24);
    display.print("Red: "); display.print(redPercentage, 1); display.print("%");

    display.setCursor(25, 34);
    display.print("Green: "); display.print(greenPercentage, 1); display.print("%");

    display.setCursor(28, 44);
    display.print("Blue: "); display.print(bluePercentage, 1); display.print("%");

    display.display();

}

void guardarValoresCalibracion(float a0, float a1, float a2) {
    EEPROM.put(0, a0); // Guarda el coeficiente a0
    EEPROM.put(4, a1); // Guarda el coeficiente a1
    EEPROM.put(8, a2); // Guarda el coeficiente a2
    EEPROM.commit();   // Escribe los datos en la memoria física
    Serial.println("Coeficientes guardados en EEPROM:");
    Serial.printf("a0=%.4f, a1=%.4f, a2=%.4f\n", a0, a1, a2);
}

void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED: {
            Serial.println("¡Un cliente se ha conectado!");
            // Actualizar mensaje en la pantalla OLED
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.clearDisplay();
            display.setCursor(31, 28);
            display.print("192.168.4.1");

            //display.print("Dispositivos: ");
            //display.setCursor(70, 30);
            //display.print(WiFi.softAPgetStationNum()); // Número de dispositivos conectados
            display.display();
            delay(2000);
            //display.clearDisplay();
            //display.setTextSize(1);
            //display.setTextColor(SSD1306_WHITE);
            //display.setCursor(0, 10);
            //display.print("Esperando medicion..");
            break;
        }
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: {
            Serial.println("¡Un cliente se ha desconectado!");
            // Actualizar mensaje en la pantalla OLED
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(28, 28);
            display.print("Desconectado");
            display.display();
            break;
        }
        default:
            break;
    }
}




// Configuración inicial del ESP32
void setup() {
    Serial.begin(115200);
    Wire.begin();
    EEPROM.begin(EEPROM_SIZE); // Inicializa EEPROM
    //cargarValoresCalibracion();
    Wire.setClock(100000);  // Reducir la frecuencia del bus I2C a 100 kHz para mayor estabilidad



    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println(F("Error al inicializar la pantalla OLED"));
        while (true);
    }
    delay(1000);
    Serial.println("Pantalla OLED inicializada correctamente.");
     // Mostrar el mensaje de bienvenida al final

        if (!tcs.begin()) {
        Serial.println("No se encontró el sensor TCS34725 ... Verifica la conexión.");
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(30, 15);
        display.print("Error");
        display.setCursor(30, 30);
        display.print("Sensor");
        display.display();
        while (true);
    }
    Serial.println("Mostrando mensaje de bienvenida...");
    display.clearDisplay();
    
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(30, 15);
    display.print("Druida");
    display.setCursor(30, 30);
    display.print("Vision");
    display.display();
    delay(500);  // Mostrar por 3 segundos
    Serial.println("Mensaje de bienvenida mostrado.");

    WiFi.onEvent(WiFiEvent); // Registrar eventos WiFi
    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    IPAddress IP = WiFi.softAPIP();
    Serial.print("IP del punto de acceso: ");
    Serial.println(IP);

    server.on("/", handleRoot);
    server.on("/medir", handleMedir);

    server.begin();
    Serial.println("Servidor iniciado.");

   
}


// Bucle principal
void loop() {
    server.handleClient();
    yield();  // Ceder tiempo a otras tareas del microcontrolador
}
