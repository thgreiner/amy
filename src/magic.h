extern uint16_t rook_table_offsets[64];
extern uint64_t rook_table[102400];

extern uint16_t bishop_table_offsets[64];
extern uint64_t bishop_table[5248];

extern const uint64_t rook_blocker_mask[];
extern const uint64_t bishop_blocker_mask[];

extern const uint64_t rook_magics[];
extern const uint64_t bishop_magics[];

extern const uint8_t rook_index_bits[];
extern const uint8_t bishop_index_bits[];

static uint64_t rook_attacks(int sq, uint64_t occupied) {
    uint64_t blockers = occupied & rook_blocker_mask[sq];
    int magic_index =
        (blockers * rook_magics[sq]) >> (64 - rook_index_bits[sq]);
    return rook_table[2 * rook_table_offsets[sq] + magic_index];
}

static uint64_t bishop_attacks(int sq, uint64_t occupied) {
    uint64_t blockers = occupied & bishop_blocker_mask[sq];
    int magic_index =
        (blockers * bishop_magics[sq]) >> (64 - bishop_index_bits[sq]);
    return bishop_table[bishop_table_offsets[sq] + magic_index];
}
