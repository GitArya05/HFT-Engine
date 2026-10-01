#pragma once

#include <cstdint>

namespace hft {

// Disable compiler padding to ensure exact binary layout for the wire protocol
#pragma pack(push, 1)

struct AddOrderMessage {
    char message_type{'A'};
    uint16_t stock_locate{0};
    uint16_t tracking_number{0};
    uint64_t timestamp_ns{0};
    uint64_t order_ref_number{0};
    char buy_sell_indicator{'B'};
    uint32_t shares{0};
    char stock[8]{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    uint32_t price{0};
};

struct OrderExecutedMessage {
    char message_type{'E'};
    uint16_t stock_locate{0};
    uint16_t tracking_number{0};
    uint64_t timestamp_ns{0};
    uint64_t order_ref_number{0};
    uint32_t executed_shares{0};
    uint64_t match_number{0};
};

struct OrderCancelMessage {
    char message_type{'X'};
    uint16_t stock_locate{0};
    uint16_t tracking_number{0};
    uint64_t timestamp_ns{0};
    uint64_t order_ref_number{0};
    uint32_t canceled_shares{0};
};

struct TradeMessage {
    char message_type{'P'};
    uint16_t stock_locate{0};
    uint16_t tracking_number{0};
    uint64_t timestamp_ns{0};
    uint64_t order_ref_number{0};
    char buy_sell_indicator{'B'};
    uint32_t shares{0};
    char stock[8]{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    uint32_t price{0};
    uint64_t match_number{0};
};

#pragma pack(pop)

union ItchMessage {
    char type;
    AddOrderMessage add_order;
    OrderExecutedMessage order_executed;
    OrderCancelMessage order_cancel;
    TradeMessage trade;

    // Provide explicitly defined constructor and destructor to satisfy C++ union rules
    // when member structs contain default initializers.
    ItchMessage() {}
    ~ItchMessage() {}
};

}  // namespace hft