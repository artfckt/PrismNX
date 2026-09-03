// Host-only definitions to compile the upstream Fizeau protocol structs.
// Never included in the Switch build.
#pragma once
#include <cstdint>
#define BIT(n) (1U << (n))
#define NX_CONSTEXPR constexpr
#define R_MODULE(x) ((x) & 0x1ff)
#define MAKERESULT(m, d) ((m) | ((d) << 9))
using Result = std::uint32_t;
struct Service {};
