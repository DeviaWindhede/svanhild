#pragma once

typedef unsigned long long eid_t;
constexpr eid_t INVALID_ENTITY = 0;
constexpr eid_t MAX_ENTITIES = 0b100000000000000;  // (32768) should be adjusted if needed
using Entity = eid_t;

