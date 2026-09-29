#include <gtest/gtest.h>
#include "hft/order.hpp"

TEST(OrderTest, BasicInitialization) {
    // Phase 2 Order takes: id, price, quantity, is_buy
    hft::Order order(1, 100, 50, true);

    EXPECT_EQ(order.id, 1);
    EXPECT_EQ(order.price, 100);
    EXPECT_EQ(order.quantity, 50);
    EXPECT_TRUE(order.is_buy);

    // Intrusive pointers should initialize to null
    EXPECT_EQ(order.next, nullptr);
    EXPECT_EQ(order.prev, nullptr);
}

TEST(OrderTest, OrderTypes) {
    hft::Order buy_order(1, 100, 50, true);
    hft::Order sell_order(2, 105, 20, false);

    EXPECT_TRUE(buy_order.is_buy);
    EXPECT_FALSE(sell_order.is_buy);
    EXPECT_NE(buy_order.id, sell_order.id);
}