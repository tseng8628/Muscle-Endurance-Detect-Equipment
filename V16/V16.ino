#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Bounce2.h>

//  MQTT設定
const char* ssid = "";
const char* password = "";
const char* mqttServer = "";  // MQTT伺服器位址
const char* mqttUserName = "cubie";  // 使用者名稱，隨意設定。
const char* mqttPwd = "你的MQTT API Key";  // MQTT密碼
const char* clientID = "B";      // 用戶端ID，隨意設定。
const char* topic = "MPU9250_data";
unsigned long prevMillis = 0;  // 暫存經過時間（毫秒）
const long interval = 100;  // 上傳資料的間隔時間，0.1秒。
String msgStr = "";      // 暫存MQTT訊息字串

// MPU6050 Slave Device Address
const uint8_t MPU6050SlaveAddress = 0x68;

// Select SDA and SCL pins for I2C communication
const uint8_t scl = D1;
const uint8_t sda = D2;

// sensitivity scale factor respective to full scale setting provided in datasheet
const uint16_t AccelScaleFactor = 16384;
const uint16_t GyroScaleFactor = 131;

// MPU6050 few configuration register addresses
const uint8_t MPU6050_REGISTER_SMPLRT_DIV   =  0x19;
const uint8_t MPU6050_REGISTER_USER_CTRL    =  0x6A;
const uint8_t MPU6050_REGISTER_PWR_MGMT_1   =  0x6B;
const uint8_t MPU6050_REGISTER_PWR_MGMT_2   =  0x6C;
const uint8_t MPU6050_REGISTER_CONFIG       =  0x1A;
const uint8_t MPU6050_REGISTER_GYRO_CONFIG  =  0x1B;
const uint8_t MPU6050_REGISTER_ACCEL_CONFIG =  0x1C;
const uint8_t MPU6050_REGISTER_FIFO_EN      =  0x23;
const uint8_t MPU6050_REGISTER_INT_ENABLE   =  0x38;
const uint8_t MPU6050_REGISTER_ACCEL_XOUT_H =  0x3B;
const uint8_t MPU6050_REGISTER_SIGNAL_PATH_RESET  = 0x68;

int16_t AccelX, AccelY, AccelZ, Temperature, GyroX, GyroY, GyroZ;

//  RFID設定
#define RST_PIN      0        // 讀卡機的重置腳位 RST-PIN für RC522 - RFID - SPI - Modul GPIO0
#define SS_PIN       2        // 晶片選擇腳位 SDA-PIN für RC522 - RFID - SPI - Modul GPIO2 
MFRC522 mfrc522(SS_PIN, RST_PIN);  // 建立MFRC522物件
MFRC522::MIFARE_Key key;  // 儲存金鑰

byte sector = 15;   // 指定讀寫的「區段」，可能值:0~15
byte block = 1;     // 指定讀寫的「區塊」，可能值:0~3

// 暫存讀取區塊內容的陣列，MIFARE_Read()方法要求至少要18位元組空間，來存放16位元組。
byte buffer[18];

MFRC522::StatusCode status;

//  變數設定
int state = 0;  //判斷啞鈴位置 變數初始值
int frequency = 0;  //啞鈴舉起放下次數
int temp = 0;  //暫存次數
String _tempID = "";
float Acc = 0;      //合力加速度
int _tempAcc = 0;
String DeviceID = "B";//  設定DeviceID，寫入時記得改動
int pin = 10;//控制下方水銀開關
int pin2 = 16;//控制上方水銀開關
int status1;//下方水銀開關的狀態
int status2;//上方水銀開關的狀態
int s = 0; //二頭肌彎舉位置二頭肌彎舉位置
int a[3] = {0};//存門檻狀態的矩陣
int current_status = 0;//判斷啞鈴當前的狀態
int last_status = 0;//判斷啞鈴當前上一次的狀態
int index_of_a = 0;//矩陣裡的第幾格


//  設定按鈕
#define BUTTON_PIN 15//定義BUTTON PIN為D15
// Instantiate a Bounce object
Bounce debouncer = Bounce();

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqttServer, 1883);
  Wire.begin(sda, scl);
  MPU6050_Init();
  SPI.begin();          // Init SPI bus
  mfrc522.PCD_Init();   // 初始化MFRC522讀卡機模組
  setup_key();
  pinMode(BUTTON_PIN, INPUT_PULLUP); //設定PIN D8為輸入模式
  digitalWrite(BUTTON_PIN, LOW); //設定PIN D8為低電位
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5); // interval in ms

  pinMode(pin, INPUT_PULLUP);//讀取(input)pin角位的電壓
  pinMode(pin2, INPUT_PULLUP);//讀取(input)pin2角位的電壓

}

void loop() {
  double Ax, Ay, Az, T, Gx, Gy, Gz;

  Read_RawValue(MPU6050SlaveAddress, MPU6050_REGISTER_ACCEL_XOUT_H);

  //divide each with their sensitivity scale factor
  Ax = (double)AccelX / AccelScaleFactor;
  Ay = (double)AccelY / AccelScaleFactor;
  Az = (double)AccelZ / AccelScaleFactor;
  Gx = (double)GyroX / GyroScaleFactor;
  Gy = (double)GyroY / GyroScaleFactor;
  Gz = (double)GyroZ / GyroScaleFactor;

  Acc = sqrt(sq(Ax) + sq(Ay) + sq(Az));
  //Serial.println(Acc);
  //Serial.print("Ax: "); Serial.print(Ax);
  //Serial.print(" Ay: "); Serial.print(Ay);
  //Serial.print(" Az: "); Serial.println(Az);

  //Serial.print(" Gx: "); Serial.print(Gx);
  //Serial.print(" Gy: "); Serial.print(Gy);
  //Serial.print(" Gz: "); Serial.print(Gz);

  delay(50);

  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  reset_frequency(DeviceID, _tempID);

  // Update the Bounce instance :
  debouncer.update();

  // Get the updated value :
  int value = debouncer.read();

  // 等待0.7秒
  if (millis() - prevMillis > interval) {
    prevMillis = millis();
    mfrc522.PCD_Init();
    uid_and_changedata(DeviceID);
    /*for (int z = 0; z < 20; z++) {
      publish_to_broker(DeviceID, _tempID, frequency, Acc, state);
    }*/
    publish_to_broker(DeviceID, _tempID, frequency, Acc, state);
    status1 = digitalRead(pin);//讀取電壓狀態
    status2 = digitalRead(pin2);
    current_status = Number_of_judgments_2();//現在啞鈴位置門檻變數的狀態
    if (last_status != current_status) {
      //插入array
      insert_to_array();
      last_status = current_status;
    }
    //判別array狀態順序
    array_frequency();
  }
}

void setup_wifi() {
  delay(10);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(clientID, mqttUserName, mqttPwd)) {
      Serial.println("MQTT connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);  // 等5秒之後再重試
    }
  }
}

void I2C_Write(uint8_t deviceAddress, uint8_t regAddress, uint8_t data) {
  Wire.beginTransmission(deviceAddress);
  Wire.write(regAddress);
  Wire.write(data);
  Wire.endTransmission();
}

// read all 14 register
void Read_RawValue(uint8_t deviceAddress, uint8_t regAddress) {
  Wire.beginTransmission(deviceAddress);
  Wire.write(regAddress);
  Wire.endTransmission();
  Wire.requestFrom(deviceAddress, (uint8_t)14);
  AccelX = (((int16_t)Wire.read() << 8) | Wire.read());
  AccelY = (((int16_t)Wire.read() << 8) | Wire.read());
  AccelZ = (((int16_t)Wire.read() << 8) | Wire.read());
  //Temperature = (((int16_t)Wire.read()<<8) | Wire.read());
  GyroX = (((int16_t)Wire.read() << 8) | Wire.read());
  GyroY = (((int16_t)Wire.read() << 8) | Wire.read());
  GyroZ = (((int16_t)Wire.read() << 8) | Wire.read());
}

//configure MPU6050
void MPU6050_Init() {
  delay(150);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_SMPLRT_DIV, 0x07);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_PWR_MGMT_1, 0x01);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_PWR_MGMT_2, 0x00);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_CONFIG, 0x00);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_GYRO_CONFIG, 0x00);//set +/-250 degree/second full scale
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_ACCEL_CONFIG, 0x00);// set +/- 2g full scale
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_FIFO_EN, 0x00);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_INT_ENABLE, 0x01);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_SIGNAL_PATH_RESET, 0x00);
  I2C_Write(MPU6050SlaveAddress, MPU6050_REGISTER_USER_CTRL, 0x00);
}

//  計算次數
int Number_of_judgments_2() {

  if (status2 == HIGH && status1 == LOW)
  {
    s = 1;
  }
  else if (status2 == LOW && status1 == LOW )
  {
    s = 2;
  }
  else if (status2 == LOW && status1 == HIGH)
  {
    s = 3;
  }
  return s;
}

void setup_key() {
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
}


void insert_to_array() {

  Serial.println("index_of_a : " + String(index_of_a));

  if (index_of_a == 0) {
    a[index_of_a] = current_status;
    index_of_a ++;//index_of_a=index_of_a+1//index_of_a+=1;
  } else if (index_of_a == 1) {
    a[1] = current_status;
    index_of_a = 2;
  }
  else if (index_of_a == 2) {
    a[2] = current_status;
    index_of_a = 3;
  } else if (index_of_a == 3) {
    a[0] = a[1];
    a[1] = a[2];
    a[2] = current_status;
    index_of_a = 3;
  }

  Serial.println("a[]:" + String(a[0]) + " / " + String(a[1]) + " / " + String(a[2]));

}

int array_frequency() {

  if (a[0] == 1 && a[1] == 2 && a[2] == 3) {
    frequency++;
    Serial.println("次數:" + String(frequency));
    a[0] = 3; a[1] = 0; a[2] = 0;
    index_of_a = 1;
    state = 1;
    Serial.print("狀態:" + String(state));
  }
  else if (a[0] == 1 && a[1] == 3 && a[2] == 1) {
    frequency++;
    Serial.println("次數:" + String(frequency));
    a[0] = 1; a[1] = 0; a[2] = 0;
    index_of_a = 1;
    state = 1;
    Serial.print("狀態:" + String(state));
  }
  else if (a[0] == 1 && a[1] == 2 && a[2] == 1) {
    Serial.println("屈舉角度不夠低" );
    a[0] = 1; a[1] = 0; a[2] = 0;
    index_of_a = 1;
    state = 2;
    Serial.print("狀態:" + String(state));
  }
  else if (a[0] == 2 && a[1] == 3 && a[2] == 2) {
    Serial.println("沒有回起始位置" );
    a[0] = 2; a[1] = 0; a[2] = 0;
    index_of_a = 1;
    state = 3;
    Serial.print("狀態:" + String(state));
  }
  else if (a[0] == 3 && a[1] == 2 && a[2] == 3) {
    Serial.println("沒有回起始位置" );
    a[0] = 3; a[1] = 0; a[2] = 0;
    index_of_a = 1;
    state = 3;
    Serial.print("狀態:" + String(state));
  }
  else if (a[0] == 3 && a[1] == 2 && a[2] == 1) {

    state = 5;
    Serial.print("狀態:" + String(state));
  }
  else if (a[0] == 2 && a[1] == 1 && a[2] == 2) {
    state = 5;
    Serial.print("狀態:" + String(state));
  }
  else if (a[0] == 2 && a[1] == 3 && a[2] == 1) {
    state = 5;
    Serial.print("狀態:" + String(state));
  }
  else if (a[0] == 3 && a[1] == 1 && a[2] == 3) {
    frequency++;
    Serial.println("次數:" + String(frequency));
    a[0] = 3; a[1] = 0; a[2] = 0;
    index_of_a = 1;
    state = 1;
    Serial.print("狀態:c" + String(state));
  }
  else if (a[0] == 1 && a[1] == 3 && a[2] == 2) {
    state = 5;
    Serial.print("狀態:" + String(state));
  }
  else if (a[0] == 2 && a[1] == 1 && a[2] == 3) {
    state = 5;
    Serial.print("狀態:" + String(state));
  }
  else if (a[0] == 3 && a[1] == 1 && a[2] == 2) {
    state = 5;
    Serial.print("狀態:" + String(state));
  }
  else {
    state = 5;
    Serial.print("狀態:" + String(state));
  }
  return state;
}

void uid_and_changedata(String DeviceID) {

  // 確認是否有新卡片
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    byte *id = mfrc522.uid.uidByte;   // 取得卡片的UID
    byte idSize = mfrc522.uid.size;   // 取得UID的長度

    Serial.print("PICC type: ");      // 顯示卡片類型
    // 根據卡片回應的SAK值（mfrc522.uid.sak）判斷卡片類型
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    //Serial.print("UID Size: ");       // 顯示卡片的UID長度值
    //Serial.println(idSize);

    String str_Uid = "";
    for (byte i = 0; i < idSize; i++) {  // 逐一顯示UID碼
      str_Uid.concat(String(id[i], HEX));
      //Serial.print(id[i], HEX); Serial.print(" ");
    }
    Serial.println(str_Uid);

    //  讀取資料
    readBlock(sector, block, buffer);      // 區段編號、區塊編號、存放讀取資料的陣列

    Serial.print(F("Read block: "));        // 顯示儲存讀取資料的陣列元素值
    String str_changedata = "";

    //  i正常為16，但僅6位，可改成６，否則後面都是0
    for (byte i = 0 ; i < 6 ; i++) {
      str_changedata.concat(String(buffer[i] - '0'));
    }
    Serial.println(str_changedata);
    Serial.println();

    if (_tempID != str_changedata) {
      frequency = 0;
      Acc = 0;
      state = 0;

      //  if_Read publish
      String payload = "{";
      payload += "\"DeviceID\":";
      payload += "\"" + String(DeviceID) + "\"";
      payload += ",";
      payload += "\"UserID\":";
      payload += str_changedata;
      payload += ",";
      payload += "\"frequency\":";
      payload += frequency;
      payload += ",";
      payload += "\"Acc\":";
      payload += Acc;
      payload += ",";
      payload += "\"State\":";
      payload += state;
      payload += "}";

      byte arrSize = payload.length() + 1;
      char msg[arrSize];
      payload.toCharArray(msg, arrSize);
      client.publish(topic, msg);

    }

    _tempID = str_changedata;

    mfrc522.PICC_HaltA();  // 讓卡片進入停止模式

  }
}

void publish_to_broker(String DeviceID, String _tempID, int frequency, float Acc, int t) {

  String f = String(frequency);

  // 製作JSON格式的payload string
  String payload = "{";
  payload += "\"DeviceID\":";
  payload += "\"" + String(DeviceID) + "\"";
  payload += ",";
  payload += "\"UserID\":";
  payload += _tempID;
  payload += ",";
  payload += "\"frequency\":";
  payload += f;
  payload += ",";
  payload += "\"Acc\":";
  payload += Acc;
  payload += ",";
  payload += "\"State\":";
  payload += state;
  payload += "}";

  // 舉一次才顯示&發布
  if (temp != frequency) {
    temp = frequency;

    /*
      byte arrSize = payload.length() + 1;
      char msg[arrSize];
      payload.toCharArray(msg, arrSize);
      client.publish(topic, msg);
    */

    Serial.print("次數：");
    Serial.println(frequency);

  }
  byte arrSize = payload.length() + 1;
  char msg[arrSize];
  payload.toCharArray(msg, arrSize);
  client.publish(topic, msg);
}

void readBlock(byte _sector, byte _block, byte _blockData[])  {
  if (_sector < 0 || _sector > 15 || _block < 0 || _block > 3) {
    // 顯示「區段或區塊碼錯誤」，然後結束函式。
    Serial.println(F("Wrong sector or block number."));
    return;
  }

  byte blockNum = _sector * 4 + _block;  // 計算區塊的實際編號（0~63）
  byte trailerBlock = _sector * 4 + 3;   // 控制區塊編號

  // 驗證金鑰
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  // 若未通過驗證…
  if (status != MFRC522::STATUS_OK) {
    // 顯示錯誤訊息
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  byte buffersize = 18;
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockNum, _blockData, &buffersize);

  // 若讀取不成功…
  if (status != MFRC522::STATUS_OK) {
    // 顯示錯誤訊息
    Serial.print(F("MIFARE_read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // 顯示「讀取成功！」
  Serial.println(F("Data was read."));
}

void reset_frequency(String DeviceID, String _tempID) {
  //讀取PIN D8是否為HIGH
  if (digitalRead(BUTTON_PIN) == HIGH )
  {
    frequency = 0;
    Acc = 0;
    state = 0;

    String payload = "{";
    payload += "\"DeviceID\":";
    payload += "\"" + String(DeviceID) + "\"";
    payload += ",";
    payload += "\"UserID\":";
    payload += _tempID;
    payload += ",";
    payload += "\"frequency\":";
    payload += frequency;
    payload += ",";
    payload += "\"Acc\":";
    payload += Acc;
    payload += ",";
    payload += "\"State\":";
    payload += state;
    payload += "}";

    byte arrSize = payload.length() + 1;
    char msg[arrSize];
    payload.toCharArray(msg, arrSize);
    client.publish(topic, msg);

    Serial.print("次數：");
    Serial.println(frequency);
  }
}
