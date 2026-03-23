/*Teno 2026 - M5 Cardputer ADV - 
Me volví loco buscando un firmware que convierta a la M5 Cardputer ADV en un teclado USB y que funcionen las flechas de dirección, F1 a F12, insert, etc...
Como creo que no existe, lo hice.
Si usted tiene un firmware que haga todo esto y encima sea USB o Bluetooth, dicho código será digno de que yo lo vanaglorie y diga "Esto si que no es chicharron de laucha"
*/

#include "M5Cardputer.h"
#include "USB.h"
#include "USBHIDKeyboard.h"

// Objeto del teclado USB HID
USBHIDKeyboard Keyboard;

// Último reporte enviado (para evitar enviar repetidos)
KeyReport lastReport = {0};

// Posiciones en pantalla
#define Y_TITLE 5
#define Y_HELP  40
#define Y_STATUS 115

// Dibuja la interfaz fija (título + ayuda)
void drawUI() {
    M5Cardputer.Display.clear();

    // Título
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(GREEN);
    M5Cardputer.Display.setCursor(10, Y_TITLE);
    M5Cardputer.Display.println("Teno USB Keyboard");

    // Ayuda (tamaño grande para que se lea bien)
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(WHITE);

    M5Cardputer.Display.setCursor(10, Y_HELP);
    M5Cardputer.Display.println("F1-F12 : FN+1..=");

    M5Cardputer.Display.setCursor(10, Y_HELP + 25);
    M5Cardputer.Display.println("INS    : FN+SPC");

    M5Cardputer.Display.setCursor(10, Y_HELP + 50);
    M5Cardputer.Display.println("ARROWS : FN+; , . /");
}

// Dibuja el estado de modificadores (CTRL, SHIFT, ALT, FN)
void drawStatus(bool fn, uint8_t modifiers) {

    int x = 10;

    // CTRL (bit 0 del campo modifiers)
    M5Cardputer.Display.setTextColor((modifiers & 0x01) ? RED : DARKGREY);
    M5Cardputer.Display.setCursor(x, Y_STATUS);
    M5Cardputer.Display.print("CTRL ");
    x += 45;

    // SHIFT (bit 1)
    M5Cardputer.Display.setTextColor((modifiers & 0x02) ? GREEN : DARKGREY);
    M5Cardputer.Display.setCursor(x, Y_STATUS);
    M5Cardputer.Display.print("SHIFT ");
    x += 55;

    // ALT (bit 2)
    M5Cardputer.Display.setTextColor((modifiers & 0x04) ? YELLOW : DARKGREY);
    M5Cardputer.Display.setCursor(x, Y_STATUS);
    M5Cardputer.Display.print("ALT ");
    x += 45;

    // FN no es parte del HID, lo mostramos aparte
    M5Cardputer.Display.setTextColor(fn ? BLUE : DARKGREY);
    M5Cardputer.Display.setCursor(x, Y_STATUS);
    M5Cardputer.Display.print("FN");
}

void setup() {
    // Configuración base del equipo
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    // Rotación de pantalla
    M5Cardputer.Display.setRotation(1);

    // Dibujar UI fija una sola vez
    drawUI();

    // Inicializar USB y teclado HID
    USB.begin();
    Keyboard.begin();
}

void loop() {
    // Actualiza estado del hardware (teclado, etc.)
    M5Cardputer.update();

    // Estado actual del teclado físico
    auto status = M5Cardputer.Keyboard.keysState();

    // Reporte HID que vamos a armar
    KeyReport report = {0};
    uint8_t index = 0;

    // Recorremos las teclas presionadas
    for (auto raw : status.hid_keys) {

        uint8_t key = raw;

        // Si la tecla viene con SHIFT embebido (bit 7),
        // lo pasamos a modifiers y limpiamos el bit
        if (key & 0x80) {
            report.modifiers |= 0x02; // LEFT SHIFT
            key &= 0x7F;
        }

        // Capa FN: remapeos especiales
        if (status.fn) {
            switch (key) {

                // Flechas
                case 0x33: key = 0x52; break; // ; -> UP
                case 0x37: key = 0x51; break; // . -> DOWN
                case 0x36: key = 0x50; break; // , -> LEFT
                case 0x38: key = 0x4F; break; // / -> RIGHT

                // F1 a F12 (fila numérica)
                case 0x1E: key = 0x3A; break; // 1 -> F1
                case 0x1F: key = 0x3B; break; // 2 -> F2
                case 0x20: key = 0x3C; break; // 3 -> F3
                case 0x21: key = 0x3D; break; // 4 -> F4
                case 0x22: key = 0x3E; break; // 5 -> F5
                case 0x23: key = 0x3F; break; // 6 -> F6
                case 0x24: key = 0x40; break; // 7 -> F7
                case 0x25: key = 0x41; break; // 8 -> F8
                case 0x26: key = 0x42; break; // 9 -> F9
                case 0x27: key = 0x43; break; // 0 -> F10
                case 0x2D: key = 0x44; break; // - -> F11
                case 0x2E: key = 0x45; break; // = -> F12

                // Insert
                case 0x2C: key = 0x49; break; // SPACE -> INSERT
            }
        }

        // Convertir ` en ESC
        if (key == 0x35) {
            key = 0x29;
        }

        // Agregar tecla al reporte (máximo 6 teclas simultáneas)
        report.keys[index++] = key;
        if (index >= 6) break;
    }

    // Agregar modifiers reales (ctrl, alt, etc.)
    report.modifiers |= status.modifiers;

    // Enviar solo si el contenido cambió (evita repeticiones)
    if (memcmp(&report, &lastReport, sizeof(KeyReport)) != 0) {
        Keyboard.sendReport(&report);
        lastReport = report;
    }

    // Actualizar indicadores en pantalla
    drawStatus(status.fn, report.modifiers);

    // Pequeño delay para no saturar CPU
    delay(10);
}