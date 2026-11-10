#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define PIN 7
#define MATRIX_WIDTH   32
#define MATRIX_HEIGHT 24
#define NUM_PIXELS    (MATRIX_WIDTH * MATRIX_HEIGHT)

#define M_SMILEY          0
#define M_ALIEN           1
#define M_HEART           2
#define M_RAINBOW         3
#define M_TEXT            4
#define M_DRAW            5
#define M_SOLID           6
#define M_OFF             7
#define MODE_COUNT        8
#define AUTO_CYCLE_COUNT  5

#define EL_FACE   0
#define EL_EYE    1
#define EL_MOUTH  2
#define EL_ALIEN  3
#define EL_HEART  4
#define EL_BG     5
#define EL_TEXT   6
#define EL_COUNT  7

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

class CustomMatrix : public Adafruit_GFX {
  public:
    CustomMatrix() : Adafruit_GFX(MATRIX_WIDTH, MATRIX_HEIGHT) {}

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {

      if ((x < 0) || (x >= MATRIX_WIDTH) || (y < 0) || (y >= MATRIX_HEIGHT)) return;

      if (y >= 0 && y <= 7)        y = y + 16;
      else if (y >= 16 && y <= 23) y = y - 16;

      x = (MATRIX_WIDTH - 1) - x;
      y = (MATRIX_HEIGHT - 1) - y;

      int blockX = x / 8;
      int blockY = y / 8;
      int localX = x % 8;
      int localY = y % 8;

      int visualBlockX = 3 - blockX;
      int visualBlockY = 2 - blockY;

      int blockIndex = (visualBlockX * 3) + visualBlockY;
      int localPixelIndex = (localY * 8) + localX;
      int realPixelIndex = (blockIndex * 64) + localPixelIndex;

      uint8_t r = ((color >> 11) & 0x1F) << 3;
      uint8_t g = ((color >> 5) & 0x3F) << 2;
      uint8_t b = (color & 0x1F) << 3;

      strip.setPixelColor(realPixelIndex, strip.Color(r, g, b));
    }
};

CustomMatrix matrix;

int currentMode = M_SMILEY;
int brightness  = 120;
int animSpeed   = 40;
uint32_t frameCounter = 0;

bool autoCycle = true;
unsigned long lastModeChangeTime = 0;
const unsigned long modeDuration = 8000;
unsigned long lastFrameTime = 0;

uint8_t elemRGB[EL_COUNT][3] = {
  { 255, 255,   0 },
  {   0, 255, 255 },
  { 255, 255, 255 },
  { 255, 153,   0 },
  { 255,   0,   0 },
  {   0,   0,   0 },
  {   0, 255, 255 }
};

#define TEXT_MAX_LEN 40
String scrollMsg = "HELLO";
int textX = MATRIX_WIDTH;

uint16_t canvasBuf[MATRIX_WIDTH][MATRIX_HEIGHT];
bool drawDirty = true;

#define KELVIN_PRESET_COUNT 5
const uint8_t kelvinPresetR[KELVIN_PRESET_COUNT] = { 255, 255, 255, 255, 255 };
const uint8_t kelvinPresetG[KELVIN_PRESET_COUNT] = { 169, 180, 209, 228, 249 };
const uint8_t kelvinPresetB[KELVIN_PRESET_COUNT] = {  87, 107, 163, 206, 253 };

uint16_t mk565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t elemCol(uint8_t e) {
  return mk565(elemRGB[e][0], elemRGB[e][1], elemRGB[e][2]);
}

uint16_t scaleColor(uint16_t color, byte intensity) {
  uint8_t r = ((color >> 11) & 0x1F) * intensity / 255;
  uint8_t g = ((color >> 5) & 0x3F) * intensity / 255;
  uint8_t b = (color & 0x1F) * intensity / 255;
  return (r << 11) | (g << 5) | b;
}

uint16_t wheel(byte pos) {
  byte r, g, b;
  if (pos < 85)       { r = 255 - pos * 3; g = 0;           b = pos * 3; }
  else if (pos < 170) { pos -= 85; r = 0;   g = pos * 3;     b = 255 - pos * 3; }
  else                { pos -= 170; r = pos * 3; g = 255 - pos * 3; b = 0; }
  return mk565(r, g, b);
}

String asciiTR(String in) {
  String out = "";
  for (uint16_t i = 0; i < in.length(); i++) {
    uint8_t c = (uint8_t)in[i];
    if (c < 0x80) { out += (char)c; continue; }
    uint8_t d = (i + 1 < in.length()) ? (uint8_t)in[i + 1] : 0;
    char r = 0;
    if (c == 0xC3) {
      if (d == 0xA7 || d == 0x87) r = 'c';
      else if (d == 0xB6 || d == 0x96) r = 'o';
      else if (d == 0xBC || d == 0x9C) r = 'u';
    } else if (c == 0xC4) {
      if (d == 0x9F || d == 0x9E) r = 'g';
      else if (d == 0xB1 || d == 0xB0) r = 'i';
    } else if (c == 0xC5) {
      if (d == 0x9F || d == 0x9E) r = 's';
    }
    if (r) { out += r; i++; }
  }
  return out;
}

void sceneSmiley(uint32_t f) {
  int cx = 16, cy = 12, rad = 10;
  byte pulse = 200 + 55 * sin(f * 0.25);
  uint16_t faceCol  = scaleColor(elemCol(EL_FACE), pulse);
  uint16_t eyeCol   = elemCol(EL_EYE);
  uint16_t mouthCol = elemCol(EL_MOUTH);

  matrix.drawCircle(cx, cy, rad, faceCol);
  matrix.drawCircle(cx, cy, rad - 1, faceCol);

  bool blink = (f % 40) > 36;
  if (blink) {
    matrix.drawLine(cx - 5, cy - 2, cx - 3, cy - 2, eyeCol);
    matrix.drawLine(cx + 3, cy - 2, cx + 5, cy - 2, eyeCol);
  } else {
    matrix.fillRect(cx - 5, cy - 3, 2, 2, eyeCol);
    matrix.fillRect(cx + 3, cy - 3, 2, 2, eyeCol);
  }

  matrix.drawPixel(cx - 4, cy + 2, mouthCol);
  matrix.drawPixel(cx - 3, cy + 3, mouthCol);
  matrix.drawPixel(cx - 2, cy + 4, mouthCol);
  matrix.drawPixel(cx - 1, cy + 4, mouthCol);
  matrix.drawPixel(cx,     cy + 4, mouthCol);
  matrix.drawPixel(cx + 1, cy + 4, mouthCol);
  matrix.drawPixel(cx + 2, cy + 4, mouthCol);
  matrix.drawPixel(cx + 3, cy + 3, mouthCol);
  matrix.drawPixel(cx + 4, cy + 2, mouthCol);
}

const uint16_t alienA[8] = { 260, 136, 508, 886, 2047, 1533, 1285, 216 };
const uint16_t alienB[8] = { 260, 1161, 1533, 1911, 2047, 1022, 260, 514 };

void drawAlien(int ox, int oy, int frame, int s, uint16_t col) {
  const uint16_t* rows = frame ? alienB : alienA;
  for (int r = 0; r < 8; r++) {
    uint16_t bits = rows[r];
    for (int c = 0; c < 11; c++)
      if (bits & (1 << (10 - c)))
        matrix.fillRect(ox + c * s, oy + r * s, s, s, col);
  }
}

void sceneAlien(uint32_t f) {
  int ox = 4 + (int)(2 * sin(f * 0.10));
  int oy = 4 + (int)(1.5 * sin(f * 0.20));
  int frame = (f / 8) % 2;
  byte pulse = 210 + 45 * sin(f * 0.25);
  drawAlien(ox, oy, frame, 2, scaleColor(elemCol(EL_ALIEN), pulse));

  if ((f % 40) < 8) {
    int bx = ox + 11;
    matrix.drawLine(bx, oy + 16, bx, MATRIX_HEIGHT - 1, elemCol(EL_EYE));
  }
}

const uint8_t heart8[8] = { 0x66, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00 };

void sceneHeart(uint32_t f) {
  byte pulse = 160 + 95 * sin(f * 0.20);
  uint16_t col = scaleColor(elemCol(EL_HEART), pulse);
  int s = 2, ox = 8, oy = 4;
  for (int r = 0; r < 8; r++)
    for (int c = 0; c < 8; c++)
      if (heart8[r] & (1 << (7 - c)))
        matrix.fillRect(ox + c * s, oy + r * s, s, s, col);
}

void sceneRainbow(uint32_t f) {
  for (int y = 0; y < MATRIX_HEIGHT; y++)
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      byte idx = (byte)(x * 7 + y * 4 + f * 3);
      matrix.drawPixel(x, y, wheel(idx));
    }
}

void sceneText() {
  matrix.setTextWrap(false);
  matrix.setTextColor(elemCol(EL_TEXT));
  matrix.setCursor(textX, 8);
  matrix.print(scrollMsg);
  textX--;
  if (textX < -((int)scrollMsg.length() * 6)) textX = MATRIX_WIDTH;
}

void sceneDraw() {
  for (int y = 0; y < MATRIX_HEIGHT; y++)
    for (int x = 0; x < MATRIX_WIDTH; x++)
      matrix.drawPixel(x, y, canvasBuf[x][y]);
}

void renderFrame() {
  strip.setBrightness(brightness);

  if (currentMode == M_DRAW) {
    if (drawDirty) {
      sceneDraw();
      strip.show();
      drawDirty = false;
    }
    return;
  }

  matrix.fillScreen(currentMode == M_OFF ? 0 : elemCol(EL_BG));

  switch (currentMode) {
    case M_SMILEY:  sceneSmiley(frameCounter); break;
    case M_ALIEN:   sceneAlien(frameCounter);  break;
    case M_HEART:   sceneHeart(frameCounter);  break;
    case M_RAINBOW: sceneRainbow(frameCounter); break;
    case M_TEXT:    sceneText();               break;
    case M_SOLID:   break;
    default:        break;
  }

  strip.show();
  frameCounter++;
}

#define P_HEADER   0xA8
#define P_VER      0x01
#define P_ADDR     0x01
#define P_DEVTYPE  0x0004

#define MT_CMD     0x01
#define MT_DATA    0x02
#define MT_STATUS  0x03

#define CMD_SET_MODE       0x30
#define CMD_SET_BRIGHT     0x31
#define CMD_GET_STATE      0x32
#define CMD_SET_AUTO       0x33
#define CMD_SET_SPEED      0x34
#define CMD_SET_COLOR      0x36
#define CMD_SET_KELVIN     0x37
#define CMD_SET_TEXT       0x39
#define CMD_DRAW_PIXELS    0x3A
#define CMD_CLEAR_CANVAS   0x3B

#define STA_SUCCESS        0x0000
#define STA_UNKNOWN_CMD    0x0001
#define STA_CRC_ERROR      0x0002
#define STA_ADDR_MISMATCH  0x0003
#define STA_INVALID_LEN    0x0004
#define STA_INVALID_DATA   0x0005
#define STA_UNSUP_VERSION  0x0006
#define STA_UNSUP_DEVTYPE  0x0007

#define FRAME_OVERHEAD  11
#define MAX_PAYLOAD     64
#define FRAME_BUF_SIZE  (FRAME_OVERHEAD + MAX_PAYLOAD)

uint16_t crc16(const uint8_t* d, size_t n) {
  uint16_t c = 0xFFFF;
  for (size_t i = 0; i < n; i++) {
    c ^= d[i];
    for (int b = 0; b < 8; b++)
      c = (c & 1) ? (c >> 1) ^ 0xA001 : c >> 1;
  }
  return c;
}

int buildStatusFrame(uint8_t* out, uint16_t status) {
  int i = 0;
  out[i++] = P_HEADER; out[i++] = P_VER; out[i++] = P_ADDR;
  out[i++] = (P_DEVTYPE >> 8) & 0xFF; out[i++] = P_DEVTYPE & 0xFF;
  out[i++] = MT_STATUS;
  out[i++] = (status >> 8) & 0xFF; out[i++] = status & 0xFF;
  uint16_t c = crc16(out, i);
  out[i++] = (c >> 8) & 0xFF; out[i++] = c & 0xFF;
  return i;
}

int buildDataFrame(uint8_t* out, uint8_t msgType, uint8_t cmd,
                   const uint8_t* data, uint16_t len) {
  int i = 0;
  out[i++] = P_HEADER; out[i++] = P_VER; out[i++] = P_ADDR;
  out[i++] = (P_DEVTYPE >> 8) & 0xFF; out[i++] = P_DEVTYPE & 0xFF;
  out[i++] = msgType; out[i++] = cmd;
  out[i++] = (len >> 8) & 0xFF; out[i++] = len & 0xFF;
  for (uint16_t k = 0; k < len; k++) out[i++] = data[k];
  uint16_t c = crc16(out, i);
  out[i++] = (c >> 8) & 0xFF; out[i++] = c & 0xFF;
  return i;
}

void applyMode(int m) {
  currentMode = m;
  autoCycle = false;
  lastModeChangeTime = millis();
  if (m == M_TEXT) textX = MATRIX_WIDTH;
  if (m == M_DRAW) drawDirty = true;
}

int protoHandleFrame(const uint8_t* f, int n, uint8_t* out) {
  if (n < FRAME_OVERHEAD || f[0] != P_HEADER) return buildStatusFrame(out, STA_INVALID_LEN);

  uint16_t rxCrc = ((uint16_t)f[n - 2] << 8) | f[n - 1];
  if (crc16(f, n - 2) != rxCrc) return buildStatusFrame(out, STA_CRC_ERROR);
  if (f[1] != P_VER)            return buildStatusFrame(out, STA_UNSUP_VERSION);
  if (f[2] != P_ADDR)           return buildStatusFrame(out, STA_ADDR_MISMATCH);

  uint16_t devType = ((uint16_t)f[3] << 8) | f[4];
  if (devType != P_DEVTYPE)     return buildStatusFrame(out, STA_UNSUP_DEVTYPE);
  if (f[5] != MT_CMD)           return buildStatusFrame(out, STA_INVALID_DATA);

  uint8_t  cmd = f[6];
  uint16_t len = ((uint16_t)f[7] << 8) | f[8];
  if (n != FRAME_OVERHEAD + len) return buildStatusFrame(out, STA_INVALID_LEN);
  const uint8_t* data = &f[9];

  switch (cmd) {
    case CMD_SET_MODE:
      if (len != 1) return buildStatusFrame(out, STA_INVALID_LEN);
      if (data[0] >= MODE_COUNT) return buildStatusFrame(out, STA_INVALID_DATA);
      applyMode(data[0]);
      return buildStatusFrame(out, STA_SUCCESS);

    case CMD_SET_BRIGHT:
      if (len != 1) return buildStatusFrame(out, STA_INVALID_LEN);
      brightness = data[0];
      drawDirty = true;
      return buildStatusFrame(out, STA_SUCCESS);

    case CMD_GET_STATE: {
      if (len != 0) return buildStatusFrame(out, STA_INVALID_LEN);
      uint8_t d[4 + EL_COUNT * 3];
      int i = 0;
      d[i++] = (uint8_t)currentMode;
      d[i++] = (uint8_t)brightness;
      d[i++] = autoCycle ? 1 : 0;
      d[i++] = (uint8_t)animSpeed;
      for (uint8_t e = 0; e < EL_COUNT; e++) {
        d[i++] = elemRGB[e][0];
        d[i++] = elemRGB[e][1];
        d[i++] = elemRGB[e][2];
      }
      return buildDataFrame(out, MT_DATA, CMD_GET_STATE, d, i);
    }

    case CMD_SET_AUTO:
      if (len != 1) return buildStatusFrame(out, STA_INVALID_LEN);
      if (data[0] == 1) {
        autoCycle = true;
        lastModeChangeTime = millis();
        if (currentMode >= AUTO_CYCLE_COUNT) currentMode = M_SMILEY;
      } else if (data[0] == 0) {
        autoCycle = false;
      } else {
        return buildStatusFrame(out, STA_INVALID_DATA);
      }
      return buildStatusFrame(out, STA_SUCCESS);

    case CMD_SET_SPEED:
      if (len != 1) return buildStatusFrame(out, STA_INVALID_LEN);
      if (data[0] < 10 || data[0] > 120) return buildStatusFrame(out, STA_INVALID_DATA);
      animSpeed = data[0];
      return buildStatusFrame(out, STA_SUCCESS);

    case CMD_SET_COLOR:
      if (len != 4) return buildStatusFrame(out, STA_INVALID_LEN);
      if (data[0] >= EL_COUNT) return buildStatusFrame(out, STA_INVALID_DATA);
      elemRGB[data[0]][0] = data[1];
      elemRGB[data[0]][1] = data[2];
      elemRGB[data[0]][2] = data[3];
      drawDirty = true;
      return buildStatusFrame(out, STA_SUCCESS);

    case CMD_SET_KELVIN:
      if (len != 2) return buildStatusFrame(out, STA_INVALID_LEN);
      if (data[0] >= EL_COUNT || data[1] >= KELVIN_PRESET_COUNT)
        return buildStatusFrame(out, STA_INVALID_DATA);
      elemRGB[data[0]][0] = kelvinPresetR[data[1]];
      elemRGB[data[0]][1] = kelvinPresetG[data[1]];
      elemRGB[data[0]][2] = kelvinPresetB[data[1]];
      drawDirty = true;
      return buildStatusFrame(out, STA_SUCCESS);

    case CMD_SET_TEXT: {
      if (len < 1 || len > TEXT_MAX_LEN) return buildStatusFrame(out, STA_INVALID_LEN);
      String msg = "";
      for (uint16_t k = 0; k < len; k++) msg += (char)data[k];
      msg = asciiTR(msg);
      if (msg.length() == 0) return buildStatusFrame(out, STA_INVALID_DATA);
      scrollMsg = msg;
      applyMode(M_TEXT);
      return buildStatusFrame(out, STA_SUCCESS);
    }

    case CMD_DRAW_PIXELS: {

      if (len < 5 || (len - 3) % 2 != 0) return buildStatusFrame(out, STA_INVALID_LEN);
      uint16_t col = mk565(data[0], data[1], data[2]);
      for (uint16_t k = 3; k + 1 < len; k += 2) {
        uint8_t x = data[k], y = data[k + 1];
        if (x >= MATRIX_WIDTH || y >= MATRIX_HEIGHT)
          return buildStatusFrame(out, STA_INVALID_DATA);
        canvasBuf[x][y] = col;
      }
      applyMode(M_DRAW);
      return buildStatusFrame(out, STA_SUCCESS);
    }

    case CMD_CLEAR_CANVAS:
      if (len != 0) return buildStatusFrame(out, STA_INVALID_LEN);
      for (int x = 0; x < MATRIX_WIDTH; x++)
        for (int y = 0; y < MATRIX_HEIGHT; y++)
          canvasBuf[x][y] = 0;
      applyMode(M_DRAW);
      return buildStatusFrame(out, STA_SUCCESS);

    default:
      return buildStatusFrame(out, STA_UNKNOWN_CMD);
  }
}

#define T_USB   0
#define T_BLE   1
#define T_WIFI  2
#define T_COUNT 3

uint8_t transportFrameBuf[T_COUNT][FRAME_BUF_SIZE];
int transportFrameLen[T_COUNT] = { 0, 0, 0 };
unsigned long transportLastByteTime[T_COUNT] = { 0, 0, 0 };

void feedByte(int t, uint8_t b, void (*writeResponse)(const uint8_t*, int)) {
  transportLastByteTime[t] = millis();

  if (transportFrameLen[t] == 0 && b != P_HEADER) return;
  if (transportFrameLen[t] < FRAME_BUF_SIZE) transportFrameBuf[t][transportFrameLen[t]++] = b;

  if (transportFrameLen[t] >= FRAME_OVERHEAD) {
    uint16_t len = ((uint16_t)transportFrameBuf[t][7] << 8) | transportFrameBuf[t][8];

    if (len <= MAX_PAYLOAD && transportFrameLen[t] == FRAME_OVERHEAD + (int)len) {
      uint8_t out[FRAME_BUF_SIZE];
      int on = protoHandleFrame(transportFrameBuf[t], transportFrameLen[t], out);
      writeResponse(out, on);
      transportFrameLen[t] = 0;
    } else if (len > MAX_PAYLOAD) {

      uint8_t out[16];
      int on = buildStatusFrame(out, STA_INVALID_LEN);
      writeResponse(out, on);
      transportFrameLen[t] = 0;
    }
  }
}

void checkTransportTimeout(int t) {
  if (transportFrameLen[t] > 0 && millis() - transportLastByteTime[t] > 300) {
    transportFrameLen[t] = 0;
  }
}

void handleTransport(Stream& port, int t, void (*writeResponse)(const uint8_t*, int)) {
  while (port.available()) feedByte(t, (uint8_t)port.read(), writeResponse);
  checkTransportTimeout(t);
}

#define AP_SSID      "LEDMatrix-Panel"
#define AP_PASSWORD  ""
#define TCP_PORT     8888

WiFiServer tcpServer(TCP_PORT);
WiFiClient tcpClient;

#define BT_NAME                 "LEDMatrix-BLE"
#define SERVICE_UUID            "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer* bleServer = nullptr;
BLECharacteristic* bleTxChar = nullptr;
volatile bool bleDeviceConnected = false;

#define BLE_RX_QUEUE_SIZE 256
uint8_t bleRxQueue[BLE_RX_QUEUE_SIZE];
volatile int bleRxHead = 0, bleRxTail = 0;

void bleQueuePush(uint8_t b) {
  int next = (bleRxHead + 1) % BLE_RX_QUEUE_SIZE;
  if (next != bleRxTail) { bleRxQueue[bleRxHead] = b; bleRxHead = next; }
}

bool bleQueuePop(uint8_t &b) {
  if (bleRxTail == bleRxHead) return false;
  b = bleRxQueue[bleRxTail];
  bleRxTail = (bleRxTail + 1) % BLE_RX_QUEUE_SIZE;
  return true;
}

class BleRxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* ch) override {
    String v = ch->getValue();
    for (size_t i = 0; i < v.length(); i++) bleQueuePush((uint8_t)v[i]);
  }
};

class BleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* s) override {

    if (s->getConnectedCount() > 1) {
      uint16_t newConnId = s->getConnId();
      Serial.println("BLE: second connection rejected (device busy)");
      s->disconnect(newConnId);
      return;
    }
    bleDeviceConnected = true;
    BLEDevice::stopAdvertising();
    Serial.println("BLE: client connected (single connection mode)");
  }
  void onDisconnect(BLEServer* s) override {
    if (s->getConnectedCount() == 0) {
      bleDeviceConnected = false;
      Serial.println("BLE: client disconnected, advertising restarted");
      BLEDevice::startAdvertising();
    }
  }
};

void writeUSB(const uint8_t* d, int n) { Serial.write(d, n); }

void writeWiFi(const uint8_t* d, int n) {
  if (tcpClient && tcpClient.connected()) tcpClient.write(d, n);
}

void writeBLE(const uint8_t* d, int n) {
  if (bleDeviceConnected && bleTxChar) {
    bleTxChar->setValue((uint8_t*)d, n);
    bleTxChar->notify();
  }
}

void setup() {
  Serial.begin(115200);

  strip.begin();
  strip.setBrightness(brightness);
  strip.show();
  matrix.setTextSize(1);
  matrix.setTextWrap(false);

  lastModeChangeTime = millis();
  lastFrameTime = millis();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, (strlen(AP_PASSWORD) > 0) ? AP_PASSWORD : NULL);
  tcpServer.begin();
  Serial.print("WiFi AP: "); Serial.println(AP_SSID);
  Serial.print("IP: ");      Serial.print(WiFi.softAPIP());
  Serial.print("  Port: ");  Serial.println(TCP_PORT);

  BLEDevice::init(BT_NAME);
  BLEDevice::setMTU(247);
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new BleServerCallbacks());

  BLEService* bleService = bleServer->createService(SERVICE_UUID);

  BLECharacteristic* bleRxChar = bleService->createCharacteristic(
      CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  bleRxChar->setCallbacks(new BleRxCallbacks());

  bleTxChar = bleService->createCharacteristic(
      CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  bleTxChar->addDescriptor(new BLE2902());

  bleService->start();
  BLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
  BLEDevice::getAdvertising()->start();
  Serial.print("BLE name: "); Serial.println(BT_NAME);
}

void loop() {

  handleTransport(Serial, T_USB, writeUSB);

  uint8_t bleByte;
  while (bleQueuePop(bleByte)) feedByte(T_BLE, bleByte, writeBLE);
  checkTransportTimeout(T_BLE);

  if (!tcpClient || !tcpClient.connected()) {
    WiFiClient incoming = tcpServer.available();
    if (incoming) {
      tcpClient = incoming;
      transportFrameLen[T_WIFI] = 0;
    }
  }
  if (tcpClient && tcpClient.connected()) handleTransport(tcpClient, T_WIFI, writeWiFi);

  if (autoCycle && millis() - lastModeChangeTime > modeDuration) {
    currentMode = (currentMode + 1) % AUTO_CYCLE_COUNT;
    if (currentMode == M_TEXT) textX = MATRIX_WIDTH;
    lastModeChangeTime = millis();
  }

  if (millis() - lastFrameTime >= (unsigned long)animSpeed) {
    renderFrame();
    lastFrameTime = millis();
  }
}