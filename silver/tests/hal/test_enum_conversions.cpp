// Comprehensive Enum Conversion Tests
// Tests all util::to_string() and util::to_char() functions to achieve full coverage

#include <gtest/gtest.h>
#include <hm11/util/string_builder.hpp>
#include <hm11/types.hpp>

using namespace hm11;
using namespace hm11::util;

// ============================================================================
// Role Enum Conversion Tests
// ============================================================================

TEST(EnumConversionTest, Role_Peripheral) {
    EXPECT_EQ(to_string(Role::Peripheral), "0");
}

TEST(EnumConversionTest, Role_Central) {
    EXPECT_EQ(to_string(Role::Central), "1");
}

// ============================================================================
// Boolean Conversion Tests
// ============================================================================

TEST(EnumConversionTest, Bool_True) {
    EXPECT_EQ(to_string(true), "1");
}

TEST(EnumConversionTest, Bool_False) {
    EXPECT_EQ(to_string(false), "0");
}

// ============================================================================
// AdvertisingInterval Conversion Tests (16 values)
// ============================================================================

TEST(EnumConversionTest, AdvertisingInterval_Ms100) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms100), '0');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms211) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms211), '1');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms252) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms252), '2');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms318) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms318), '3');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms417) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms417), '4');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms546) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms546), '5');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms760) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms760), '6');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms852) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms852), '7');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms1022) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms1022), '8');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms1285) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms1285), '9');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms2000) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms2000), 'A');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms3000) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms3000), 'B');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms4000) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms4000), 'C');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms5000) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms5000), 'D');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms6000) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms6000), 'E');
}

TEST(EnumConversionTest, AdvertisingInterval_Ms7000) {
    EXPECT_EQ(to_char(AdvertisingInterval::Ms7000), 'F');
}

// ============================================================================
// AdvertisingType Conversion Tests (4 values)
// ============================================================================

TEST(EnumConversionTest, AdvertisingType_AdvertisingScanResponseConnectable) {
    EXPECT_EQ(to_string(AdvertisingType::AdvertisingScanResponseConnectable), "0");
}

TEST(EnumConversionTest, AdvertisingType_LastDeviceConnect) {
    EXPECT_EQ(to_string(AdvertisingType::LastDeviceConnect), "1");
}

TEST(EnumConversionTest, AdvertisingType_AdvertisingScanResponse) {
    EXPECT_EQ(to_string(AdvertisingType::AdvertisingScanResponse), "2");
}

TEST(EnumConversionTest, AdvertisingType_Advertising) {
    EXPECT_EQ(to_string(AdvertisingType::Advertising), "3");
}

// ============================================================================
// MacAddressType Conversion Tests (4 values)
// ============================================================================

TEST(EnumConversionTest, MacAddressType_NormalAddress) {
    EXPECT_EQ(to_string(MacAddressType::NormalAddress), "0");
}

TEST(EnumConversionTest, MacAddressType_StaticMac) {
    EXPECT_EQ(to_string(MacAddressType::StaticMac), "1");
}

TEST(EnumConversionTest, MacAddressType_StaticRandomMac) {
    EXPECT_EQ(to_string(MacAddressType::StaticRandomMac), "2");
}

TEST(EnumConversionTest, MacAddressType_RandomMac) {
    EXPECT_EQ(to_string(MacAddressType::RandomMac), "3");
}

// ============================================================================
// UartBaudRate Conversion Tests (9 values)
// ============================================================================

TEST(EnumConversionTest, UartBaudRate_Br9600) {
    EXPECT_EQ(to_string(UartBaudRate::Br9600), "0");
}

TEST(EnumConversionTest, UartBaudRate_Br19200) {
    EXPECT_EQ(to_string(UartBaudRate::Br19200), "1");
}

TEST(EnumConversionTest, UartBaudRate_Br38400) {
    EXPECT_EQ(to_string(UartBaudRate::Br38400), "2");
}

TEST(EnumConversionTest, UartBaudRate_Br57600) {
    EXPECT_EQ(to_string(UartBaudRate::Br57600), "3");
}

TEST(EnumConversionTest, UartBaudRate_Br115200) {
    EXPECT_EQ(to_string(UartBaudRate::Br115200), "4");
}

TEST(EnumConversionTest, UartBaudRate_Br4800) {
    EXPECT_EQ(to_string(UartBaudRate::Br4800), "5");
}

TEST(EnumConversionTest, UartBaudRate_Br2400) {
    EXPECT_EQ(to_string(UartBaudRate::Br2400), "6");
}

TEST(EnumConversionTest, UartBaudRate_Br1200) {
    EXPECT_EQ(to_string(UartBaudRate::Br1200), "7");
}

TEST(EnumConversionTest, UartBaudRate_Br230400) {
    EXPECT_EQ(to_string(UartBaudRate::Br230400), "8");
}

// ============================================================================
// SendDataMethod Conversion Tests (4 values)
// ============================================================================

TEST(EnumConversionTest, SendDataMethod_Write) {
    EXPECT_EQ(to_string(SendDataMethod::Write), "WR");
}

TEST(EnumConversionTest, SendDataMethod_WriteWithoutResponse) {
    EXPECT_EQ(to_string(SendDataMethod::WriteWithoutResponse), "WN");
}

TEST(EnumConversionTest, SendDataMethod_Indicate) {
    EXPECT_EQ(to_string(SendDataMethod::Indicate), "IN");
}

TEST(EnumConversionTest, SendDataMethod_Notify) {
    EXPECT_EQ(to_string(SendDataMethod::Notify), "NO");
}

// ============================================================================
// SendDataCharacteristic Conversion Tests (2 values)
// ============================================================================

TEST(EnumConversionTest, SendDataCharacteristic_Write) {
    EXPECT_EQ(to_string(SendDataCharacteristic::Write), "WR");
}

TEST(EnumConversionTest, SendDataCharacteristic_WriteWithoutResponse) {
    EXPECT_EQ(to_string(SendDataCharacteristic::WriteWithoutResponse), "WN");
}

// ============================================================================
// SensorType Conversion Tests (3 values)
// ============================================================================

TEST(EnumConversionTest, SensorType_None) {
    EXPECT_EQ(to_string(SensorType::None), "0");
}

TEST(EnumConversionTest, SensorType_DHT11) {
    EXPECT_EQ(to_string(SensorType::DHT11), "1");
}

TEST(EnumConversionTest, SensorType_DS18B20) {
    EXPECT_EQ(to_string(SensorType::DS18B20), "2");
}

// ============================================================================
// ModulePower Conversion Tests (4 values)
// ============================================================================

TEST(EnumConversionTest, ModulePower_DbmMinus23) {
    EXPECT_EQ(to_string(ModulePower::DbmMinus23), "0");
}

TEST(EnumConversionTest, ModulePower_DbmMinus6) {
    EXPECT_EQ(to_string(ModulePower::DbmMinus6), "1");
}

TEST(EnumConversionTest, ModulePower_Dbm0) {
    EXPECT_EQ(to_string(ModulePower::Dbm0), "2");
}

TEST(EnumConversionTest, ModulePower_Dbm6) {
    EXPECT_EQ(to_string(ModulePower::Dbm6), "3");
}

// ============================================================================
// OutputPower Conversion Tests (2 values)
// ============================================================================

TEST(EnumConversionTest, OutputPower_NormalPower) {
    EXPECT_EQ(to_string(OutputPower::NormalPower), "0");
}

TEST(EnumConversionTest, OutputPower_MaxPower) {
    EXPECT_EQ(to_string(OutputPower::MaxPower), "1");
}

// ============================================================================
// AdvertisingMode Conversion Tests (2 values)
// ============================================================================

TEST(EnumConversionTest, AdvertisingMode_NormalAdvertising) {
    EXPECT_EQ(to_string(AdvertisingMode::NormalAdvertising), "0");
}

TEST(EnumConversionTest, AdvertisingMode_ReliableAdvertising) {
    EXPECT_EQ(to_string(AdvertisingMode::ReliableAdvertising), "1");
}

// ============================================================================
// ParityBit Conversion Tests (3 values)
// ============================================================================

TEST(EnumConversionTest, ParityBit_None) {
    EXPECT_EQ(to_string(ParityBit::None), "0");
}

TEST(EnumConversionTest, ParityBit_Odd) {
    EXPECT_EQ(to_string(ParityBit::Odd), "1");
}

TEST(EnumConversionTest, ParityBit_Even) {
    EXPECT_EQ(to_string(ParityBit::Even), "2");
}

// ============================================================================
// StopBit Conversion Tests (2 values)
// ============================================================================

TEST(EnumConversionTest, StopBit_OneStopBit) {
    EXPECT_EQ(to_string(StopBit::OneStopBit), "0");
}

TEST(EnumConversionTest, StopBit_TwoStopBit) {
    EXPECT_EQ(to_string(StopBit::TwoStopBit), "1");
}

// ============================================================================
// LinkLayerConnectionInterval Conversion Tests (10 values)
// ============================================================================

TEST(EnumConversionTest, LinkLayerConnectionInterval_Ms7) {
    EXPECT_EQ(to_string(LinkLayerConnectionInterval::Ms7), "0");
}

TEST(EnumConversionTest, LinkLayerConnectionInterval_Ms10) {
    EXPECT_EQ(to_string(LinkLayerConnectionInterval::Ms10), "1");
}

TEST(EnumConversionTest, LinkLayerConnectionInterval_Ms15) {
    EXPECT_EQ(to_string(LinkLayerConnectionInterval::Ms15), "2");
}

TEST(EnumConversionTest, LinkLayerConnectionInterval_Ms20) {
    EXPECT_EQ(to_string(LinkLayerConnectionInterval::Ms20), "3");
}

TEST(EnumConversionTest, LinkLayerConnectionInterval_Ms25) {
    EXPECT_EQ(to_string(LinkLayerConnectionInterval::Ms25), "4");
}

TEST(EnumConversionTest, LinkLayerConnectionInterval_Ms30) {
    EXPECT_EQ(to_string(LinkLayerConnectionInterval::Ms30), "5");
}

TEST(EnumConversionTest, LinkLayerConnectionInterval_Ms35) {
    EXPECT_EQ(to_string(LinkLayerConnectionInterval::Ms35), "6");
}

TEST(EnumConversionTest, LinkLayerConnectionInterval_Ms40) {
    EXPECT_EQ(to_string(LinkLayerConnectionInterval::Ms40), "7");
}

TEST(EnumConversionTest, LinkLayerConnectionInterval_Ms45) {
    EXPECT_EQ(to_string(LinkLayerConnectionInterval::Ms45), "8");
}

TEST(EnumConversionTest, LinkLayerConnectionInterval_Ms4000) {
    EXPECT_EQ(to_string(LinkLayerConnectionInterval::Ms4000), "9");
}

// ============================================================================
// ConnectionSupervisionTimeout Conversion Tests (7 values)
// ============================================================================

TEST(EnumConversionTest, ConnectionSupervisionTimeout_Ms100) {
    EXPECT_EQ(to_string(ConnectionSupervisionTimeout::Ms100), "0");
}

TEST(EnumConversionTest, ConnectionSupervisionTimeout_Ms1000) {
    EXPECT_EQ(to_string(ConnectionSupervisionTimeout::Ms1000), "1");
}

TEST(EnumConversionTest, ConnectionSupervisionTimeout_Ms2000) {
    EXPECT_EQ(to_string(ConnectionSupervisionTimeout::Ms2000), "2");
}

TEST(EnumConversionTest, ConnectionSupervisionTimeout_Ms3000) {
    EXPECT_EQ(to_string(ConnectionSupervisionTimeout::Ms3000), "3");
}

TEST(EnumConversionTest, ConnectionSupervisionTimeout_Ms4000) {
    EXPECT_EQ(to_string(ConnectionSupervisionTimeout::Ms4000), "4");
}

TEST(EnumConversionTest, ConnectionSupervisionTimeout_Ms5000) {
    EXPECT_EQ(to_string(ConnectionSupervisionTimeout::Ms5000), "5");
}

TEST(EnumConversionTest, ConnectionSupervisionTimeout_Ms6000) {
    EXPECT_EQ(to_string(ConnectionSupervisionTimeout::Ms6000), "6");
}

// ============================================================================
// PioOutput Conversion Tests (2 values)
// ============================================================================

TEST(EnumConversionTest, PioOutput_Low) {
    EXPECT_EQ(to_string(PioOutput::Low), "0");
}

TEST(EnumConversionTest, PioOutput_High) {
    EXPECT_EQ(to_string(PioOutput::High), "1");
}
