/**
 * @file command_builder_test.cpp
 * @brief Unit tests for CommandBuilder AT command construction utilities
 *
 * Tests verify:
 * - Correct command construction for all append types
 * - Bounds checking and overflow handling
 * - Method chaining functionality
 * - Various command patterns (simple, with parameters, query format)
 * - Constexpr evaluation where applicable
 * - Edge cases (empty commands, maximum length, overflow)
 *
 * @copyright Copyright (c) 2025
 * @license BSD-3-Clause
 */

#include <gtest/gtest.h>

#include <string>

#include "hm11/types/type_aliases.hpp"
#include "hm11/util/command_builder.hpp"

using namespace hm11::util;
using namespace hm11::types;

// =============================================================================
// Basic Construction Tests
// =============================================================================

TEST(CommandBuilderTest, DefaultConstructor) {
    CommandBuilder builder;
    EXPECT_TRUE(builder.empty());
    EXPECT_EQ(builder.size(), 0);
    EXPECT_FALSE(builder.overflow());
    EXPECT_EQ(builder.get(), "");
}

TEST(CommandBuilderTest, AppendCString) {
    CommandBuilder builder;
    builder.append("AT+ROLE");
    EXPECT_EQ(builder.get(), "AT+ROLE");
    EXPECT_EQ(builder.size(), 7);
    EXPECT_FALSE(builder.overflow());
}

TEST(CommandBuilderTest, AppendStringView) {
    CommandBuilder builder;
    std::string_view sv = "AT+NAME";
    builder.append(sv);
    EXPECT_EQ(builder.get(), "AT+NAME");
    EXPECT_EQ(builder.size(), 7);
    EXPECT_FALSE(builder.overflow());
}

TEST(CommandBuilderTest, AppendCharacter) {
    CommandBuilder builder;
    builder.append('A').append('T').append('+');
    EXPECT_EQ(builder.get(), "AT+");
    EXPECT_EQ(builder.size(), 3);
    EXPECT_FALSE(builder.overflow());
}

TEST(CommandBuilderTest, MethodChaining) {
    CommandBuilder builder;
    auto result = builder.append("AT+").append("ROLE").append('0').get();
    EXPECT_EQ(result, "AT+ROLE0");
}

TEST(CommandBuilderTest, ClearAndReuse) {
    CommandBuilder builder;
    builder.append("AT+ROLE0");
    EXPECT_EQ(builder.get(), "AT+ROLE0");

    builder.clear();
    EXPECT_TRUE(builder.empty());
    EXPECT_EQ(builder.size(), 0);
    EXPECT_FALSE(builder.overflow());

    builder.append("AT+NAME");
    EXPECT_EQ(builder.get(), "AT+NAME");
}

TEST(CommandBuilderTest, CStrNullTermination) {
    CommandBuilder builder;
    builder.append("AT+ROLE0");
    const char* cmd = builder.c_str();
    EXPECT_STREQ(cmd, "AT+ROLE0");
    EXPECT_EQ(std::strlen(cmd), 8);
}

// =============================================================================
// Enum Append Tests
// =============================================================================

TEST(CommandBuilderTest, AppendRolePeripheral) {
    CommandBuilder builder;
    builder.append("AT+ROLE").append(Role::Peripheral);
    EXPECT_EQ(builder.get(), "AT+ROLE0");
}

TEST(CommandBuilderTest, AppendRoleCentral) {
    CommandBuilder builder;
    builder.append("AT+ROLE").append(Role::Central);
    EXPECT_EQ(builder.get(), "AT+ROLE1");
}

TEST(CommandBuilderTest, AppendBondMode) {
    CommandBuilder builder;
    builder.append("AT+TYPE").append(BondMode::NotNeedPin);
    EXPECT_EQ(builder.get(), "AT+TYPE0");

    builder.clear();
    builder.append("AT+TYPE").append(BondMode::AuthNotNeedPin);
    EXPECT_EQ(builder.get(), "AT+TYPE1");

    builder.clear();
    builder.append("AT+TYPE").append(BondMode::AuthAndBonded);
    EXPECT_EQ(builder.get(), "AT+TYPE3");
}

TEST(CommandBuilderTest, AppendAdvertisingInterval) {
    CommandBuilder builder;
    builder.append("AT+ADVI").append(AdvertisingInterval::Ms100);
    EXPECT_EQ(builder.get(), "AT+ADVI0");

    builder.clear();
    builder.append("AT+ADVI").append(AdvertisingInterval::Ms2000);
    EXPECT_EQ(builder.get(), "AT+ADVIA");
}

// NOTE: AdvertisingType, UartBaudRate tests skipped as enum_to_string
// mappings are not yet implemented.

TEST(CommandBuilderTest, AppendEnumsWithStrings) {
    // Test that enum append can be used with string literals as workaround
    CommandBuilder builder;
    builder.append("AT+ADTY").append("0");
    EXPECT_EQ(builder.get(), "AT+ADTY0");

    builder.clear();
    builder.append("AT+BAUD").append("8");
    EXPECT_EQ(builder.get(), "AT+BAUD8");
}

// =============================================================================
// FixedString Append Tests (Strong Types)
// =============================================================================

TEST(CommandBuilderTest, AppendMacAddress) {
    MacAddress mac{"B4994C123456"};
    CommandBuilder builder;
    builder.append("AT+CON").append(mac);
    EXPECT_EQ(builder.get(), "AT+CONB4994C123456");
}

TEST(CommandBuilderTest, AppendUuid) {
    UUID uuid{"180F"};
    CommandBuilder builder;
    builder.append("AT+UUID0x").append(uuid);
    EXPECT_EQ(builder.get(), "AT+UUID0x180F");
}

TEST(CommandBuilderTest, AppendHandle) {
    HandleType handle{"002A"};
    CommandBuilder builder;
    builder.append("AT+NOTIFY_ON").append(handle);
    EXPECT_EQ(builder.get(), "AT+NOTIFY_ON002A");
}

TEST(CommandBuilderTest, AppendIBeaconUuid) {
    IBeaconUuid uuid{"E2C56DB5DFFB48D2B060D0F5A71096E0"};
    CommandBuilder builder;
    builder.append("AT+IBE00x").append(uuid);
    EXPECT_EQ(builder.get(), "AT+IBE00xE2C56DB5DFFB48D2B060D0F5A71096E0");
}

TEST(CommandBuilderTest, AppendVersionType) {
    VersionType major{"0001"};
    VersionType minor{"FFFF"};
    CommandBuilder builder;
    builder.append("AT+MARJ0x").append(major);
    EXPECT_EQ(builder.get(), "AT+MARJ0x0001");

    builder.clear();
    builder.append("AT+MINO0x").append(minor);
    EXPECT_EQ(builder.get(), "AT+MINO0xFFFF");
}

TEST(CommandBuilderTest, AppendPinType) {
    PinType pin{"123456"};
    CommandBuilder builder;
    builder.append("AT+PASS").append(pin);
    EXPECT_EQ(builder.get(), "AT+PASS123456");
}

TEST(CommandBuilderTest, AppendWorkInterval) {
    WorkInterval interval{"30"};
    CommandBuilder builder;
    builder.append("AT+CYC").append(interval);
    EXPECT_EQ(builder.get(), "AT+CYC30");
}

// =============================================================================
// RangedInteger Append Tests
// =============================================================================

TEST(CommandBuilderTest, AppendRangedIntegerSingleDigit) {
    MacWhiteListIndex idx{1};
    CommandBuilder builder;
    builder.append("AT+AD").append(idx);
    EXPECT_EQ(builder.get(), "AT+AD1");
}

TEST(CommandBuilderTest, AppendRangedIntegerDoubleDigit) {
    Percent battery{75};
    CommandBuilder builder;
    builder.append("Battery: ").append(battery).append('%');
    EXPECT_EQ(builder.get(), "Battery: 75%");
}

TEST(CommandBuilderTest, AppendRangedIntegerZero) {
    LayerConnectionLatency latency{0};
    CommandBuilder builder;
    builder.append("AT+COLA").append(latency);
    EXPECT_EQ(builder.get(), "AT+COLA0");
}

TEST(CommandBuilderTest, AppendRangedIntegerMaxValue) {
    TemperatureType temp{120};
    CommandBuilder builder;
    builder.append("Temp: ").append(temp);
    EXPECT_EQ(builder.get(), "Temp: 120");
}

TEST(CommandBuilderTest, AppendDiscoveryTime) {
    DiscoveryTime time{5};
    CommandBuilder builder;
    builder.append("AT+DISI").append(time);
    EXPECT_EQ(builder.get(), "AT+DISI5");
}

// =============================================================================
// Complex Command Pattern Tests
// =============================================================================

TEST(CommandBuilderTest, SimpleCommand) {
    CommandBuilder builder;
    builder.append("AT");
    EXPECT_EQ(builder.get(), "AT");
}

TEST(CommandBuilderTest, SimpleCommandWithResponse) {
    CommandBuilder builder;
    builder.append("AT+RESET");
    EXPECT_EQ(builder.get(), "AT+RESET");
}

TEST(CommandBuilderTest, QueryCommand) {
    CommandBuilder builder;
    builder.append("AT+ROLE?");
    EXPECT_EQ(builder.get(), "AT+ROLE?");
}

TEST(CommandBuilderTest, QueryCommandBuilt) {
    CommandBuilder builder;
    builder.append("AT+ROLE").append('?');
    EXPECT_EQ(builder.get(), "AT+ROLE?");
}

TEST(CommandBuilderTest, CommandWithEnumParameter) {
    CommandBuilder builder;
    builder.append("AT+ROLE").append(Role::Central);
    EXPECT_EQ(builder.get(), "AT+ROLE1");
}

TEST(CommandBuilderTest, CommandWithMacParameter) {
    MacAddress mac{"001122334455"};
    CommandBuilder builder;
    builder.append("AT+CON").append(mac);
    EXPECT_EQ(builder.get(), "AT+CON001122334455");
}

TEST(CommandBuilderTest, CommandWithMultipleParameters) {
    CommandBuilder builder;
    HandleType from{"0001"};
    HandleType to{"FFFF"};
    builder.append("AT+CHAR").append(from).append(to).append('?');
    EXPECT_EQ(builder.get(), "AT+CHAR0001FFFF?");
}

TEST(CommandBuilderTest, ComplexIBeaconCommand) {
    IBeaconUuid uuid{"12345678901234567890123456789012"};
    CommandBuilder builder;
    builder.append("AT+IBE00x").append(uuid);
    EXPECT_EQ(builder.get(), "AT+IBE00x12345678901234567890123456789012");
}

TEST(CommandBuilderTest, SetNameCommand) {
    // Module name can be up to 12 characters
    CommandBuilder builder;
    builder.append("AT+NAME").append("HM11Device");
    EXPECT_EQ(builder.get(), "AT+NAMEHM11Device");
}

TEST(CommandBuilderTest, GattNotifyCommand) {
    HandleType handle{"0010"};
    CommandBuilder builder;
    builder.append("AT+NOTIFY_ON").append(handle);
    EXPECT_EQ(builder.get(), "AT+NOTIFY_ON0010");
}

TEST(CommandBuilderTest, GattReadCommand) {
    HandleType handle{"002A"};
    CommandBuilder builder;
    builder.append("AT+READDATA").append(handle);
    EXPECT_EQ(builder.get(), "AT+READDATA002A");
}

// =============================================================================
// Edge Cases and Error Handling
// =============================================================================

TEST(CommandBuilderTest, EmptyCommand) {
    CommandBuilder builder;
    EXPECT_EQ(builder.get(), "");
    EXPECT_TRUE(builder.empty());
    EXPECT_EQ(builder.size(), 0);
}

TEST(CommandBuilderTest, MaximumLengthCommand) {
    // Build a command close to the maximum size
    CommandBuilder builder;
    const char* prefix = "AT+COMMAND_WITH_LONG_PREFIX";
    builder.append(prefix);

    // Append a long parameter (iBeacon UUID is 32 chars)
    IBeaconUuid uuid{"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"};
    builder.append(uuid);

    // Should not overflow with reasonable command
    EXPECT_FALSE(builder.overflow());
    // PREFIX is 27 chars + 32 chars UUID = 59 total
    EXPECT_EQ(builder.size(), 59);
}

TEST(CommandBuilderTest, OverflowDetection) {
    // Overflow tests are skipped in debug builds due to assertions
    // In release builds, overflow is handled gracefully via the overflow flag
#ifdef NDEBUG
    CommandBuilder builder;

    // Fill buffer to near capacity
    const std::string LONG_PREFIX(MAX_COMMAND_SIZE - 10, 'A');
    builder.append(LONG_PREFIX);
    EXPECT_FALSE(builder.overflow());

    // This should trigger overflow
    builder.append("THIS_WILL_OVERFLOW");
    EXPECT_TRUE(builder.overflow());
#else
    GTEST_SKIP() << "Overflow detection asserts in debug builds";
#endif
}

TEST(CommandBuilderTest, OverflowPreventsSubsequentAppends) {
    // Overflow tests are skipped in debug builds due to assertions
#ifdef NDEBUG
    CommandBuilder builder;

    // Trigger overflow
    const std::string TOO_LONG(MAX_COMMAND_SIZE + 10, 'X');
    builder.append(TOO_LONG);
    EXPECT_TRUE(builder.overflow());

    const std::size_t SIZE_AFTER_OVERFLOW = builder.size();

    // Subsequent appends should be ignored
    builder.append("MORE");
    EXPECT_EQ(builder.size(), SIZE_AFTER_OVERFLOW);
    EXPECT_TRUE(builder.overflow());
#else
    GTEST_SKIP() << "Overflow handling asserts in debug builds";
#endif
}

TEST(CommandBuilderTest, ClearResetsOverflowFlag) {
    // Overflow tests are skipped in debug builds due to assertions
#ifdef NDEBUG
    CommandBuilder builder;

    // Trigger overflow
    const std::string TOO_LONG(MAX_COMMAND_SIZE + 10, 'X');
    builder.append(TOO_LONG);
    EXPECT_TRUE(builder.overflow());

    // Clear should reset overflow
    builder.clear();
    EXPECT_FALSE(builder.overflow());
    EXPECT_TRUE(builder.empty());

    // Should be able to append again
    builder.append("AT+ROLE0");
    EXPECT_EQ(builder.get(), "AT+ROLE0");
    EXPECT_FALSE(builder.overflow());
#else
    GTEST_SKIP() << "Overflow handling asserts in debug builds";
#endif
}

TEST(CommandBuilderTest, SingleCharacterAppends) {
    CommandBuilder builder;
    const char* str = "AT+ROLE1";
    for (std::size_t i = 0; i < std::strlen(str); ++i) {
        builder.append(str[i]);
    }
    EXPECT_EQ(builder.get(), "AT+ROLE1");
}

// =============================================================================
// Realistic Driver Usage Scenarios
// =============================================================================

TEST(CommandBuilderTest, ScenarioSetRole) {
    // Set_Role procedure from hm11.adb:187-195
    CommandBuilder builder;
    Role role = Role::Central;
    builder.append("AT+ROLE").append(role);
    EXPECT_EQ(builder.get(), "AT+ROLE1");
}

TEST(CommandBuilderTest, ScenarioConnect) {
    // Connect procedure with MAC address
    MacAddress target{"B4994C123456"};
    CommandBuilder builder;
    builder.append("AT+CON").append(target);
    EXPECT_EQ(builder.get(), "AT+CONB4994C123456");
}

TEST(CommandBuilderTest, ScenarioDiscoveryQuery) {
    // Discovery query command from hm11.adb:978
    CommandBuilder builder;
    builder.append("AT+DISC?");
    EXPECT_EQ(builder.get(), "AT+DISC?");
}

TEST(CommandBuilderTest, ScenarioGattCharacteristicQuery) {
    // GATT characteristic query from hm11.adb:1301
    HandleType from{"0001"};
    HandleType to{"FFFF"};
    CommandBuilder builder;
    builder.append("AT+CHAR").append(from).append(to).append('?');
    EXPECT_EQ(builder.get(), "AT+CHAR0001FFFF?");
}

TEST(CommandBuilderTest, ScenarioIBeaconConfig) {
    // iBeacon UUID configuration
    IBeaconUuid uuid{"E2C56DB5DFFB48D2B060D0F5A71096E0"};
    CommandBuilder builder;
    builder.append("AT+IBE00x").append(uuid);
    EXPECT_EQ(builder.get(), "AT+IBE00xE2C56DB5DFFB48D2B060D0F5A71096E0");
    EXPECT_FALSE(builder.overflow());
}

TEST(CommandBuilderTest, ScenarioIBeaconMajorMinor) {
    // iBeacon major/minor configuration
    VersionType major{"0001"};
    VersionType minor{"BEEF"};

    CommandBuilder builder1;
    builder1.append("AT+MARJ0x").append(major);
    EXPECT_EQ(builder1.get(), "AT+MARJ0x0001");

    CommandBuilder builder2;
    builder2.append("AT+MINO0x").append(minor);
    EXPECT_EQ(builder2.get(), "AT+MINO0xBEEF");
}

TEST(CommandBuilderTest, ScenarioSetAdvertisingInterval) {
    // Set advertising interval from hm11.adb:410
    CommandBuilder builder;
    builder.append("AT+ADVI").append(AdvertisingInterval::Ms1022);
    EXPECT_EQ(builder.get(), "AT+ADVI8");
}

TEST(CommandBuilderTest, ScenarioBaudRateConfig) {
    // Set baud rate from hm11.adb:564
    // Using string literal as UartBaudRate enum_to_string not yet implemented
    CommandBuilder builder;
    builder.append("AT+BAUD").append("8");
    EXPECT_EQ(builder.get(), "AT+BAUD8");
}

TEST(CommandBuilderTest, ScenarioWhiteListAdd) {
    // Add MAC to white list with index
    MacWhiteListIndex idx{2};
    MacAddress mac{"001122334455"};
    CommandBuilder builder;
    builder.append("AT+AD").append(idx).append(mac);
    EXPECT_EQ(builder.get(), "AT+AD2001122334455");
}

TEST(CommandBuilderTest, ScenarioSensorWorkInterval) {
    // Set sensor work interval
    WorkInterval interval{"45"};
    CommandBuilder builder;
    builder.append("AT+CYC").append(interval);
    EXPECT_EQ(builder.get(), "AT+CYC45");
}

// =============================================================================
// Constexpr Evaluation Tests
// =============================================================================

TEST(CommandBuilderTest, ConstexprConstruction) {
    // Verify that CommandBuilder can be used in constexpr context
    constexpr CommandBuilder BUILDER;
    static_assert(BUILDER.empty());
    // Note: Cannot use size() == 0 check due to clang-tidy rule
    EXPECT_EQ(BUILDER.size(), 0);
}

TEST(CommandBuilderTest, ConstexprAppend) {
    // Verify that simple append operations are constexpr
    constexpr auto MAKE_COMMAND = []() {
        CommandBuilder builder;
        builder.append("AT");
        return builder;
    };

    constexpr auto BUILDER = MAKE_COMMAND();
    static_assert(BUILDER.size() == 2);
    static_assert(!BUILDER.empty());
}

TEST(CommandBuilderTest, NoBufferOverrunOnOverflow) {
    // Overflow tests are skipped in debug builds due to assertions
#ifdef NDEBUG
    // Use canary values to detect buffer overruns
    constexpr std::uint8_t CANARY = 0xAA;

    // Create a buffer with canary values surrounding the CommandBuilder
    alignas(CommandBuilder) std::array<std::uint8_t, sizeof(CommandBuilder) + 16> buffer{};
    buffer.fill(CANARY);

    // NOLINTBEGIN(cppcoreguidelines-owning-memory, modernize-use-auto)
    // Placement new for canary test
    CommandBuilder* builder = new (&buffer[8]) CommandBuilder();
    // NOLINTEND(cppcoreguidelines-owning-memory, modernize-use-auto)

    // Verify canaries are intact before test
    for (std::size_t i = 0; i < 8; ++i) {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        // Loop bounds are checked
        ASSERT_EQ(buffer[i], CANARY) << "Pre-test canary corrupted at offset " << i;
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    }
    for (std::size_t i = 8 + sizeof(CommandBuilder); i < buffer.size(); ++i) {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        // Loop bounds are checked
        ASSERT_EQ(buffer[i], CANARY) << "Pre-test canary corrupted at offset " << i;
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    // Try to overflow the buffer
    const std::string TOO_LONG(MAX_COMMAND_SIZE + 10, 'X');
    builder->append(TOO_LONG);
    EXPECT_TRUE(builder->overflow());

    // Verify canaries are still intact after overflow attempt
    for (std::size_t i = 0; i < 8; ++i) {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        // Loop bounds are checked
        EXPECT_EQ(buffer[i], CANARY) << "Buffer overrun detected at offset " << i;
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    }
    for (std::size_t i = 8 + sizeof(CommandBuilder); i < buffer.size(); ++i) {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        // Loop bounds are checked
        EXPECT_EQ(buffer[i], CANARY) << "Buffer overrun detected at offset " << i;
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    // Cleanup - manually call destructor since we used placement new
    builder->~CommandBuilder();
#else
    GTEST_SKIP() << "Overflow handling asserts in debug builds";
#endif
}

// Test main is provided by test_main.cpp
