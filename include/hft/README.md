# include/hft/

Public headers, namespace `hft::`. Anything the tests, benchmarks, tools,
and server all need to `#include` lives here — this is where the hot-path
code (book, pool allocator, ring buffer) will end up mostly header-only, so
the compiler can inline across translation units.

Empty until Stage 1, which adds `types.hpp` (`Price`, `Qty`, `OrderId`),
`order.hpp`, `order_book.hpp` (the shared interface), and `naive_book.hpp`
(the permanent reference oracle).
