#include <gtest/gtest.h>
#include <vector>
#include "hft/ouch_parser.hpp"

TEST(NetworkParserTest, ParseOuchEnterOrder) {
    // Construct a mock raw network packet
    std::vector<char> raw_packet(sizeof(hft::OuchEnterOrder), 0);
    hft::OuchEnterOrder* msg = reinterpret_cast<hft::OuchEnterOrder*>(raw_packet.data());

    // Populate with Big-Endian Network Data
    msg->msg_type = 'O';
    msg->side = 'B';

    // Simulate ID 1000 in Big-Endian: 0x00 00 00 00 00 00 03 E8
    uint64_t mock_id = 1000;
    mock_id = hft::fast_byteswap64(mock_id);
    std::memcpy(msg->order_token, &mock_id, 8);

    // Simulate Qty 500 in Big-Endian: 0x00 00 01 F4
    msg->shares = hft::fast_byteswap32(500);

    // Simulate Price 15000 in Big-Endian: 0x00 00 3A 98
    msg->price = hft::fast_byteswap32(15000);

    // Execute zero-copy parse
    hft::OrderCommand cmd;
    bool success = hft::parse_ouch_message(raw_packet.data(), cmd);

    // Verify
    EXPECT_TRUE(success);
    EXPECT_EQ(cmd.id, 1000);
    EXPECT_EQ(cmd.qty, 500);
    EXPECT_EQ(cmd.price, 15000);
    EXPECT_TRUE(cmd.is_buy);
}