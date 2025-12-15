#include <gtest/gtest.h>

#include <string>

#include "hm11/ATCommandBuilder.hpp"
#include "hm11/util/numeric_formatter.hpp"

using namespace hm11;

TEST(at_command_builder_test, build_set_role) {
    auto cmd = ATCommandBuilder::build(AtCommand::SetRole, {util::NumericFormatter::format(1U)});
    constexpr std::string_view EXPECTED_STR = "AT+ROLE1\r\n";
    EXPECT_EQ(cmd.finalize(), EXPECTED_STR);
}

TEST(at_command_builder_test, build_ibeacon_uuid) {
    auto cmd = ATCommandBuilder::build(AtCommand::SetiBeaconUUID, {util::NumericFormatter::format(0x12345678U)});
    constexpr std::string_view EXPECTED_STR = "AT+IBE305419896\r\n";
    EXPECT_EQ(cmd.finalize(), EXPECTED_STR);
}
