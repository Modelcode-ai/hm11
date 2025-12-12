// Simple unit tests for RangedInteger and FixedString
#include <gtest/gtest.h>

#include "hm11/util/type_traits.hpp"

using namespace hm11::util;

TEST(RangedIntegerTest, ConstructionAndConversion) {
    Percent p{50};
    EXPECT_EQ(static_cast<std::uint8_t>(p), 50U);
    std::uint8_t raw = p;
    EXPECT_EQ(raw, 50U);
}

TEST(RangedIntegerTest, Arithmetic) {
    Percent a{30};
    Percent b{20};
    Percent c = a + b;
    EXPECT_EQ(c, 50);
    Percent d = c - Percent{10};
    EXPECT_EQ(d, 40);
}

TEST(FixedStringTest, ValidConstruction) {
    MACAddress mac{"0123ABcdEF56"};
    EXPECT_EQ(std::string(mac.view()), "0123ABcdEF56");
}
