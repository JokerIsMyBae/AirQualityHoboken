#pragma once

// This EUI must be in little-endian format, so least-significant-byte (lsb)
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0x00, 0x00,
// 0x00.
static const u1_t PROGMEM APPEUI[8]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// This should also be in little endian format (lsb), see above.
// Note: You do not need to set this field, if unset it will be generated automatically based on the device macaddr
static u1_t DEVEUI[8]  = {  };

// This key should be in big endian format (msb) (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
// The key shown here is the semtech default key.
static const u1_t PROGMEM APPKEY[16] = { 0x14, 0x07, 0xC3, 0xF9, 0xDA, 0x69, 0x48, 0x77, 0x02, 0x2E, 0xA7, 0xFD, 0x87, 0x87, 0x06, 0x1E };

