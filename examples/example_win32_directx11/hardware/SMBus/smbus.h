#ifndef SMBUS_H
#define SMBUS_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "types.h"
#include "CP2112/SLABCP2112.h"

// All address human inputs are 7-bit addresses
// Convert 7-bit address to 8-bit address for CP2112 interface
#define ConvertTo8BitAddress(addr) ((addr) << 1)
// Convert 8-bit address to 7-bit address for human readability
#define ConvertTo7BitAddress(addr) ((addr) >> 1)

INT SMBus_Open(HID_SMBUS_DEVICE* device, char** string);
INT SMBus_Close(HID_SMBUS_DEVICE device);
INT SMBus_Reset(HID_SMBUS_DEVICE device);
INT SMBus_Configure(HID_SMBUS_DEVICE device, DWORD bitRate, BYTE address, BOOL autoReadRespond, WORD writeTimeout, WORD readTimeout, BOOL sclLowTimeout, WORD transferRetries, DWORD responseTimeout);
INT SMBus_WriteRead(HID_SMBUS_DEVICE device, BYTE *buffer, BYTE slaveAddress, WORD numBytesToRead, BYTE targetAddressSize, BYTE *targetAddress);
INT SMBus_Read(HID_SMBUS_DEVICE device, BYTE *buffer, BYTE slaveAddress, WORD numBytesToRead);
INT SMBus_Write(HID_SMBUS_DEVICE device, BYTE *buffer, BYTE slaveAddress, BYTE numBytesToWrite);

//helper function
INT SMBus_Scan(HID_SMBUS_DEVICE device, BYTE *slave_addr_group, BYTE slaveAddressStart, BYTE slaveAddressEnd);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SMBUS_H
