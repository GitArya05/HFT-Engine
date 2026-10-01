#include "hft/types.hpp"
#include <gtest/gtest.h>
#include "hft/order.hpp"

TEST(TypesTest, OrderCreation) {
    hft::Order order{1, 100.0, 50, 0, 0, true, hft::OrderType::LIMIT, nullptr, nullptr};
    EXPECT_EQ(order.id, 1ULL);
    EXPECT_DOUBLE_EQ(order.price, 100.0);
    EXPECT_EQ(order.quantity, 50U);
}

TEST(TypesTest, OrderEquality) {
    hft::Order o1{1, 100.0, 50, 0, 0, true, hft::OrderType::LIMIT, nullptr, nullptr};
    hft::Order o2{2, 100.0, 50, 0, 0, true, hft::OrderType::LIMIT, nullptr, nullptr};
    EXPECT_NE(o1.id, o2.id);
}