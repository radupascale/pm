/* ======== PM Lab 6 - I2C ======== */
#include <util/twi.h>

/* ==== High-level AVR I2C routines ==== */
// constants
#define I2C_TIMEOUT 20  /* increments of 50 microseconds => 1ms */
#define I2C_READ    1 // read mode (LSBit must be 1)
#define I2C_WRITE   0 // write transaction (LSBit must be 0)
// I2C error statuses
#define I2C_ERR_TIMEOUT -1
#define I2C_ERR_START   -2
#define I2C_ERR_ACK     -3

/** Waits for an I2C operation to complete */
uint8_t I2C_waitForInterrupt() {
  uint8_t i = 0;
  while (!(TWCR & (1<<TWINT))) {
    if (i >= I2C_TIMEOUT) return 0;
    _delay_us(50); i++;
  }
  return 1;
}

/** Starts an I2C transaction */
int8_t I2C_start(uint8_t address, uint8_t mode) {
  TWCR = 0;
  // transmit START condition (clear interrupt flag + send start bit + enable TWI module)
  TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
  // wait for completion
  if (!I2C_waitForInterrupt()) /* wait for compeltion */
    return I2C_ERR_TIMEOUT;
  // check if the start condition was successfully transmitted
  uint8_t twst = (TWSR & TW_STATUS_MASK);
  if (twst != TW_START) {
    return I2C_ERR_START;
  }
  // load slave address into data register
  TWDR = (((address) << 1) | (mode));
  // start transmission of address
  TWCR = (1<<TWINT) | (1<<TWEN);
  if (!I2C_waitForInterrupt()) /* wait for compeltion */
    return I2C_ERR_TIMEOUT;
  // check if the device has acknowledged the READ / WRITE mode
  twst = (TWSR & TW_STATUS_MASK);
  if ((twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK))
    return I2C_ERR_ACK;
  return 0;
}

/** Stops an I2C transaction */
void I2C_stop() {
  // clear interrupt flag + send Stop bit + enable TWI module
  TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN) ;
}

/**
 * Sends 1 byte over the I2C line.
 * (Note: I2C_WRITE transaction must be started first!)
 */
int8_t I2C_write(uint8_t data) {
  // load data into register
  TWDR = data;
  // configure write operation (simply clear interrupt flag + enable TWI)
  TWCR = (1<<TWINT) | (1<<TWEN);
  if (!I2C_waitForInterrupt()) /* wait for compeltion */
     return I2C_ERR_TIMEOUT;
  if ((TWSR & TW_STATUS_MASK) != TW_MT_DATA_ACK)
     return I2C_ERR_ACK;
  return 0;
}

/**
 * Reads 1 byte from the I2C line (and ACK or NACK at the end).
 * (Note: I2C_READ transaction must be started first!)
 */
int16_t I2C_read(uint8_t ack_more) {
  // configure read operation; if ACK was requested, also set "EnableAck" flag
  TWCR = (1<<TWINT) | (1<<TWEN) | (ack_more ? (1 << TWEA) : 0);
  if (!I2C_waitForInterrupt()) /* wait for compeltion */
    return I2C_ERR_TIMEOUT;
  return TWDR;
}

/* ==== Arduino board functions ==== */

// board setup function ;)
void setup()
{
  // initialize UART for console printing
  Serial.begin(9600);
  // Initialize I2C to a 100KHz clock
  // TWI Status Register: initialize prescaler to 1
  TWSR = (0b00 << TWPS0);
  // TWI Bitrate Register: set bitrate
  // SCL_Freq = CPU_Freq / (16 + 2*TWBR * TWSR_Prescaler)
  // so: TWBR = (SCL_Freq / CPU_Freq - 16) / (TWSR_Prescaler * 2)
  TWBR = 72; // (16000000/100000 - 16) / (1 * 2)
}

int I2C_Scan()
{
  uint8_t foundAddress = 0;
  int res;
  for (uint8_t addr=1; addr <= 0x7F; addr++) {
    res = I2C_start(addr, I2C_WRITE); // start a dummy write transaction
    I2C_stop();
    // TODO: I2C_Scan: check result
    if (res == 0) {
      Serial.print("Found I2C dev at 0x");
      Serial.println(addr, HEX);
      firstAddress = addr;
    }
  }
  return foundAddress;
}

// TODO: implement high-level register write routine
int I2C_RegisterWrite(uint8_t addr, uint8_t reg_id, uint8_t *data, uint8_t data_len)
{
  int res;
  // TODO: 1. start a transaction
  if (I2C_start(addr, I2C_WRITE) < 0) return -1;
  // TODO: 2. write the address of the register
  I2C_write(reg_id);
  for (int i=0; i<data_len; i++) {
    // TODO: 3. write the register's contents (`data_len` bytes)
    res = I2C_write(data[i]);
    if (res < 0) { I2C_stop(); return res; }
  }
  // finally, stop the transaction
  I2C_stop();
  return 0;
}

// TODO: implement high-level register read routine
int I2C_RegisterRead(uint8_t addr, uint8_t reg_id, uint8_t *data_out, uint8_t data_len)
{
  int res;
  // TODO: 1. start a transaction
  if (I2C_start(addr, I2C_WRITE) < 0) return -1;
  // TODO: 2. write the address of the register
  I2C_write(reg_id);
  // TODO: 3. re-start with a I2C_READ transaction
  if (I2C_start(addr, I2C_READ) < 0) return -1;
  // TODO: 4. read the register's contents (`data_len` bytes)
  for (int i=0; i<data_len; i++) {
    res = I2C_read((i < (data_len - 1))); // do not ACK for the last byte
    // append the result into our data array
    if (res >= 0) data_out[i] = res;
  }
  // finally, stop the transaction
  I2C_stop();
  return 0;
}

/** BMP280 compensation parameters (for temperature only). */
struct BMP280_compensation_t {
  uint16_t dig_T1;
  int16_t dig_T2;
  int16_t dig_T3;
};
static int32_t BMP280_CompensateTemp_32(int32_t raw_value, BMP280_compensation_t *compensation);

// macro for checking each I2C transaction
#define _I2C_TRANSACTION_CHECK(res, _text) { \
  if ((res) < 0) { \
    Serial.print("I2C error: " _text); Serial.println(res); \
    delay(5000); return; } }

BMP280_compensation_t compensation;
uint8_t bme280_sampling_initialized = 0;

void loop()
{
  Serial.println("Scanning...");
  int address = I2C_Scan();
  if (!address) {
    Serial.println("Nothing found :(");
    delay(5000); return;
  }

  
  // TODO: read all BMP280 registers (check datasheet)
  int res;
  // example: read ID register, must equal to 0x58 for BMP280
  uint8_t id_value;
  res = I2C_RegisterRead(address, 0xD0, &id_value, 1);
  _I2C_TRANSACTION_CHECK(res, "reading ID register");
  Serial.print("ID register: 0x");
  Serial.println(id_value, HEX);

  // BMP280: read compensation parameters (just for the temperature, for now)
  if (!compensation.dig_T1) {
    uint8_t raw_compensation[6];
    res = I2C_RegisterRead(address, 0x88, raw_compensation, 6);
    _I2C_TRANSACTION_CHECK(res, "reading compensation registers");
    // convert the raw compensation values to 16-bit integers (Little Endian)
    // note: only T1 is unsigned, the others have signs (see datasheet)
    compensation.dig_T1 = ((uint16_t)raw_compensation[0] | ((uint16_t)raw_compensation[1] << 8));
    compensation.dig_T2 = (int16_t)((uint16_t)raw_compensation[2] | ((uint16_t)raw_compensation[3] << 8));
    compensation.dig_T3 = (int16_t)((uint16_t)raw_compensation[4] | ((uint16_t)raw_compensation[5] << 8));
    Serial.println("Compensation parameters:");
    Serial.println(compensation.dig_T1);
    Serial.println(compensation.dig_T2);
    Serial.println(compensation.dig_T3);
  }

  if (!bme280_sampling_initialized) {
    // first, enable temperature sampling
    // measurement control: oversampling 1 for Temp and Pressure, normal mode
    uint8_t ctrl_meas_data = (0b001 << 5) | (0b001 << 2) | 0b11;
    res = I2C_RegisterWrite(address, 0xF4, &ctrl_meas_data, 1);
    _I2C_TRANSACTION_CHECK(res, "write ctrl_meas reg");
    // config: standby 1s, no filtering
    uint8_t config_data = (0b101 << 5) | (0b00 << 2);
    res = I2C_RegisterWrite(address, 0xF5, &config_data, 1);
    _I2C_TRANSACTION_CHECK(res, "write config reg");
    bme280_sampling_initialized = 1;
  }

  // TODO: read & display temperature data from the sensor
  uint8_t raw_temp[3];
  res = I2C_RegisterRead(address, 0xFA, raw_temp, 3);
  _I2C_TRANSACTION_CHECK(res, "reading temp registers");
  int32_t temp32 = (int32_t)(((uint32_t)raw_temp[0] << 12) | ((uint32_t)raw_temp[1] << 4)
    | (uint32_t)(raw_temp[2] & 0x0F));
  int32_t temperature = BMP280_CompensateTemp_32(temp32, &compensation);
  Serial.print("Temperature: ");
  Serial.println((float)temperature / 100);

  // TODO: BONUS: read raw pressure data and convert it to physical quantity (in Pascals) :P
  // Serial.println("pressure not implemented :(");
  delay(5000);
  Serial.println("--");
}

/** BMP280 compensation routines (from the datasheet). */
int32_t bmp280_t_fine; // required for pressure compensation
int32_t BMP280_CompensateTemp_32(int32_t raw_value, BMP280_compensation_t *compensation)
{
  int32_t var1, var2, T;
  // this is the raw temperature compensation formula from datasheet
  var1 = ((((raw_value>>3) - ((int32_t)compensation->dig_T1 << 1))) * ((int32_t)compensation->dig_T2)) >> 11;
  var2 = (((((raw_value >> 4) - ((int32_t)compensation->dig_T1)) * ((raw_value >> 4) - ((int32_t)compensation->dig_T1))) >> 12) *
    ((int32_t)compensation->dig_T3)) >> 14;
  bmp280_t_fine = var1 + var2;
  T = (bmp280_t_fine * 5 + 128) >> 8;
  return T; // value is in 0.01 degrees Celsius
}

