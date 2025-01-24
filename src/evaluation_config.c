/*

    Amy - a chess playing program

    Copyright (c) 2002-2025, Thorsten Greiner
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdint.h>
#include <stdio.h>

#include "amy.h"
#include "dbase.h"
#include "evaluation.h"
#include "search.h"
#include "utils.h"
#include "yaml.h"

/** The name of the current configuration. */
char *ConfigurationName = "default";

static void configure_name(struct Node *);
static void configure_search(struct Node *);
static void configure_pawn_scores(struct Node *);
static void configure_knight_scores(struct Node *);
static void configure_bishop_scores(struct Node *);
static void configure_rook_scores(struct Node *);
static void configure_queen_scores(struct Node *);
static void configure_king_scores(struct Node *);
static char *read_file(char *);
static void print_piece_square_table(FILE *, int16_t *);
static void print_array(FILE *, char *, int16_t *, size_t);

/**
 * Reads evaluation parameters from a file.
 */
void LoadEvaluationConfig(char *file_name) {
    char *buffer = read_file(file_name);

    if (buffer == NULL)
        return;

    struct Node *node = parse_yaml(buffer);
    free(buffer);

    if (node == NULL) {
        return;
    }

    configure_name(node);
    configure_search(node);
    configure_pawn_scores(node);
    configure_knight_scores(node);
    configure_bishop_scores(node);
    configure_rook_scores(node);
    configure_queen_scores(node);
    configure_king_scores(node);

    free_yaml_node(node);
}

/**
 * Writes evaluation config to a file.
 */
void SaveEvaluationConfig(char *file_name) {
    FILE *fout = fopen(file_name, "w");

    if (!fout) {
        perror("Cannot open file");
        return;
    }

    fprintf(fout, "name: %s\n\n", ConfigurationName);

    fprintf(fout, "search:\n");
    fprintf(fout, "  extend_in_check: %d\n", ExtendInCheck);
    fprintf(fout, "  extend_double_check: %d\n", ExtendDoubleCheck);
    fprintf(fout, "  extend_discovered_check: %d\n", ExtendDiscoveredCheck);
    fprintf(fout, "  extend_singular_reply: %d\n", ExtendSingularReply);
    fprintf(fout, "  extend_passed_pawn: %d\n", ExtendPassedPawn);
    fprintf(fout, "  extend_zugzwang: %d\n", ExtendZugzwang);
    fprintf(fout, "  reduce_null_move: %d\n", ReduceNullMove);
    fprintf(fout, "  reduce_null_move_deep: %d\n", ReduceNullMoveDeep);
    print_array(fout, "extend_recapture", ExtendRecapture + 1, 5);
    fprintf(fout, "\n");

    fprintf(fout, "pawn:\n");
    fprintf(fout, "  doubled: %d\n", DoubledPawn);
    fprintf(fout, "  backward: %d\n", BackwardPawn);
    fprintf(fout, "  hidden_backward: %d\n", HiddenBackwardPawn);
    fprintf(fout, "  outruns_king: %d\n", PawnOutrunsKing);
    fprintf(fout, "  blocked_development: %d\n", PawnDevelopmentBlocked);
    fprintf(fout, "  duo: %d\n", PawnDuo);
    fprintf(fout, "  storm: %d\n", PawnStorm);
    fprintf(fout, "  cramping: %d\n", CrampingPawn);
    fprintf(fout, "  majority: %d\n", PawnMajority);
    fprintf(fout, "  covered_passed_pawn_6th_rank: %d\n", CoveredPassedPawn6th);
    fprintf(fout, "  covered_passed_pawn_7th_rank: %d\n", CoveredPassedPawn7th);
    print_array(fout, "passed", PassedPawn + 1, 6);
    print_array(fout, "passed_blocked", PassedPawnBlocked + 1, 6);
    print_array(fout, "passed_connected", PassedPawnConnected + 1, 6);
    print_array(fout, "isolated", IsolatedPawn, 8);
    print_array(fout, "advance_opening", PawnAdvanceOpening, 8);
    print_array(fout, "advance_middle_game", PawnAdvanceMiddlegame, 8);
    print_array(fout, "advance_end_game", PawnAdvanceEndgame, 8);
    print_array(fout, "distant_passed", DistantPassedPawn, 35);
    print_array(fout, "scale_half_open_files_mine", ScaleHalfOpenFilesMine, 5);
    print_array(fout, "scale_half_open_files_yours", ScaleHalfOpenFilesYours,
                5);
    print_array(fout, "scale_open_files", ScaleOpenFiles, 5);
    fprintf(fout, "\n");

    fprintf(fout, "knight:\n");
    fprintf(fout, "  value: %d\n", Value[Knight]);
    fprintf(fout, "  king_proximity: %d\n", KnightKingProximity);
    fprintf(fout, "  blocks_c_pawn: %d\n", KnightBlocksCPawn);
    fprintf(fout, "  edge_penalty: %d\n", KnightEdgePenalty);
    fprintf(fout, "  piece_square_table: [\n");
    print_piece_square_table(fout, KnightPos);
    fprintf(fout, "    ]\n");
    fprintf(fout, "  piece_square_table_outpost: [\n");
    print_piece_square_table(fout, KnightOutpost);
    fprintf(fout, "    ]\n");
    fprintf(fout, "\n");

    fprintf(fout, "bishop:\n");
    fprintf(fout, "  value: %d\n", Value[Bishop]);
    fprintf(fout, "  mobility: %d\n", BishopMobility);
    fprintf(fout, "  king_proximity: %d\n", BishopKingProximity);
    fprintf(fout, "  trapped: %d\n", BishopTrapped);
    print_array(fout, "pair", BishopPair, 9);
    fprintf(fout, "  piece_square_table: [\n");
    print_piece_square_table(fout, BishopPos);
    fprintf(fout, "    ]\n");
    fprintf(fout, "\n");

    fprintf(fout, "rook:\n");
    fprintf(fout, "  value: %d\n", Value[Rook]);
    fprintf(fout, "  mobility: %d\n", RookMobility);
    fprintf(fout, "  on_open_file: %d\n", RookOnOpenFile);
    fprintf(fout, "  on_semi_open_file: %d\n", RookOnSemiOpenFile);
    fprintf(fout, "  king_proximity: %d\n", RookKingProximity);
    fprintf(fout, "  connected: %d\n", RookConnected);
    fprintf(fout, "  behind_passer: %d\n", RookBehindPasser);
    fprintf(fout, "  on_7th_rank: %d\n", RookOn7thRank);
    fprintf(fout, "  piece_square_table: [\n");
    print_piece_square_table(fout, RookPos);
    fprintf(fout, "    ]\n");
    fprintf(fout, "\n");

    fprintf(fout, "queen:\n");
    fprintf(fout, "  value: %d\n", Value[Queen]);
    fprintf(fout, "  king_proximity: %d\n", QueenKingProximity);
    fprintf(fout, "  piece_square_table: [\n");
    print_piece_square_table(fout, QueenPos);
    fprintf(fout, "    ]\n");
    fprintf(fout, "  piece_square_table_development: [\n");
    print_piece_square_table(fout, QueenPosDevelopment);
    fprintf(fout, "    ]\n");
    fprintf(fout, "\n");

    fprintf(fout, "king:\n");
    fprintf(fout, "  blocks_rook: %d\n", KingBlocksRook);
    fprintf(fout, "  in_center: %d\n", KingInCenter);
    fprintf(fout, "  safety_scale: %d\n", KingSafetyScale);
    fprintf(fout, "  piece_square_table_middle_game: [\n");
    print_piece_square_table(fout, KingPosMiddlegame);
    fprintf(fout, "    ]\n");
    fprintf(fout, "  piece_square_table_end_game: [\n");
    print_piece_square_table(fout, KingPosEndgame);
    fprintf(fout, "    ]\n");
    fprintf(fout, "  piece_square_table_end_game_queen_side: [\n");
    print_piece_square_table(fout, KingPosEndgameQueenSide);
    fprintf(fout, "    ]\n");

    fclose(fout);

    Print(0, "Saved configuration '%s' to '%s'.\n", ConfigurationName,
          file_name);
}

/**
 * Writes a piece-square table to file fout.
 */
static void print_piece_square_table(FILE *fout, int16_t *piece_square_table) {
    for (int rank = 0; rank < 8; rank++) {
        fprintf(fout, "    ");
        for (int file = 0; file < 8; file++) {
            fprintf(fout, "%5d, ", (int)piece_square_table[8 * rank + file]);
        }
        fprintf(fout, "\n");
    }
}

/**
 * Writes an array to file fout in a single line.
 */
static void print_array(FILE *fout, char *prefix, int16_t *array,
                        size_t count) {
    fprintf(fout, "  %s: [", prefix);
    for (size_t i = 0; i < count; i++) {
        fprintf(fout, "%d, ", (int)array[i]);
        if ((i % 10) == 9) {
            fprintf(fout, "\n    ");
        }
    }
    fprintf(fout, "]\n");
}

/**
 * Set a named parameter.
 */
static void set_parameter(struct Node *node, char *name, int *parameter) {
    struct IntLookupResult result = get_as_int(node, name);
    if (result.result_code == OK) {
        Print(9, "%s: %d\n", name, result.result);
        *parameter = result.result;
    }
}

static void set_piece_square_table(struct Node *node, char *name,
                                   int16_t *target_table) {
    int piece_square_table[64];

    struct IntArrayLookupResult array_result =
        get_as_int_array(node, name, piece_square_table, 64);

    if (array_result.result_code == OK) {
        if (array_result.elements_read != 64) {
            Print(0, "Warning: expected 64 entries for %s, got %d!\n", name,
                  array_result.elements_read);
        }
        Print(9, "%s:\n", name);
        for (unsigned int i = 0; i < array_result.elements_read; i++) {
            target_table[i] = (int16_t)piece_square_table[i];
            Print(9, "%5d, ", piece_square_table[i]);
            if (i % 8 == 7) {
                Print(9, "\n");
            }
        }
    }
}

static void set_array(struct Node *node, char *name, int16_t *target_array,
                      size_t count) {
    int *destination = malloc(sizeof(int) * count);
    abort_if_allocation_failed(destination);

    struct IntArrayLookupResult array_result =
        get_as_int_array(node, name, destination, count);

    if (array_result.result_code == OK) {
        if (array_result.elements_read != count) {
            Print(0, "Warning: expected %d entries for %s, got %d!\n", count,
                  name, array_result.elements_read);
        }
        Print(9, "%s: ", name);
        for (unsigned int i = 0; i < array_result.elements_read; i++) {
            target_array[i] = (int16_t)destination[i];
            Print(9, "%d, ", destination[i]);
        }
        Print(9, "\n");
    }

    free(destination);
}

static void configure_name(struct Node *node) {
    struct StringLookupResult result = get_as_string(node, "name");

    if (result.result_code == OK) {
        ConfigurationName = result.result;
        abort_if_allocation_failed(ConfigurationName);
        Print(0, "Using configuration name: %s\n", ConfigurationName);
    }
}

static void configure_search(struct Node *node) {
    set_parameter(node, "search.extend_in_check", &ExtendInCheck);
    set_parameter(node, "search.extend_double_check", &ExtendDoubleCheck);
    set_parameter(node, "search.extend_discovered_check",
                  &ExtendDiscoveredCheck);
    set_parameter(node, "search.extend_singular_reply", &ExtendSingularReply);
    set_parameter(node, "search.extend_passed_pawn", &ExtendPassedPawn);
    set_parameter(node, "search.extend_zugzwang", &ExtendZugzwang);
    set_parameter(node, "search.reduce_null_move", &ReduceNullMove);
    set_parameter(node, "search.reduce_null_move_deep", &ReduceNullMoveDeep);
    set_array(node, "search.extend_recapture", ExtendRecapture + 1, 5);
}

static void configure_pawn_scores(struct Node *node) {
    set_parameter(node, "pawn.doubled", &DoubledPawn);
    set_parameter(node, "pawn.backward", &BackwardPawn);
    set_parameter(node, "pawn.hidden_backward", &HiddenBackwardPawn);
    set_parameter(node, "pawn.outruns_king", &PawnOutrunsKing);
    set_parameter(node, "pawn.blocked_development", &PawnDevelopmentBlocked);
    set_parameter(node, "pawn.duo", &PawnDuo);
    set_parameter(node, "pawn.storm", &PawnStorm);
    set_parameter(node, "pawn.cramping", &CrampingPawn);
    set_parameter(node, "pawn.majority", &PawnMajority);
    set_parameter(node, "pawn.covered_passed_pawn_6th_rank",
                  &CoveredPassedPawn6th);
    set_parameter(node, "pawn.covered_passed_pawn_7th_rank",
                  &CoveredPassedPawn7th);
    set_array(node, "pawn.passed", PassedPawn + 1, 6);
    set_array(node, "pawn.passed_blocked", PassedPawnBlocked + 1, 6);
    set_array(node, "pawn.passed_connected", PassedPawnConnected + 1, 6);
    set_array(node, "pawn.isolated", IsolatedPawn, 8);
    set_array(node, "pawn.advance_opening", PawnAdvanceOpening, 8);
    set_array(node, "pawn.advance_middle_game", PawnAdvanceMiddlegame, 8);
    set_array(node, "pawn.advance_end_game", PawnAdvanceEndgame, 8);
    set_array(node, "pawn.distant_passed", DistantPassedPawn, 35);
    set_array(node, "pawn.scale_half_open_files_mine", ScaleHalfOpenFilesMine,
              5);
    set_array(node, "pawn.scale_half_open_files_yours", ScaleHalfOpenFilesYours,
              5);
    set_array(node, "pawn.scale_open_files", ScaleOpenFiles, 5);
}

static void configure_knight_scores(struct Node *node) {
    set_parameter(node, "knight.value", &Value[Knight]);
    set_parameter(node, "knight.king_proximity", &KnightKingProximity);
    set_parameter(node, "knight.blocks_c_pawn", &KnightBlocksCPawn);
    set_parameter(node, "knight.edge_penalty", &KnightEdgePenalty);

    set_piece_square_table(node, "knight.piece_square_table", KnightPos);
    set_piece_square_table(node, "knight.piece_square_table_outpost",
                           KnightOutpost);
}

static void configure_bishop_scores(struct Node *node) {
    set_parameter(node, "bishop.value", &Value[Bishop]);
    set_parameter(node, "bishop.mobility", &BishopMobility);
    set_parameter(node, "bishop.king_proximity", &BishopKingProximity);
    set_parameter(node, "bishop.trapped", &BishopTrapped);

    set_array(node, "bishop.pair", BishopPair, 9);
    set_piece_square_table(node, "bishop.piece_square_table", BishopPos);
}

static void configure_rook_scores(struct Node *node) {
    set_parameter(node, "rook.value", &Value[Rook]);
    set_parameter(node, "rook.mobility", &RookMobility);
    set_parameter(node, "rook.on_open_file", &RookOnOpenFile);
    set_parameter(node, "rook.on_semi_open_file", &RookOnSemiOpenFile);
    set_parameter(node, "rook.king_proximity", &RookKingProximity);
    set_parameter(node, "rook.connected", &RookConnected);
    set_parameter(node, "rook.behind_passer", &RookBehindPasser);
    set_parameter(node, "rook.on_7th_rank", &RookOn7thRank);
    set_piece_square_table(node, "rook.piece_square_table", RookPos);
}

static void configure_queen_scores(struct Node *node) {
    set_parameter(node, "queen.value", &Value[Queen]);
    set_parameter(node, "queen.king_proximity", &QueenKingProximity);
    set_piece_square_table(node, "queen.piece_square_table", QueenPos);
    set_piece_square_table(node, "queen.piece_square_table_development",
                           QueenPosDevelopment);
}

static void configure_king_scores(struct Node *node) {
    set_parameter(node, "king.blocks_rook", &KingBlocksRook);
    set_parameter(node, "king.in_center", &KingInCenter);
    set_parameter(node, "king.safety_scale", &KingSafetyScale);
    set_piece_square_table(node, "king.piece_square_table_middle_game",
                           KingPosMiddlegame);
    set_piece_square_table(node, "king.piece_square_table_end_game",
                           KingPosEndgame);
    set_piece_square_table(node, "king.piece_square_table_end_game_queen_side",
                           KingPosEndgameQueenSide);
}

static char *read_file(char *file_name) {
    FILE *fin = fopen(file_name, "r");
    if (!fin) {
        perror("Cannot open file");
        return NULL;
    }

    const size_t page_size = 1024;
    size_t buf_size = page_size;
    size_t total_bytes_read = 0;

    char *buffer = malloc(buf_size);
    abort_if_allocation_failed(buffer);

    char *ptr = buffer;

    for (;;) {
        size_t bytes_read = fread(ptr, 1, page_size, fin);

        if (bytes_read == 0)
            break;

        ptr += bytes_read;
        total_bytes_read += bytes_read;

        if ((total_bytes_read + page_size) >= buf_size) {
            buf_size *= 2;
            buffer = realloc(buffer, buf_size);
            abort_if_allocation_failed(buffer);
            ptr = buffer + total_bytes_read;
        }
    }

    fclose(fin);

    if ((total_bytes_read + 1) >= buf_size) {
        buf_size += 1;
        buffer = realloc(buffer, buf_size);
        abort_if_allocation_failed(buffer);
    }
    *ptr = '\0';

    return buffer;
}
