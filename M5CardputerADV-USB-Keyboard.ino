/*
Teno 2026 - M5 Cardputer ADV -
Me volví loco buscando un firmware que convierta a la M5 Cardputer ADV en un teclado USB y que funcionen las flechas de dirección, F1 a F12, insert, etc...
Como creo que no existe, lo hice.
Si usted tiene un firmware que haga todo esto y encima sea USB o Bluetooth, dicho código será digno de que yo lo vanaglorie y diga "Esto si que no es chicharron de laucha"
*/

#include "M5Cardputer.h"
#include "USB.h"
#include "USBHIDKeyboard.h"

// Objeto del teclado USB HID
USBHIDKeyboard Keyboard;

// Último reporte enviado (para evitar repetir teclas constantemente)
KeyReport lastReport = {0};

// Posiciones verticales en pantalla
#define Y_TITLE 5
#define Y_HELP  35
#define Y_STATUS 115

// ---------------- INTERFAZ ----------------

// Dibuja toda la UI estática (título y ayuda)
void drawUI() {
    M5Cardputer.Display.clear();

    // Título centrado
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(GREEN);

    const char* title = "Teno USB Keyboard";

    // Cálculo básico del ancho del texto para centrarlo
    int titleWidth = strlen(title) * 12;
    int x = (M5Cardputer.Display.width() - titleWidth) / 2;

    M5Cardputer.Display.setCursor(x, Y_TITLE);
    M5Cardputer.Display.println(title);

    // Bloque de ayuda
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(WHITE);

    M5Cardputer.Display.setCursor(10, Y_HELP);
    M5Cardputer.Display.println("F1-F12 -> FN+1..=");

    M5Cardputer.Display.setCursor(10, Y_HELP + 12);
    M5Cardputer.Display.println("INSERT -> FN+Space");

    M5Cardputer.Display.setCursor(10, Y_HELP + 24);
    M5Cardputer.Display.println("ARROWS -> FN+; , . /");

    M5Cardputer.Display.setCursor(10, Y_HELP + 36);
    M5Cardputer.Display.println("ALT+1 -> CTRL+ALT+DEL");

    M5Cardputer.Display.setCursor(10, Y_HELP + 48);
    M5Cardputer.Display.println("ALT+2 -> CTRL+ESC");
}

// Dibuja el estado de teclas modificadoras (CTRL, SHIFT, ALT y FN)
void drawStatus(bool fn, uint8_t modifiers) {

    M5Cardputer.Display.setTextSize(1);

    // Etiquetas
    const char* labels[4] = {"CTRL", "SHIFT", "ALT", "FN"};

    // Colores según estado
    uint16_t colors[4] = {
        (modifiers & 0x01) ? RED : DARKGREY,    // CTRL
        (modifiers & 0x02) ? GREEN : DARKGREY,  // SHIFT
        (modifiers & 0x04) ? YELLOW : DARKGREY, // ALT
        fn ? BLUE : DARKGREY                    // FN
    };

    // Cálculo para centrar todo el bloque
    int spacing = 45;
    int totalWidth = spacing * 4;
    int startX = (M5Cardputer.Display.width() - totalWidth) / 2;

    for (int i = 0; i < 4; i++) {
        M5Cardputer.Display.setTextColor(colors[i]);
        M5Cardputer.Display.setCursor(startX + i * spacing, Y_STATUS);
        M5Cardputer.Display.print(labels[i]);
    }
}

// ---------------- SETUP ----------------

void setup() {
    auto cfg = M5.config();

    // Inicializa la Cardputer
    M5Cardputer.begin(cfg, true);

    // Rotación horizontal
    M5Cardputer.Display.setRotation(1);

    // Dibuja la interfaz
    drawUI();

    // Inicializa USB y teclado
    USB.begin();
    Keyboard.begin();
}

// ---------------- LOOP PRINCIPAL ----------------

void loop() {

    // Actualiza estado del teclado físico
    M5Cardputer.update();

    auto status = M5Cardputer.Keyboard.keysState();

    // Reporte HID que se va a enviar
    KeyReport report = {0};

    // Índice de teclas (máximo 6 simultáneas)
    uint8_t index = 0;

    // Detecta si ALT está presionado (bitmask estándar HID)
    bool alt_active = status.modifiers & 0x04;

    // Recorre todas las teclas presionadas
    for (auto raw : status.hid_keys) {

        uint8_t key = raw;

        // Si viene con SHIFT embebido, lo separa
        if (key & 0x80) {
            report.modifiers |= 0x02;
            key &= 0x7F;
        }

        // ---------------- COMBINACIONES CON ALT ----------------

        if (alt_active) {

            // ALT + 1 → CTRL + ALT + DEL
            if (key == 0x1E) {
                report.modifiers |= (0x01 | 0x04);
                report.keys[index++] = 0x4C;
                continue;
            }

            // ALT + 2 → CTRL + ESC
            if (key == 0x1F) {
                report.modifiers |= 0x01;
                report.keys[index++] = 0x29;
                continue;
            }
        }

        // ---------------- CAPA FN ----------------

        if (status.fn) {
            switch (key) {

                // Flechas
                case 0x33: key = 0x52; break; // arriba
                case 0x37: key = 0x51; break; // abajo
                case 0x36: key = 0x50; break; // izquierda
                case 0x38: key = 0x4F; break; // derecha

                // F1 a F12
                case 0x1E: key = 0x3A; break;
                case 0x1F: key = 0x3B; break;
                case 0x20: key = 0x3C; break;
                case 0x21: key = 0x3D; break;
                case 0x22: key = 0x3E; break;
                case 0x23: key = 0x3F; break;
                case 0x24: key = 0x40; break;
                case 0x25: key = 0x41; break;
                case 0x26: key = 0x42; break;
                case 0x27: key = 0x43; break;
                case 0x2D: key = 0x44; break;
                case 0x2E: key = 0x45; break;

                // Insert
                case 0x2C: key = 0x49; break;
            }
        }

        // La tecla ` se transforma en ESC
        if (key == 0x35) {
            key = 0x29;
        }

        // Agrega la tecla al reporte
        report.keys[index++] = key;

        // Límite HID
        if (index >= 6) break;
    }

    // Agrega modificadores reales (CTRL, SHIFT, ALT)
    report.modifiers |= status.modifiers;

    // Solo envía si cambió algo (evita repetición infinita)
    if (memcmp(&report, &lastReport, sizeof(KeyReport)) != 0) {
        Keyboard.sendReport(&report);
        lastReport = report;
    }

    // Actualiza barra de estado
    drawStatus(status.fn, report.modifiers);

    delay(10);
}
