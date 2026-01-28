#include "smbus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

INT SMBus_Scan(HID_SMBUS_DEVICE device, BYTE *slave_addr_group, BYTE slaveAddressStart, BYTE slaveAddressEnd)
{
    BYTE Slave_addr_start_8bit = ConvertTo8BitAddress(slaveAddressStart);
    BYTE Slave_addr_end_8bit = ConvertTo8BitAddress(slaveAddressEnd);
    BYTE totalNumBytesRead = 0;
    BYTE _buffer;

    HID_SMBUS_STATUS    status;
    DWORD responseTimeout;

    // Get response timeout
    status = HidSmbus_GetTimeouts(device, &responseTimeout);
    // Check status
    if(status != HID_SMBUS_SUCCESS)
    {
        return -1;
    }
    // Set response timeout to short value for scanning
    status = HidSmbus_SetTimeouts(device, 10);
    // Check status
    if(status != HID_SMBUS_SUCCESS)
    {
        return -1;
    }
    int j = Slave_addr_start_8bit;

    for (BYTE i = Slave_addr_start_8bit ; i <= Slave_addr_end_8bit && j<= Slave_addr_end_8bit; i += 2 )
    {
        j += 2;
        status = SMBus_Read(device, &_buffer, i, 1);
        if (status > 0) // slave device responded
        {
            slave_addr_group[totalNumBytesRead] = ConvertTo7BitAddress(i);
            totalNumBytesRead++;
        }
        else if (status == HID_SMBUS_DEVICE_IO_FAILED)
            return -2;
    }

    // Restore response timeout
    status = HidSmbus_SetTimeouts(device, responseTimeout); 
    // Check status
    if(status != HID_SMBUS_SUCCESS)
    {
        return -1;
    }

    // Success
    return totalNumBytesRead;
}
