#include "dhmp_hash.h"
#include "murmur3_hash.h"

hash_func hash;
//=============================== public methods ===============================

void dhmp_hash_init() { hash = MurmurHash3_x86_32; }
