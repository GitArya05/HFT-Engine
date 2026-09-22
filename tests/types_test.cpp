#include "hft/types.hpp"
#include <gtest/gtest.h>
#include "hft/order.hpp"

using namespace hft;

TEST(TypesTest, OrderInitialization) {
    // order_id, price, quantity, is_buy, timestamp
    Order order(1, 10050, 100, true, 1620000000);

    EXPECT_EQ(order.order_id, 1);
    EXPECT_EQ(order.price, 10050);
    EXPECT_EQ(order.quantity, 100);
    EXPECT_TRUE(order.is_buy);
    EXPECT_EQ(order.timestamp, 1620000000);
}

TEST(TypesTest, OrderSideLogic) {
    Order buy_order(1, 100, 50, true, 0);
    Order sell_order(2, 100, 50, false, 0);

    // Testing the equivalent of 'opposite'
    EXPECT_NE(buy_order.is_buy, sell_order.is_buy);
    EXPECT_FALSE(!buy_order.is_buy);
}