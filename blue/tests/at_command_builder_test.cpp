#include <gtest/gtest.h>

#include <string>

#include "hm11/ATCommandBuilder.hpp"

using namespace hm11;

TEST(at_command_builder_test, build_set_role) {
    std::string cmd = ATCommandBuilder::build(AtCommand::SetRole, {1U});
    constexpr std::string_view EXPECTED_STR = "AT+ROLE1\r\n";
    EXPECT_EQ(cmd, EXPECTED_STR);
}

TEST(at_command_builder_test, build_ibeacon_uuid) {
    std::string cmd = ATCommandBuilder::build(AtCommand::SetiBeaconUUID, {0x12345678U});
    constexpr std::string_view EXPECTED_STR = "AT+IBE305419896\r\n";
    EXPECT_EQ(cmd, EXPECTED_STR);
}
