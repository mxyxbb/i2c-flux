//==============================================================================
//
// File:
//
//    Ni845x.h
//
// Purpose:
//
//    This file contains function prototypes exported by the driver,
//    error codes, function arguments, and basic type definitions.
//
//==============================================================================
#ifndef __Ni845x_HEADER__
#define __Ni845x_HEADER__


//==============================================================================
//
// Compiler Defines
//
//==============================================================================

#ifdef __cplusplus
   extern "C" {
#endif

#if defined(WIN32)
   #define NI845X_FUNC  __stdcall
#elif defined(WIN64)
   #define NI845X_FUNC __fastcall
#elif defined(macosxU)
   #define NI845X_FUNC
#endif

#if (defined(WIN32) || defined(WIN64))
   #if defined(kExportSymbols)
      #define kNI845XExport
   #else
      #define kNI845XExport
   #endif
#elif defined (macosxU)
   #if defined(kExportSymbols)
      #if ( __GNUC__ >= 4 )
         #define kNI845XExport __attribute__((visibility("default")))
      #else
         #define kNI845XExport __attribute__ ((section ("__TEXT,__export")))
      #endif
   #else
      #define kNI845XExport
   #endif
#endif

#ifdef _CVI_
   #pragma EnableLibraryRuntimeChecking
#endif

//==============================================================================
//
// Type Definitions
//
//==============================================================================

#ifndef _NI_int8_DEFINED_
#define _NI_int8_DEFINED_
   typedef char            int8;
#endif

#ifndef _NI_uInt8_DEFINED_
#define _NI_uInt8_DEFINED_
   typedef unsigned char   uInt8;
#endif

#ifndef _NI_int16_DEFINED_
#define _NI_int16_DEFINED_
   typedef short           int16;
#endif

#ifndef _NI_uInt16_DEFINED_
#define _NI_uInt16_DEFINED_
   typedef unsigned short  uInt16;
#endif

#ifndef _NI_int32_DEFINED_
#define _NI_int32_DEFINED_
   typedef long            int32;
#endif

#ifndef _NI_uInt32_DEFINED_
#define _NI_uInt32_DEFINED_
   typedef unsigned long   uInt32;
#endif

#if defined(WIN64)
   typedef unsigned long long NiHandle;
#elif defined(WIN32)
   typedef unsigned long NiHandle;
#endif

typedef struct {
   uInt32 dimSize;
   uInt8 elt[1];
} Array1DU8_t;
typedef Array1DU8_t **Array1DU8Handle_t;


//==============================================================================
//
// Status Codes
// ... For a description of each error/warning (including solutions),
//     use ni845xStatusToString
//
//==============================================================================

//=====================================
//
// Warnings
//
//=====================================

#define kNi845xWarningClockRateCoerced             301700
#define kNi845xWarningSpiSampleDataIgnored         301701
#define kNi845xWarningUnknown                      301719


//=====================================
//
// Local Errors
//
//=====================================

#define kNi845xErrorNoError                        0
#define kNi845xErrorInsufficientMemory             -301700
#define kNi845xErrorInvalidResourceName            -301701
#define kNi845xErrorInvalidClockRate               -301702
#define kNi845xErrorTooManyScriptReads             -301703
#define kNi845xErrorInvalidScriptReadIndex         -301704
#define kNi845xErrorInvalidScriptReference         -301705
#define kNi845xErrorInvalidDeviceId                -301706
#define kNi845xErrorConnectionLost                 -301707
#define kNi845xErrorTimeout                        -301708
#define kNi845xErrorInternal                       -301709
#define kNi845xErrorInvalidConfigurationReference  -301710
#define kNi845xErrorTooManyConfigurations          -301711
#define kNi845xErrorInvalidActiveProperty          -301712
#define kNi845xErrorInvalidParameter               -301713
#define kNi845xErrorResourceBusy                   -301714
#define kNi845xErrorInvalidMasterCode              -301715
#define kNi845xErrorMasterCodeAck                  -301716
#define kNi845xErrorOverCurrentError               -301718
#define kNi845xErrorSpiStreamingModeNotSupported   -301780
#define kNi845xErrorI2cSlaveModeNotSupported       -301781
#define kNi845xErrorInvalidI2cSlaveEventResponse   -301782
#define kNi845xErrorI2cSlaveEventPending           -301783

#define kNi845xErrorUnknown                        -301719


//=====================================
//
// General Errors from the device
//
//=====================================
#define kNi845xErrorBadOpcode                      -301720
#define kNi845xErrorUnknownStatus                  -301721
#define kNi845xErrorProtocolViolation              -301722
#define kNi845xErrorInvalidScript                  -301723
#define kNi845xErrorInvalidFirmware                -301724
#define kNi845xErrorIncompatibleFirmware           -301725

//=====================================
//
// SPI Errors from the device
//
//=====================================
#define kNi845xErrorMasterWriteCollision           -301730
#define kNi845xErrorInvalidSpiPortNumber           -301732
#define kNi845xErrorInvalidCsPortNumber            -301733
#define kNi845xErrorInvalidChipSelect              -301734
#define kNi845xErrorInvalidBitsPerSample           -301735

//=====================================
//
// I2C Errors from the device
//
//=====================================
#define kNi845xErrorMasterBusFreeTimeout           -301740
#define kNi845xErrorMasterCodeArbLost              -301741
#define kNi845xErrorMasterAddressNotAcknowledged   -301742
#define kNi845xErrorMasterDataNotAcknowledged      -301743
#define kNi845xErrorMasterAddressArbitrationLost   -301744
#define kNi845xErrorMasterDataArbitrationLost      -301745
#define kNi845xErrorInvalidI2CPortNumber           -301746

//=====================================
//
// DIO Errors from the device
//
//=====================================
#define kNi845xErrorInvalidDioPortNumber           -301750
#define kNi845xErrorInvalidDioLineNumber           -301751

//=====================================
//
// SPI Streaming Errors from the device
//
//=====================================
#define kNi845xErrorInStreamingMode                -301717
#define kNi845xErrorNotInStreamingMode             -301760

//=====================================
//
// I2C Slave Errors from the device
//
//=====================================
#define kNi845xErrorInSlaveMode                    -301770
#define kNi845xErrorNotInSlaveMode                 -301771
#define kNi845xErrorInvalidDataBufferSize          -301772
#define kNi845xErrorInvalidSlaveAddress            -301773
#define kNi845xErrorI2cSpecViolation               -301774


//=====================================
//
// SPI Function Arguments
//
//=====================================

#define kNi845xSpiClockPolarityIdleLow             0 // Idle Low
#define kNi845xSpiClockPolarityIdleHigh            1 // Idle High

#define kNi845xSpiClockPhaseFirstEdge              0 // First Edge
#define kNi845xSpiClockPhaseSecondEdge             1 // Second Edge

//=====================================
//
// I2C Function Arguments
//
//=====================================

#define kNi845xI2cAddress7Bit                      0 // 7-Bit Addressing
#define kNi845xI2cAddress10Bit                     1 // 10-Bit Addressing

#define kNi845xI2cNakFalse                         0 // ACK Last Byte
#define kNi845xI2cNakTrue                          1 // NAK Last Byte

#define kNi845xHSDisable                           0 // Disable HS Mode
#define kNi845xHSEnable                            1 // Enable HS Mode

#define kNi845xPullupDisable                       0 // Disable Onboard Pullups
#define kNi845xPullupEnable                        1 // Enable Onboard Pullups

//=====================================
//
// DIO Function Arguments
//
//=====================================

#define kNi845xDioInput                            0 // DIO Direction Input
#define kNi845xDioOutput                           1 // DIO Direction Output

#define kNi845xDioLogicLow                         0 // DIO Level Low
#define kNi845xDioLogicHigh                        1 // DIO Level High

//=====================================
//
// Generic Function Arguments
//
//=====================================

#define kNi845xOpenDrain                           0 // Open Drain
#define kNi845xPushPull                            1 // Push Pull

#define kNi845x33Volts                             33 // 3.3V
#define kNi845x25Volts                             25 // 2.5V
#define kNi845x18Volts                             18 // 1.8V
#define kNi845x15Volts                             15 // 1.5V
#define kNi845x12Volts                             12 // 1.2V

//These defines are deprecated and the 845x Driver Type and IO Voltage Level
//should be used instead
#define kNi845xOpenDrain                           0 // Open-Drain
#define kNi845xPushPull33Volts                     1 // 3.3V Push-Pull


//=====================================
//
// SPI Stream Arguments
//
//=====================================

#define kNi845xSpiStreamClockPolarityIdleLow       0x00 // Idle Low
#define kNi845xSpiStreamClockPolarityIdleHigh      0x01 // Idle High

#define kNi845xSpiStreamClockPhaseFirstEdge        0x00 // First Edge
#define kNi845xSpiStreamClockPhaseSecondEdge       0x01 // Second Edge

#define kNi845xSpiStreamWave1SclkL                 0x00 // Sclk Low Period
#define kNi845xSpiStreamWave1SclkH                 0x01 // Sclk High Period
#define kNi845xSpiStreamWave1T1                    0x02 // T1 (convA -> convD)
#define kNi845xSpiStreamWave1T2                    0x03 // T2 (convD -> csA)
#define kNi845xSpiStreamWave1T3                    0x04 // T3 (convD->sclkA)
#define kNi845xSpiStreamWave1T4                    0x05 // T4 (drdyA->csA)
#define kNi845xSpiStreamWave1T5                    0x06 // T5 (drdyA->sclkA)
#define kNi845xSpiStreamWave1T6                    0x07 // T6 (drdyD->convA)
#define kNi845xSpiStreamWave1T7                    0x08 // T7 (csA->sclkA)
#define kNi845xSpiStreamWave1T8                    0x09 // T8 (csD->convA)
#define kNi845xSpiStreamWave1T9                    0x0A // T9 (csD->csA)
#define kNi845xSpiStreamWave1T10                   0x0B // T10 (sclkD->convA)
#define kNi845xSpiStreamWave1T11                   0x0C // T11 (sclkD->csD)
#define kNi845xSpiStreamWave1T12                   0x0D // T12 (sclkD->sclkA)

//ConvA  =  CONV line assert
//ConvD  =  CONV line deassert
//DrdyA  =  DRDY line assert
//DrdyD  =  DRDY line deassert
//SclkA  =  Assertion of first pulse of SCLK
//SclkD  =  Deassertion of the last pulse of SCLK
//CsA    =  CS line assert
//CsD    =  CS line deassert

#define kNi845xSpiStreamWave1ConvA_ConvD           kNi845xSpiStreamWave1T1
#define kNi845xSpiStreamWave1ConvD_CsA             kNi845xSpiStreamWave1T2
#define kNi845xSpiStreamWave1ConvD_SclkA           kNi845xSpiStreamWave1T3
#define kNi845xSpiStreamWave1DrdyA_CsA             kNi845xSpiStreamWave1T4
#define kNi845xSpiStreamWave1DrdyA_SclkA           kNi845xSpiStreamWave1T5
#define kNi845xSpiStreamWave1DrdyD_ConvA           kNi845xSpiStreamWave1T6
#define kNi845xSpiStreamWave1CsA_SclkA             kNi845xSpiStreamWave1T7
#define kNi845xSpiStreamWave1CsD_ConvA             kNi845xSpiStreamWave1T8
#define kNi845xSpiStreamWave1CsD_CsA               kNi845xSpiStreamWave1T9
#define kNi845xSpiStreamWave1SclkD_ConvA           kNi845xSpiStreamWave1T10
#define kNi845xSpiStreamWave1SclkD_CsD             kNi845xSpiStreamWave1T11
#define kNi845xSpiStreamWave1SclkD_SclkA           kNi845xSpiStreamWave1T12

#define kNi845xSpiStreamWave1ConvPin               0x00 // Convert (CONV)
#define kNi845xSpiStreamWave1DrdyPin               0x01 // Device Ready (DRDY)
#define kNi845xSpiStreamWave1CsPin                 0x02 // Chip Select (CS)

#define kNi845xSpiStreamDisabled                   0x00 // Disabled
#define kNi845xSpiStreamActiveHigh                 0x01 // Active High
#define kNi845xSpiStreamActiveLow                  0x02 // Active Low
#define kNi845xSpiStreamDriveHigh                  0x03 // Drive (constant) High
#define kNi845xSpiStreamDriveLow                   0x04 // Drive (constant) Low


//=====================================
//
// I2C Slave Arguments
//
//=====================================


#define kNi845xI2cSlaveEventNoEvent                0x00 // None
#define kNi845xI2cSlaveEventCmdEvent               0x01 // Command
#define kNi845xI2cSlaveEventDataAvailEvent         0x02 // Data Available
#define kNi845xI2cSlaveEventDataReqEvent           0x03 // Data Requested
#define kNi845xI2cSlaveEventWriteCompleteEvent     0x04 // Write Complete

#define kNi845xI2cEventFlagIoStart                 0x01 // I/O Start
#define kNi845xI2cEventFlagIoComplete              0x02 // I/O Complete
#define kNi845xI2cEventFlagIoCompleteStop          0x04 // I/O Complete Stop



//==============================================================================
//
// DEVICE FUNCTION PROTOTYPES
//
//==============================================================================


kNI845XExport int32 NI845X_FUNC ni845xFindDevice (
   char *     FirstDevice,
   NiHandle * FindDeviceHandle,
   uInt32 *   NumberFound
   );

kNI845XExport int32 NI845X_FUNC ni845xFindDeviceNext (
   NiHandle FindDeviceHandle,
   char *   NextDevice
   );

kNI845XExport int32 NI845X_FUNC ni845xCloseFindDeviceHandle(
   NiHandle FindDeviceHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xOpen(
   char *     ResourceName,
   NiHandle * DeviceHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xClose (
   NiHandle DeviceHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xDeviceLock (
   NiHandle DeviceHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xDeviceUnlock (
   NiHandle DeviceHandle
   );

kNI845XExport void NI845X_FUNC ni845xStatusToString(
   int32  StatusCode,
   uInt32 MaxSize,
   int8 * StatusString
   );

kNI845XExport int32 NI845X_FUNC ni845xSetIoVoltageLevel(
   NiHandle DeviceHandle,
   uInt8    VoltageLevel
   );


kNI845XExport int32 NI845X_FUNC ni845xSetTimeout(
   NiHandle DeviceHandle,
   uInt32   Timeout
   );


//==============================================================================
//
// SPI BASIC API FUNCTION PROTOTYPES
//
//==============================================================================


kNI845XExport int32 NI845X_FUNC ni845xSpiWriteRead(
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle,
   uInt32   WriteSize,
   uInt8  * WriteData,
   uInt32 * ReadSize,
   uInt8  * ReadData
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationOpen (
   NiHandle * ConfigurationHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationClose (
   NiHandle ConfigurationHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationSetChipSelect (
   NiHandle ConfigurationHandle,
   uInt32 ChipSelect
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationSetClockRate (
   NiHandle ConfigurationHandle,
   uInt16 ClockRate
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationSetClockPolarity (
   NiHandle ConfigurationHandle,
   int32  ClockPolarity
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationSetClockPhase (
   NiHandle ConfigurationHandle,
   int32  ClockPhase
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationSetNumBitsPerSample (
   NiHandle ConfigurationHandle,
   uInt16   NumBitsPerSample
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationSetPort (
   NiHandle ConfigurationHandle,
   uInt8  PortNumber
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationGetChipSelect (
   NiHandle ConfigurationHandle,
   uInt32 * ChipSelect
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationGetClockRate (
   NiHandle ConfigurationHandle,
   uInt16 * ClockRate
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationGetClockPolarity (
   NiHandle ConfigurationHandle,
   int32 * ClockPolarity
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationGetClockPhase (
   NiHandle ConfigurationHandle,
   int32 * ClockPhase
   );

int32 NI845X_FUNC ni845xSpiConfigurationGetNumBitsPerSample (
   NiHandle ConfigurationHandle,
   uInt16 * NumBitsPerSample
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiConfigurationGetPort (
   NiHandle ConfigurationHandle,
   uInt8 * PortNumber
   );


//==============================================================================
//
// SPI SCRIPTING API FUNCTION PROTOTYPES
//
//==============================================================================

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptOpen (
   NiHandle * ScriptHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptClose (
   NiHandle ScriptHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptRun (
   NiHandle ScriptHandle,
   NiHandle DeviceHandle,
   uInt8    PortNumber
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptReset (
   NiHandle ScriptHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptClockPolarityPhase (
   NiHandle ScriptHandle,
   int32    ClockPolarity,
   int32    ClockPhase
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptClockRate (
   NiHandle ScriptHandle,
   uInt16   ClockRate
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptNumBitsPerSample (
   NiHandle ScriptHandle,
   uInt16   NumBitsPerSample
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptCSLow (
   NiHandle ScriptHandle,
   uInt32   ChipSelectNum
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptCSHigh (
   NiHandle ScriptHandle,
   uInt32   ChipSelectNum
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptWriteRead (
   NiHandle ScriptHandle,
   uInt32   WriteSize,
   uInt8 *  WriteData,
   uInt32 * ScriptReadIndex
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptDelay (
   NiHandle ScriptHandle,
   uInt8    Delay
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptUsDelay (
   NiHandle ScriptHandle,
   uInt16   Delay
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptEnableSPI (
   NiHandle ScriptHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptDisableSPI (
   NiHandle ScriptHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptDioConfigurePort(
   NiHandle ScriptHandle,
   uInt8    PortNumber,
   uInt8    ConfigurationValue
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptDioWritePort (
   NiHandle ScriptHandle,
   uInt8    PortNumber,
   uInt8    WriteData
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptDioReadPort (
   NiHandle ScriptHandle,
   uInt8    PortNumber,
   uInt32 * ScriptReadIndex
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptDioConfigureLine (
   NiHandle ScriptHandle,
   uInt8    PortNumber,
   uInt8    LineNumber,
   int32    ConfigurationValue
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptDioWriteLine (
   NiHandle ScriptHandle,
   uInt8    PortNumber,
   uInt8    LineNumber,
   int32    WriteData
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptDioReadLine (
   NiHandle ScriptHandle,
   uInt8    PortNumber,
   uInt8    LineNumber,
   uInt32 * ScriptReadIndex
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptExtractReadData (
   NiHandle ScriptHandle,
   uInt32   ScriptReadIndex,
   uInt8 *  ReadData
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiScriptExtractReadDataSize (
   NiHandle ScriptHandle,
   uInt32   ScriptReadIndex,
   uInt32 * ReadDataSize
   );


//==============================================================================
//
// SPI Streaming Exports
//
//==============================================================================


kNI845XExport int32 NI845X_FUNC ni845xSpiStreamStart(
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamRead(
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle,
   uInt32   NumBytesToRead,
   uInt8 *  ReadData,
   uInt32 * ReadSize
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamStop(
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationOpen(
   NiHandle * ConfigurationHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationClose(
   NiHandle ConfigurationHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationSetClockPhase(
   NiHandle ConfigurationHandle,
   uInt8    ClockPhase
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationSetClockPolarity(
   NiHandle ConfigurationHandle,
   uInt8    ClockPolarity
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationSetPacketSize(
   NiHandle ConfigurationHandle,
   uInt32   PacketSize
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationSetNumBits(
   NiHandle ConfigurationHandle,
   uInt8    NumBits
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationSetNumSamples(
   NiHandle ConfigurationHandle,
   uInt32   NumSamples
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationWave1SetMosiData(
   NiHandle ConfigurationHandle,
   uInt8 *  DataArray,
   uInt32   ArraySize
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationWave1SetPinConfig(
   NiHandle ConfigurationHandle,
   uInt8    PinNumber,
   uInt8    Mode
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationWave1SetTimingParam(
   NiHandle ConfigurationHandle,
   uInt8    TimingParameter,
   uInt32   ParameterValue
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationGetClockPhase(
   NiHandle ConfigurationHandle,
   uInt8 *  ClockPhase
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationGetClockPolarity(
   NiHandle ConfigurationHandle,
   uInt8 *  ClockPolarity
   );


kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationGetNumBits(
   NiHandle ConfigurationHandle,
   uInt8 *  NumBits
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationGetNumSamples(
   NiHandle ConfigurationHandle,
   uInt32 * NumSamples
   );


kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationGetPacketSize(
   NiHandle ConfigurationHandle,
   uInt32 * PacketSize
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationWave1GetPinConfig(
   NiHandle ConfigurationHandle,
   uInt8    PinNumber,
   uInt8 *  Mode
   );

kNI845XExport int32 NI845X_FUNC ni845xSpiStreamConfigurationWave1GetTimingParam(
   NiHandle ConfigurationHandle,
   uInt8    TimingParameter,
   uInt32 * ParameterValue
   );


//==============================================================================
//
// I2C BASIC API FUNCTION PROTOTYPES
//
//==============================================================================


kNI845XExport int32 NI845X_FUNC ni845xI2cWrite (
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle,
   uInt32   WriteSize,
   uInt8 *  WriteData
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cRead (
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle,
   uInt32   NumBytesToRead,
   uInt32 * ReadSize,
   uInt8 *  ReadData
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cWriteRead (
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle,
   uInt32   WriteSize,
   uInt8 *  WriteData,
   uInt32   NumBytesToRead,
   uInt32 * ReadSize,
   uInt8 *  ReadData
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSetPullupEnable (
   NiHandle DeviceHandle,
   uInt8    Enable
   );


kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationOpen (
   NiHandle * ConfigurationHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationClose (
   NiHandle ConfigurationHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationSetPort (
   NiHandle ConfigurationHandle,
   uInt8    PortNumber
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationSetAddress (
   NiHandle ConfigurationHandle,
   uInt16   Address
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationSetClockRate (
   NiHandle ConfigurationHandle,
   uInt16   ClockRate
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationSetAddressSize (
   NiHandle ConfigurationHandle,
   int32    Size
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationSetAckPollTimeout (
   NiHandle ConfigurationHandle,
   uInt16   Timeout
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationSetHSEnable (
   NiHandle ConfigurationHandle,
   uInt8    HsEnable
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationSetHSClockRate (
   NiHandle ConfigurationHandle,
   uInt16   HsClockRate
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationSetHSMasterCode (
   NiHandle ConfigurationHandle,
   uInt8    HsMasterCode
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationGetPort (
   NiHandle ConfigurationHandle,
   uInt8 *  Port
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationGetAddress (
   NiHandle ConfigurationHandle,
   uInt16 * Address
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationGetClockRate (
   NiHandle ConfigurationHandle,
   uInt16 * ClockRate
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationGetAddressSize (
   NiHandle ConfigurationHandle,
   int32 *  Size
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationGetAckPollTimeout (
   NiHandle ConfigurationHandle,
   uInt16 * Timeout
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationGetHSEnable (
   NiHandle ConfigurationHandle,
   uInt8 *  HsEnable
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationGetHSClockRate (
   NiHandle ConfigurationHandle,
   uInt16 * HsClockRate
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cConfigurationGetHSMasterCode (
   NiHandle ConfigurationHandle,
   uInt8 *  HsMasterCode
   );


//==============================================================================
//
// I2C SCRIPTING FUNCTION PROTOTYPES
//
//==============================================================================

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptOpen (
   NiHandle * ScriptHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptClose (
   NiHandle ScriptHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptRun (
   NiHandle ScriptHandle,
   NiHandle DeviceHandle,
   uInt8    PortNumber
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptReset (
   NiHandle ScriptHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptIssueStart (
   NiHandle ScriptHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptAddressWrite (
   NiHandle ScriptHandle,
   uInt8    Address
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptAddressRead (
   NiHandle ScriptHandle,
   uInt8    Address
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptIssueStop (
   NiHandle ScriptHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptWrite (
   NiHandle ScriptHandle,
   uInt32   WriteSize,
   uInt8  * WriteData
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptRead (
   NiHandle ScriptHandle,
   uInt32   NumBytesToRead,
   int32    NotAcknowledgeLastByte,
   uInt32 * ScriptReadIndex
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptDelay (
   NiHandle ScriptHandle,
   uInt8    Delay
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptUsDelay (
   NiHandle ScriptHandle,
   uInt16   Delay
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptClockRate (
   NiHandle ScriptHandle,
   uInt16   ClockRate
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptAckPollTimeout (
   NiHandle ScriptHandle,
   uInt16   Timeout
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptDioConfigurePort (
   NiHandle ScriptHandle,
   uInt8    PortNumber,
   uInt8    ConfigurationValue
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptDioWritePort (
   NiHandle ScriptHandle,
   uInt8    PortNumber,
   uInt8    WriteData
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptDioReadPort (
   NiHandle ScriptHandle,
   uInt8    PortNumber,
   uInt32 * ScriptReadIndex
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptDioConfigureLine (
   NiHandle ScriptHandle,
   uInt8    PortNumber,
   uInt8    LineNumber,
   int32    ConfigurationValue
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptDioWriteLine (
   NiHandle ScriptHandle,
   uInt8    PortNumber,
   uInt8    LineNumber,
   int32    WriteData
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptDioReadLine (
   NiHandle ScriptHandle,
   uInt8    PortNumber,
   uInt8    LineNumber,
   uInt32 * ScriptReadIndex
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptHSEnable (
   NiHandle ScriptHandle,
   uInt8    HsEnable
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptHSMasterCode (
   NiHandle ScriptHandle,
   uInt8    HsMasterCode
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptHSClockRate (
   NiHandle ScriptHandle,
   uInt16   HsClockRate
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptPullupEnable (
   NiHandle ScriptHandle,
   uInt8    Enable
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptExtractReadData (
   NiHandle ScriptHandle,
   uInt32   ScriptReadIndex,
   uInt8 *  ReadData
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cScriptExtractReadDataSize (
   NiHandle ScriptHandle,
   uInt32   ScriptReadIndex,
   uInt32 * ReadDataSize
   );


//==============================================================================
//
// I2C SLAVE FUNCTION PROTOTYPES
//
//==============================================================================

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveStart (
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveStop (
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveWaitForEvent (
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle,
   uInt32   Timeout,
   uInt8 *  EventType,
   uInt16 * EventFlags
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveRead (
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle,
   uInt16   NumBytesToRead,
   uInt16 * ReadSize,
   uInt8 *  ReadData,
   uInt8 *  EventCleared
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveWrite (
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle,
   uInt16   WriteSize,
   uInt8 *  WriteData
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveGetWriteInfo(
   NiHandle DeviceHandle,
   NiHandle ConfigurationHandle,
   uInt16 * NumBytesWritten
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveConfigurationOpen (
   NiHandle * ConfigurationHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveConfigurationClose (
   NiHandle ConfigurationHandle
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveConfigurationSetAddress (
   NiHandle ConfigurationHandle,
   uInt16   Address
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveConfigurationSetCommandBufferSize (
   NiHandle ConfigurationHandle,
   uInt16   CommandBufferSize
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveConfigurationSetDataBufferSize (
   NiHandle ConfigurationHandle,
   uInt16   DataBufferSize
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveConfigurationSetAutoRemovalTimeout (
   NiHandle ConfigurationHandle,
   int32   AutoRemovalTimeout
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveConfigurationGetAddress (
   NiHandle ConfigurationHandle,
   uInt16 * Address
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveConfigurationGetCommandBufferSize (
   NiHandle ConfigurationHandle,
   uInt16 * CommandBufferSize
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveConfigurationGetDataBufferSize (
   NiHandle ConfigurationHandle,
   uInt16 * DataBufferSize
   );

kNI845XExport int32 NI845X_FUNC ni845xI2cSlaveConfigurationGetAutoRemovalTimeout (
   NiHandle ConfigurationHandle,
   int32 * AutoRemovalTimeout
   );


//==============================================================================
//
// DIO I/O FUNCTION PROTOTYPES
//
//==============================================================================


kNI845XExport int32 NI845X_FUNC ni845xDioWritePort (
   NiHandle DeviceHandle,
   uInt8    PortNumber,
   uInt8    WriteData
   );

kNI845XExport int32 NI845X_FUNC ni845xDioReadPort (
   NiHandle DeviceHandle,
   uInt8    PortNumber,
   uInt8 *  ReadData
   );

kNI845XExport int32 NI845X_FUNC ni845xDioWriteLine (
   NiHandle DeviceHandle,
   uInt8    PortNumber,
   uInt8    LineNumber,
   int32    WriteData
   );

kNI845XExport int32 NI845X_FUNC ni845xDioReadLine (
   NiHandle DeviceHandle,
   uInt8    PortNumber,
   uInt8    LineNumber,
   int32 *  ReadData
   );

//This function has been deprecated and ni845xDioSetDriverType and
//ni845xSetIoVoltageLevel should be used instead.
kNI845XExport int32 NI845X_FUNC ni845xDioSetPortVoltageType (
   NiHandle DeviceHandle,
   uInt8    PortNumber,
   uInt8    Type
   );

kNI845XExport int32 NI845X_FUNC ni845xDioSetDriverType(
   NiHandle DeviceHandle,
   uInt8    DioPort,
   uInt8    Type
   );

kNI845XExport int32 NI845X_FUNC ni845xDioSetPortLineDirectionMap (
   NiHandle DeviceHandle,
   uInt8    PortNumber,
   uInt8    Map
   );



#ifdef __cplusplus
   }
#endif

#endif // __Ni845x_HEADER__
