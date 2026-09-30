#pragma once

#include <cstdint>
#include <cstring>

// Hardware Endianness Intrinsics
#ifdef _WIN32
#include <stdlib.h>
#else
#include <byteswap.h>
#endif

namespace hft {

// Re-declare OrderCommand so the parser can output it
struct OrderCommand {
    uint64_t id;
    uint32_t price;
    uint32_t qty;
    bool is_buy;
};

// NASDAQ OUCH 5.0 'Enter Order' Message Definition (Wire Format)
#pragma pack(push, 1)
struct OuchEnterOrder {
    char msg_type;           // 'O'
    char order_token[8];     // Order ID (ASCII or binary)
    char side;               // 'B' (Buy) or 'S' (Sell)
    uint32_t shares;         // Quantity (Big-Endian)
    char stock[8];           // Ticker Symbol
    uint32_t price;          // Price (Big-Endian)
    uint32_t time_in_force;  // (Big-Endian)
    char firm[4];            // MPID
    char display;            // Display instruction
    char capacity;           // Broker capacity
    char intermarket_sweep;  // ISO flag
    uint32_t min_quantity;   // (Big-Endian)
    char cross_type;         // Cross instruction
    char customer_type;      // 'R' or 'N'
};
#pragma pack(pop)

inline uint32_t fast_byteswap32(uint32_t val) {
#ifdef _WIN32
    return _byteswap_ulong(val);
#else
    return __builtin_bswap32(val);
#endif
}

inline uint64_t fast_byteswap64(uint64_t val) {
#ifdef _WIN32
    return _byteswap_uint64(val);
#else
    return __builtin_bswap64(val);
#endif
}

// Zero-copy cast and parse
inline bool parse_ouch_message(const char* raw_buffer, OrderCommand& out_cmd) {
    if (raw_buffer[0] != 'O') [[unlikely]] {
        return false;  // Not an Enter Order message
    }

    // Cast the raw byte buffer directly onto our packed struct
    const OuchEnterOrder* ouch_msg = reinterpret_cast<const OuchEnterOrder*>(raw_buffer);

    // Parse ID (treating 8-byte token as a uint64_t for speed)
    std::memcpy(&out_cmd.id, ouch_msg->order_token, 8);
    out_cmd.id = fast_byteswap64(out_cmd.id);

    // Convert network byte order (Big-Endian) to CPU byte order (Little-Endian)
    out_cmd.qty = fast_byteswap32(ouch_msg->shares);
    out_cmd.price = fast_byteswap32(ouch_msg->price);
    out_cmd.is_buy = (ouch_msg->side == 'B');

    return true;
}

}  // namespace hft