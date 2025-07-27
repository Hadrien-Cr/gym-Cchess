#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <unistd.h>
#include <sys/time.h>
#include "./nnue/nnue.h"

#define U64 unsigned long long
#define get_bit(bitboard, square) ((bitboard) & (1ULL << square))
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << square))
#define pop_bit(bitboard, square) (get_bit(bitboard, square) ? ((bitboard) ^= (1ULL << square)) : 0)
#define encode_move(from, to, piece, promotion, capture,  double_move, en_passant, castling) \
    ((from) | ((to) << 6) | ((piece) << 12) | ((promotion) << 16) | ((capture) << 20) | ((double_move) << 21) | ((en_passant) << 22) | ((castling) << 23))
#define decode_move_source(move) (move & 0x3f)
#define decode_move_target(move) (((move) & 0xfc0) >> 6) 
#define decode_move_piece(move) (((move)  & 0xf000) >> 12)
#define decode_move_promotion(move) (((move) & 0xf0000) >> 16)
#define decode_move_capture(move) (((move) & 0x100000) ? 1 : 0)
#define decode_move_double(move) (((move) & 0x200000) ? 1 : 0)
#define decode_move_en_passant(move) (((move) & 0x400000) ? 1 : 0)
#define decode_move_castling(move) (((move) & 0x800000) ? 1 : 0)

#define hash_size 100000
#define no_hash_entry 100000

#define MAX_PLY 64
#define MAX_VAL 50000
#define MATE_VALUE 49000
#define MATE_SCORE 48000
#define FULL_DEPTH_MOVES 4
#define REDUCTION_LIMIT 3
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "

enum squares {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1,
};
enum hash_flags { hash_flag_exact, hash_flag_alpha, hash_flag_beta};
enum castling_rights {wk = 1, wq = 2, bk = 4, bq = 8};
enum pieces { P, N, B, R, Q, K, p, n, b, r, q, k };

typedef struct {
    int moves[256];
    int move_count;
} move_list;

char *unicode_pieces[12] = {(char*)"♟︎", (char*)"♞", (char*)"♝", (char*)"♜", (char*)"♛", (char*)"♚", (char*)"♙", (char*)"♘", (char*)"♗", (char*)"♖", (char*)"♕", (char*)"♔"};

char get_piece_from_char(char c) {
    switch (c) {
        case 'Q': return Q;
        case 'R': return R;
        case 'B': return B;
        case 'N': return N;
        case 'P': return P;
        case 'K': return K;
        case 'q': return q;
        case 'r': return r;
        case 'b': return b;
        case 'n': return n;
        case 'p': return p;
        case 'k': return k;
        default: return P;
    }
}
char get_promoted_piece_char(int piece) {
    switch (piece) {
        case Q: case q: return 'q';
        case R: case r: return 'r';
        case B: case b: return 'b';
        case N: case n: return 'n';
        default: return 0;
    }
}

const char *square_names[64] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
};

const int castling_rights_sq[64] = {
     7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14
};

const int bishop_rel_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 7, 7, 7, 5, 5, 5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5, 5, 5, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 5, 5, 5, 5, 5, 6
};

const int rook_rel_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11, 12, 11, 11, 11, 11, 11, 11, 12,
};

const U64 rook_magic[64] = {
    0x8a80104000800020ULL, 0x140002000100040ULL, 0x2801880a0017001ULL, 0x100081001000420ULL, 0x200020010080420ULL,
    0x3001c0002010008ULL, 0x8480008002000100ULL, 0x2080088004402900ULL, 0x800098204000ULL, 0x2024401000200040ULL,
    0x100802000801000ULL, 0x120800800801000ULL, 0x208808088000400ULL, 0x2802200800400ULL, 0x2200800100020080ULL,
    0x801000060821100ULL, 0x80044006422000ULL, 0x100808020004000ULL, 0x12108a0010204200ULL, 0x140848010000802ULL,
    0x481828014002800ULL, 0x8094004002004100ULL, 0x4010040010010802ULL, 0x20008806104ULL, 0x100400080208000ULL,
    0x2040002120081000ULL, 0x21200680100081ULL, 0x20100080080080ULL, 0x2000a00200410ULL, 0x20080800400ULL,
    0x80088400100102ULL, 0x80004600042881ULL, 0x4040008040800020ULL, 0x440003000200801ULL, 0x4200011004500ULL,
    0x188020010100100ULL, 0x14800401802800ULL, 0x2080040080800200ULL, 0x124080204001001ULL, 0x200046502000484ULL,
    0x480400080088020ULL, 0x1000422010034000ULL, 0x30200100110040ULL, 0x100021010009ULL, 0x2002080100110004ULL,
    0x202008004008002ULL, 0x20020004010100ULL, 0x2048440040820001ULL, 0x101002200408200ULL, 0x40802000401080ULL,
    0x4008142004410100ULL, 0x2060820c0120200ULL, 0x1001004080100ULL, 0x20c020080040080ULL, 0x2935610830022400ULL,
    0x44440041009200ULL, 0x280001040802101ULL, 0x2100190040002085ULL, 0x80c0084100102001ULL, 0x4024081001000421ULL,
    0x20030a0244872ULL, 0x12001008414402ULL, 0x2006104900a0804ULL, 0x1004081002402ULL,
};
const U64 bishop_magic[64] = {
    0x40040844404084ULL, 0x2004208a004208ULL, 0x10190041080202ULL, 0x108060845042010ULL, 0x581104180800210ULL,
    0x2112080446200010ULL, 0x1080820820060210ULL, 0x3c0808410220200ULL, 0x4050404440404ULL, 0x21001420088ULL,
    0x24d0080801082102ULL, 0x1020a0a020400ULL, 0x40308200402ULL, 0x4011002100800ULL, 0x401484104104005ULL,
    0x801010402020200ULL, 0x400210c3880100ULL, 0x404022024108200ULL, 0x810018200204102ULL, 0x4002801a02003ULL,
    0x85040820080400ULL, 0x810102c808880400ULL, 0xe900410884800ULL, 0x8002020480840102ULL, 0x220200865090201ULL,
    0x2010100a02021202ULL, 0x152048408022401ULL, 0x20080002081110ULL, 0x4001001021004000ULL, 0x800040400a011002ULL,
    0xe4004081011002ULL, 0x1c004001012080ULL, 0x8004200962a00220ULL, 0x8422100208500202ULL, 0x2000402200300c08ULL,
    0x8646020080080080ULL, 0x80020a0200100808ULL, 0x2010004880111000ULL, 0x623000a080011400ULL, 0x42008c0340209202ULL,
    0x209188240001000ULL, 0x400408a884001800ULL, 0x110400a6080400ULL, 0x1840060a44020800ULL, 0x90080104000041ULL,
    0x201011000808101ULL, 0x1a2208080504f080ULL, 0x8012020600211212ULL, 0x500861011240000ULL, 0x180806108200800ULL,
    0x4000020e01040044ULL, 0x300000261044000aULL, 0x802241102020002ULL, 0x20906061210001ULL, 0x5a84841004010310ULL,
    0x4010801011c04ULL, 0xa010109502200ULL, 0x4a02012000ULL, 0x500201010098b028ULL, 0x8040002811040900ULL,
    0x28000010020204ULL, 0x6000020202d0240ULL, 0x8918844842082200ULL, 0x4010011029020020ULL,
};

const U64 not_a_file = 18374403900871474942ULL;
const U64 not_h_file = 9187201950435737471ULL;
const U64 not_hg_file = 4557430888798830399ULL;
const U64 not_ab_file = 18229723555195321596ULL;

int nnue_pieces[12] = { 6, 5, 4, 3, 2, 1, 12, 11, 10, 9, 8, 7 };
int nnue_squares[64] = {
    a1, b1, c1, d1, e1, f1, g1, h1, a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3, a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5, a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7, a8, b8, c8, d8, e8, f8, g8, h8
};

static int mvv_lva[12][12] = {
    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,
    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};

class ChessBoard {
public:
    // State Attributes
    U64 piece_bitboards[12];
    U64 block_bitboards[3];
    int side_to_move;
    int en_passant_square;
    int castling_rights;
    int fifty;
    U64 hash_key;
    U64 repetition_table[150];
    int repetition_index;
    int pv_table[MAX_PLY][MAX_PLY];

    ChessBoard(const std::string& fen) {
        init_all();
        char* fen_char = new char[fen.length() + 1];
        strcpy(fen_char, fen.c_str());
        parse_fen(fen_char);
        delete[] fen_char;
    }

    void parse_fen(char *fen){
        memset(piece_bitboards, 0ULL, sizeof(piece_bitboards));
        memset(block_bitboards, 0ULL, sizeof(block_bitboards));
        
        side_to_move = 0;
        en_passant_square = -1;
        castling_rights= 0;
        
        for (int rank = 0; rank < 8; rank++){
            for (int file = 0; file < 8; file++){
                int square = rank * 8 + file;
                
                if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')){
                    int piece = get_piece_from_char(*fen);
                    set_bit(piece_bitboards[piece], square);
                    *fen++;
                }
                
                if (*fen >= '0' && *fen <= '9'){
                    int offset = *fen - '0';
                    int piece = -1;
                    
                    for (int bb_piece = P; bb_piece <= k; bb_piece++){
                        if (get_bit(piece_bitboards[bb_piece], square))
                            piece = bb_piece;
                    }
                    
                    if (piece == -1)
                        file--;
                    
                    file += offset;
                    
                    *fen++;
                }
                
                if (*fen == '/')
                    *fen++;
            }
        }
        
        *fen++;
        
        (*fen == 'w') ? (side_to_move = white) : (side_to_move = black);
        fen += 2;
        
        while (*fen != ' '){
            switch (*fen)
            {
                case 'K': castling_rights |= wk; break;
                case 'Q': castling_rights |= wq; break;
                case 'k': castling_rights |= bk; break;
                case 'q': castling_rights |= bq; break;
                case '-': break;
            }

            *fen++;
        }
        
        *fen++;
        
        if (*fen != '-'){
            int file = fen[0] - 'a';
            int rank = 8 - (fen[1] - '0');
            en_passant_square = rank * 8 + file;
        }
        
        else
            en_passant_square = -1;
        
        fen++;
        fifty = atoi(fen);
        for (int piece = P; piece <= K; piece++)
            block_bitboards[white] |= piece_bitboards[piece];
        
        for (int piece = p; piece <= k; piece++)
            block_bitboards[black] |= piece_bitboards[piece];
        
        block_bitboards[2] |= block_bitboards[white];
        block_bitboards[2] |= block_bitboards[black];
        hash_key = generate_hash_key();
    }

    void print_board(){
        for (int rank = 0; rank < 8; rank++){
            for (int file = 0; file < 8; file++){
                if (file == 0){
                    printf(" %d ", 8 - rank);
                }
                int piece_found = -1;
                for (int piece = 0; piece < 12; piece++) {
                    if (get_bit(piece_bitboards[piece], rank * 8 + file)) {
                        piece_found = piece;
                        break;
                    }
                }
                if(piece_found != -1) {
                    printf(" %s ", unicode_pieces[piece_found]);
                } else {
                    printf(" . ");
                }
            }
            printf("\n");
        }
        printf("    a  b  c  d  e  f  g  h\n");
        printf("Side to move:      %s\n", side_to_move == white ? "White" : "Black");
        if (en_passant_square != -1) {
            printf("En passant square: %s\n", square_names[en_passant_square]);
        } else {
            printf("En passant square: None\n");
        }
        printf("Castling rights:   %c%c%c%c\n",
               (castling_rights & wk ? 'K' : '-'),
               (castling_rights & wq ? 'Q' : '-'),
               (castling_rights & bk ? 'k' : '-'),
               (castling_rights & bq ? 'q' : '-'));
        printf("     Hash key:  %llx\n\n", hash_key);
    }

    void search_position(int depth){
        int score = 0;
        nodes = 0;
        ply = 0;
        follow_pv = 0;
        score_pv = 0;
        memset(pv_table, 0, sizeof(pv_table));
        memset(pv_length, 0, sizeof(pv_length));
        memset(killer_moves, 0, sizeof(killer_moves));
        memset(history_moves, 0, sizeof(history_moves));

        int alpha = -MAX_VAL;
        int beta = MAX_VAL;
        
        for (int current_depth = 1; current_depth <= depth; current_depth++){
            follow_pv = 1;
            score = negamax(current_depth, alpha, beta);
            if ((score <= alpha) || (score >= beta)){
                alpha = -MAX_VAL;
                beta = MAX_VAL;
                continue;
            }
            alpha = score - 50;
            beta = score + 50;

            printf("info score cp %d depth %d nodes %ld pv ", score, current_depth, nodes);
            for (int i = 0; i < pv_length[0]; i++){
                print_move(pv_table[0][i]);
                printf(" ");
            }
            printf("\n");

        }
        printf("\n");
        printf("bestmove ");
        print_move(pv_table[0][0]);
        printf("\n");
    }

    void print_move_uci(int move, char* move_string) {
        if (decode_move_promotion(move)) {
            sprintf(move_string, "%s%s%c", square_names[decode_move_source(move)], square_names[decode_move_target(move)], get_promoted_piece_char(decode_move_promotion(move)));
        } else {
            sprintf(move_string, "%s%s", square_names[decode_move_source(move)], square_names[decode_move_target(move)]);
        }
    }

    void generate_moves(move_list* moves_list){
        moves_list->move_count = 0;
        int source_square, target_square;
        U64 piece_bitboards_copy, attacks;

        for (int piece = P; piece <= k; piece++){
            piece_bitboards_copy = piece_bitboards[piece];
            // White pawns
            if (side_to_move == white && piece == P){
                while (piece_bitboards_copy){
                    source_square = get_lsb_index(piece_bitboards_copy);
                    target_square = source_square - 8;
                    if (target_square >= 0 && target_square < 64 && !get_bit(block_bitboards[2], target_square)){
                        if (source_square >= a7 && source_square <= h7){
                            add_move(moves_list, encode_move(source_square, target_square, piece, Q, 0, 0, 0, 0));
                            add_move(moves_list, encode_move(source_square, target_square, piece, R, 0, 0, 0, 0));
                            add_move(moves_list, encode_move(source_square, target_square, piece, B, 0, 0, 0, 0));
                            add_move(moves_list, encode_move(source_square, target_square, piece, N, 0, 0, 0, 0));
                        }
                        else{
                            if (!get_bit(block_bitboards[2], target_square)){
                                add_move(moves_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                            }
                            if (source_square >= a2 && source_square <= h2 && !get_bit(block_bitboards[2], target_square - 8)){
                                add_move(moves_list, encode_move(source_square, target_square - 8, piece, 0, 0, 1, 0, 0));
                            }
                        }
                    }
                    attacks = pawn_attacks[white][source_square] & block_bitboards[black];
                    while (attacks){
                        target_square = get_lsb_index(attacks);
                        if (source_square >= a7 && source_square <= h7){
                            add_move(moves_list, encode_move(source_square, target_square, piece, Q, 1, 0, 0, 0));
                            add_move(moves_list, encode_move(source_square, target_square, piece, R, 1, 0, 0, 0));
                            add_move(moves_list, encode_move(source_square, target_square, piece, B, 1, 0, 0, 0));
                            add_move(moves_list, encode_move(source_square, target_square, piece, N, 1, 0, 0, 0));               
                        }
                        else {
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        } 
                        pop_bit(attacks, target_square);
                    }

                    if (en_passant_square != -1){
                        U64 enpassant_attacks= pawn_attacks[white][source_square] & (1ULL << en_passant_square);
                        if (enpassant_attacks){
                            target_square = get_lsb_index(enpassant_attacks);
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 1, 0, 1, 0));    
                        }
                    }
                    pop_bit(piece_bitboards_copy, source_square);
                }
            }
            // White king castles
            if (side_to_move == white && piece == K){
                if (castling_rights & wk){
                    if (!get_bit(block_bitboards[2], f1) && !get_bit(block_bitboards[2], g1) && !is_square_attacked(f1, black)){
                        source_square = e1;
                        target_square = g1;
                        add_move(moves_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 1)); 
                    }
                }
                if (castling_rights & wq){
                    if (!get_bit(block_bitboards[2], d1) && !get_bit(block_bitboards[2], c1) && !get_bit(block_bitboards[2], b1) && !is_square_attacked(d1, black)){
                        source_square = e1;
                        target_square = c1;
                        add_move(moves_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 1)); 
                    }
                }
            }
            // Black pawns
            if (side_to_move == black && piece == p){
                while (piece_bitboards_copy){
                    source_square = get_lsb_index(piece_bitboards_copy);
                    target_square = source_square + 8;
                    if (target_square >= 0 && target_square < 64 && !get_bit(block_bitboards[2], target_square)){
                        if (source_square >= a2 && source_square <= h2){
                            add_move(moves_list, encode_move(source_square, target_square, piece, q, 0, 0, 0, 0));
                            add_move(moves_list, encode_move(source_square, target_square, piece, r, 0, 0, 0, 0));
                            add_move(moves_list, encode_move(source_square, target_square, piece, b, 0, 0, 0, 0));
                            add_move(moves_list, encode_move(source_square, target_square, piece, n, 0, 0, 0, 0));
                        }
                        else {
                            if (!get_bit(block_bitboards[2], target_square)){
                                add_move(moves_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                            }
                            if (source_square >= a7 && source_square <= h7 && !get_bit(block_bitboards[2], target_square + 8)){
                                add_move(moves_list, encode_move(source_square, target_square + 8, piece, 0, 0, 1, 0, 0));
                            }
                        }
                    }
                    attacks = pawn_attacks[black][source_square] & block_bitboards[white];
                    while (attacks){
                        target_square = get_lsb_index(attacks);
                        if (source_square >= a2 && source_square <= h2){
                            add_move(moves_list, encode_move(source_square, target_square, piece, q, 1, 0, 0, 0));
                            add_move(moves_list, encode_move(source_square, target_square, piece, r, 1, 0, 0, 0));
                            add_move(moves_list, encode_move(source_square, target_square, piece, b, 1, 0, 0, 0));
                            add_move(moves_list, encode_move(source_square, target_square, piece, n, 1, 0, 0, 0));                   
                        }
                        else {
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        } 
                        pop_bit(attacks, target_square);
                    }

                    if (en_passant_square != -1){
                        U64 enpassant_attacks= pawn_attacks[black][source_square] & (1ULL << en_passant_square);
                        if (enpassant_attacks){
                            target_square = get_lsb_index(enpassant_attacks);
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 1, 0, 1, 0));   
                        }
                    }
                    pop_bit(piece_bitboards_copy, source_square);
                }
            }
            // Black king castles
            if (side_to_move == black && piece == k){
                if (castling_rights & bk){
                    if (!get_bit(block_bitboards[2], f8) && !get_bit(block_bitboards[2], g8) && !is_square_attacked(f8, white)){
                        source_square = e8;
                        target_square = g8;
                        add_move(moves_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 1)); 
                    }
                }
                if (castling_rights & bq){
                    if (!get_bit(block_bitboards[2], d8) && !get_bit(block_bitboards[2], c8) && !get_bit(block_bitboards[2], b8) && !is_square_attacked(d8, white)){
                        source_square = e8;
                        target_square = c8;
                        add_move(moves_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 1)); 
                    }
                }
            }
            // Knights
            if ((side_to_move ==white) ? (piece == N) : (piece == n)){
                while (piece_bitboards_copy){
                    source_square = get_lsb_index(piece_bitboards_copy);
                    attacks = knight_attacks[source_square] & ((side_to_move ==white) ? ~block_bitboards[white] : ~block_bitboards[black]);
                    while (attacks){
                        target_square = get_lsb_index(attacks);
                        if (get_bit(((side_to_move ==white) ? block_bitboards[black] : block_bitboards[white]), target_square)){
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        else {
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                        }
                        pop_bit(attacks, target_square);
                    }
                    pop_bit(piece_bitboards_copy, source_square);
                }
            }

            // Bishops
            if ((side_to_move ==white) ? (piece == B) : (piece == b)){
                while (piece_bitboards_copy){
                    source_square = get_lsb_index(piece_bitboards_copy);
                    attacks = get_bishop_attacks(source_square, block_bitboards[2]) & ((side_to_move ==white) ? ~block_bitboards[white] : ~block_bitboards[black]);
                    while (attacks){
                        target_square = get_lsb_index(attacks);
                        if (!get_bit(((side_to_move ==white) ? block_bitboards[black] : block_bitboards[white]), target_square)){
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                        }
                        else {
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        pop_bit(attacks, target_square);
                    }
                    pop_bit(piece_bitboards_copy, source_square);
                }
            }
            // Rooks
            if ((side_to_move ==white) ? (piece == R) : (piece == r)){
                while (piece_bitboards_copy){
                    source_square = get_lsb_index(piece_bitboards_copy);
                    attacks = get_rook_attacks(source_square, block_bitboards[2]) & ((side_to_move ==white) ? ~block_bitboards[white] : ~block_bitboards[black]);
                    while (attacks){
                        target_square = get_lsb_index(attacks);
                        if (!get_bit(((side_to_move ==white) ? block_bitboards[black] : block_bitboards[white]), target_square)){
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                        }
                        else {
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        pop_bit(attacks, target_square);
                    }
                    pop_bit(piece_bitboards_copy, source_square);
                }
            }
            // Queens
            if ((side_to_move ==white) ? (piece == Q) : (piece == q)){
                while (piece_bitboards_copy){
                    source_square = get_lsb_index(piece_bitboards_copy);
                    attacks = get_queen_attacks(source_square, block_bitboards[2]) & ((side_to_move ==white) ? ~block_bitboards[white] : ~block_bitboards[black]);
                    while (attacks){
                        target_square = get_lsb_index(attacks);
                        if (!get_bit(((side_to_move ==white) ? block_bitboards[black] : block_bitboards[white]), target_square)){
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                        }
                        else {
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        pop_bit(attacks, target_square);
                    }
                    pop_bit(piece_bitboards_copy, source_square);
                }
            }
            // Kings
            if ((side_to_move ==white) ? (piece == K) : (piece == k)){
                while (piece_bitboards_copy){
                    source_square = get_lsb_index(piece_bitboards_copy);
                    attacks = king_attacks[source_square] & ((side_to_move ==white) ? ~block_bitboards[white] : ~block_bitboards[black]);
                    while (attacks){
                        target_square = get_lsb_index(attacks);
                        if (!get_bit(((side_to_move ==white) ? block_bitboards[black] : block_bitboards[white]), target_square)){
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                        }
                        else {
                            add_move(moves_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        pop_bit(attacks, target_square);
                    }
                    pop_bit(piece_bitboards_copy, source_square);
                }
            }
        }
    }

    int apply_move(int move, int only_capture_flag) {
        if (!only_capture_flag){
            copy_board();

            int source = decode_move_source(move);
            int target = decode_move_target(move);
            int piece = decode_move_piece(move);
            int promotion = decode_move_promotion(move);
            int capture = decode_move_capture(move);
            int double_move = decode_move_double(move);
            int en_passant = decode_move_en_passant(move);
            int castling = decode_move_castling(move);

            pop_bit(piece_bitboards[piece], source);
            set_bit(piece_bitboards[piece], target);

            hash_key ^= piece_keys[piece][source];
            hash_key ^= piece_keys[piece][target];

            fifty++;
            
            if (piece == P || piece == p)
                fifty = 0;
            
            if (capture) {
                fifty = 0;
                int start_piece = (side_to_move == white ? p : P);
                int end_piece = (side_to_move == white ? k : K);
                for (int piece_idx = start_piece; piece_idx <= end_piece; piece_idx++) {
                    if (get_bit(piece_bitboards[piece_idx], target)) {
                        pop_bit(piece_bitboards[piece_idx], target);
                        hash_key ^= piece_keys[piece_idx][target];
                        break;
                    }
                }
            }
            
            if (promotion) {
                pop_bit(piece_bitboards[(side_to_move == white) ? P : p], target);
                hash_key ^= piece_keys[(side_to_move == white) ? P : p][target];
                set_bit(piece_bitboards[promotion], target);
                hash_key ^= piece_keys[promotion][target];
            }

            if (en_passant) {
                if (side_to_move == white) {
                    pop_bit(piece_bitboards[p], target + 8);
                    hash_key ^= piece_keys[p][target + 8];
                }
                else {
                    pop_bit(piece_bitboards[P], target - 8);
                    hash_key ^= piece_keys[P][target - 8];
                }
            }

            if (en_passant_square != -1){
                hash_key ^= enpassant_keys[en_passant_square];
            }

            en_passant_square = -1;
            if (double_move) {
                en_passant_square = target - 8 * (2 * side_to_move - 1);
                hash_key ^= enpassant_keys[en_passant_square];
            }
            
            if (castling) {
                if (is_square_attacked(source, 1 - side_to_move)) {
                    take_back();
                    return 0;
                }
                if (side_to_move == white) {
                    if (target == c1) {
                        pop_bit(piece_bitboards[R], a1);
                        set_bit(piece_bitboards[R], d1);
                        hash_key ^= piece_keys[R][a1];
                        hash_key ^= piece_keys[R][d1];
                    }
                    else {
                        pop_bit(piece_bitboards[R], h1);
                        set_bit(piece_bitboards[R], f1);
                        hash_key ^= piece_keys[R][h1];
                        hash_key ^= piece_keys[R][f1];
                    }
                }
                if (side_to_move == black) {
                    if (target == c8) {
                        pop_bit(piece_bitboards[r], a8);
                        set_bit(piece_bitboards[r], d8);
                        hash_key ^= piece_keys[R][a8];
                        hash_key ^= piece_keys[R][d8];
                    }
                    else {
                        pop_bit(piece_bitboards[r], h8);
                        set_bit(piece_bitboards[r], f8);
                        hash_key ^= piece_keys[R][h8];
                        hash_key ^= piece_keys[R][f8];
                    }
                }
            }

            hash_key ^= castle_keys[castling_rights];
            castling_rights &= castling_rights_sq[source];
            castling_rights &= castling_rights_sq[target];
            hash_key ^= castle_keys[castling_rights];

            memset(block_bitboards, 0ULL, 24);

            for (int i = P; i <= K; i++) {block_bitboards[white] |= piece_bitboards[i];}
            for (int i = p; i <= k; i++) {block_bitboards[black] |= piece_bitboards[i];}
            
            block_bitboards[2] |= block_bitboards[white];
            block_bitboards[2] |= block_bitboards[black];
            
            side_to_move ^= 1;
            hash_key ^= side_key;

            int king_position = (side_to_move == black ? get_lsb_index(piece_bitboards[K]) : get_lsb_index(piece_bitboards[k]));
            if (is_square_attacked(king_position, side_to_move)) {
                take_back();
                return 0;
            }
            U64 hash_from_scratch = generate_hash_key();
            return 1;
        }
        else {
            if (decode_move_capture(move))
                return apply_move(move, 0);
            else
                return 0;
        }
    }

private:
    U64 piece_keys[12][64];
    U64 enpassant_keys[64];
    U64 castle_keys[16];
    U64 side_key;
    unsigned int rd_state;

    U64 pawn_attacks[2][64];
    U64 knight_attacks[64];
    U64 king_attacks[64];
    U64 bishop_masks[64];
    U64 rook_masks[64];
    U64 bishop_attacks[64][512];
    U64 rook_attacks[64][4096];

    int killer_moves[2][MAX_PLY];
    int history_moves[12][MAX_PLY];
    int pv_length[MAX_PLY];
    
    int follow_pv, score_pv;
    long nodes;
    int ply;

    typedef struct {
        U64 hash_key;
        int depth;
        int flag;
        int score;
    } TT;

    TT hash_table[hash_size];

    U64 piece_bitboards_cp[12], block_bitboards_cp[3];
    int side_to_move_cp, en_passant_square_cp, castling_rights_cp, fifty_cp;
    U64 hash_key_cp;

    void copy_board() {
        memcpy(piece_bitboards_cp, piece_bitboards, 96);
        memcpy(block_bitboards_cp, block_bitboards, 24);
        side_to_move_cp = side_to_move;
        en_passant_square_cp = en_passant_square;
        castling_rights_cp = castling_rights;
        fifty_cp = fifty;
        hash_key_cp = hash_key;
    }

    void take_back() {
        memcpy(piece_bitboards, piece_bitboards_cp, 96);
        memcpy(block_bitboards, block_bitboards_cp, 24);
        side_to_move = side_to_move_cp;
        en_passant_square = en_passant_square_cp;
        castling_rights = castling_rights_cp;
        fifty = fifty_cp;
        hash_key = hash_key_cp;
    }

    int count_bits(U64 bitboard) {
        int count = 0;
        while (bitboard) {
            count++;
            bitboard &= (bitboard - 1);
        }
        return count;
    }

    int get_lsb_index(U64 bitboard) {
        if (bitboard) {
            return count_bits((bitboard & -bitboard) - 1);
        }
        else { return -1; }
    }

    void add_move(move_list *list, int move) {
        list->moves[list->move_count] = move;
        list->move_count++;
    }

    unsigned int get_random_U32_number(){
        unsigned int x = rd_state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        rd_state = x;
        return x;
    }

    U64 get_random_U64_number(){
        U64 x1, x2, x3, x4;
        x1 = get_random_U32_number() & 0xFFFF;
        x2 = get_random_U32_number() & 0xFFFF;
        x3 = get_random_U32_number() & 0xFFFF;
        x4 = get_random_U32_number() & 0xFFFF;
        return x1 | (x2 << 16) | (x3 << 32) | (x4 << 48);
    }

    U64 generate_hash_key(){
        U64 final_key = 0ULL;
        U64 bitboard;
        for (int piece = P; piece <= k; piece++){
            bitboard = piece_bitboards[piece];
            while (bitboard) {
                int square = get_lsb_index(bitboard);
                final_key ^= piece_keys[piece][square];
                pop_bit(bitboard, square);
            }
        }
        
        if (en_passant_square != -1)
            final_key ^= enpassant_keys[en_passant_square];
        
        final_key ^= castle_keys[castling_rights];
        
        if (side_to_move == black) final_key ^= side_key;
        
        return final_key;
    }

    void init_random_keys(){
        rd_state = 1804289383;

        for (int piece = P; piece <= k; piece++){
            for (int square = 0; square < 64; square++)
                piece_keys[piece][square] = get_random_U64_number();
        }
        
        for (int square = 0; square < 64; square++)
            enpassant_keys[square] = get_random_U64_number();
        
        for (int index = 0; index < 16; index++)
            castle_keys[index] = get_random_U64_number();
            
        side_key = get_random_U64_number();
    }

    U64 mask_pawn_attacks(int color, int square){
        U64 attacks = 0ULL;
        U64 bitboard = 0ULL;
        set_bit(bitboard, square);
        if (color == white){
            if ((bitboard >> 7) & not_a_file){attacks |= (bitboard >> 7);}
            if ((bitboard >> 9) & not_h_file){attacks |= (bitboard >> 9);}

        } else {
            if ((bitboard << 9) & not_a_file){attacks |= (bitboard << 9);}
            if ((bitboard << 7) & not_h_file){attacks |= (bitboard << 7);}
        }
        return attacks;
    }

    U64 mask_knight_attacks(int square){
        U64 attacks = 0ULL;
        U64 bitboard = 0ULL;
        set_bit(bitboard, square);

        if ((bitboard >> 17) & not_h_file){attacks |= (bitboard >> 17);}
        if ((bitboard >> 15) & not_a_file){attacks |= (bitboard >> 15);}
        if ((bitboard >> 10) & not_hg_file){attacks |= (bitboard >> 10);}
        if ((bitboard >> 6 ) & not_ab_file){attacks |= (bitboard >> 6);}

        if ((bitboard << 17) & not_a_file){attacks |= (bitboard << 17);}
        if ((bitboard << 15) & not_h_file){attacks |= (bitboard << 15);}
        if ((bitboard << 10) & not_ab_file){attacks |= (bitboard << 10);}
        if ((bitboard << 6 ) & not_hg_file){attacks |= (bitboard << 6);}
        return attacks;
    }

    U64 mask_king_attacks(int square){
        U64 attacks = 0ULL;
        U64 bitboard = 0ULL;
        set_bit(bitboard, square);

        if ((bitboard >> 7) & not_a_file){attacks |= (bitboard >> 7);}
        if ((bitboard >> 8)){attacks |= (bitboard >> 8);}
        if ((bitboard >> 9) & not_h_file){attacks |= (bitboard >> 9);}

        if ((bitboard << 7) & not_h_file){attacks |= (bitboard << 7);}
        if ((bitboard << 8)){attacks |= (bitboard << 8);}
        if ((bitboard << 9) & not_a_file){attacks |= (bitboard << 9);}
        
        if ((bitboard >> 1) & not_h_file){attacks |= (bitboard >> 1);}
        if ((bitboard << 1) & not_a_file){attacks |= (bitboard << 1);}
        return attacks;
    }

    U64 mask_bishop_attacks(int square){
        U64 attacks = 0ULL;
        int tr = square/8, tf = square%8;

        for (int r=tr+1, f = tf+1; r < 7 && f < 7; r++, f++){
            attacks |= (1ULL << (r*8 + f));
        }
        for (int r=tr+1, f = tf-1; r < 7 && f >= 1; r++, f--){
            attacks |= (1ULL << (r*8 + f));
        }
        for (int r=tr-1, f = tf+1; r >= 1 && f < 7; r--, f++){
            attacks |= (1ULL << (r*8 + f));
        }
        for (int r=tr-1, f = tf-1; r >= 1 && f >= 1; r--, f--){
            attacks |= (1ULL << (r*8 + f));
        }
        return attacks;
    }

    U64 bishop_attacks_otf(int square, U64 block){
        U64 attacks = 0ULL;
        int r, f;
        int tr = square / 8;
        int tf = square % 8;
        
        for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++){
            attacks |= (1ULL << (r * 8 + f));
            if ((1ULL << (r * 8 + f)) & block) break;
        }
        
        for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++){
            attacks |= (1ULL << (r * 8 + f));
            if ((1ULL << (r * 8 + f)) & block) break;
        }
        
        for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--){
            attacks |= (1ULL << (r * 8 + f));
            if ((1ULL << (r * 8 + f)) & block) break;
        }
        
        for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--){
            attacks |= (1ULL << (r * 8 + f));
            if ((1ULL << (r * 8 + f)) & block) break;
        }
        
        return attacks;
    }

    U64 mask_rook_attacks(int square){
        U64 attacks = 0ULL;
        int tr = square/8, tf = square%8;

        for (int r=tr+1; r<7; r++){
            attacks |= (1ULL << (r*8 + tf));
        }
        for (int r=tr-1; r>=1; r--){
            attacks |= (1ULL << (r*8 + tf));
        }
        for (int f=tf+1; f<7; f++){
            attacks |= (1ULL << (tr*8 + f));
        }
        for (int f=tf-1; f>=1; f--){
            attacks |= (1ULL << (tr*8 + f));
        }
        return attacks;
    }

    U64 rook_attacks_otf(int square, U64 blocking_pieces){
        U64 attacks = 0ULL;
        int tr = square/8, tf = square%8;

        for (int r=tr+1; r<=7; r++){
            attacks |= (1ULL << (r*8 + tf));
            if ((1ULL << (r*8 + tf)) & blocking_pieces){break;}
        }
        for (int r=tr-1; r>=0; r--){
            attacks |= (1ULL << (r*8 + tf));
            if ((1ULL << (r*8 + tf)) & blocking_pieces){break;}

        }
        for (int f=tf+1; f<=7; f++){
            attacks |= (1ULL << (tr*8 + f));
            if ((1ULL << (tr*8 + f)) & blocking_pieces){break;}
        }
        for (int f=tf-1; f>=0; f--){
            attacks |= (1ULL << (tr*8 + f));
            if ((1ULL << (tr*8 + f)) & blocking_pieces){break;}

        }
        return attacks;
    }

    U64 set_block(int index, int n_bits_in_mask, U64 attack_mask){
        U64 block = 0ULL;
        for (int i = 0; i < n_bits_in_mask; i++){
            int square = get_lsb_index(attack_mask);
            pop_bit(attack_mask, square);
            if (index & (1 << i)) {
                set_bit(block, square);
            }
        }
        return block;
    }

    void init_all(){
        init_random_keys();

        for (int color = 0; color < 2; color++){
            for (int square = 0; square < 64; square++){
                pawn_attacks[color][square] = mask_pawn_attacks(color, square);
            }
        }
        
        for (int square = 0; square < 64; square++){
            knight_attacks[square] = mask_knight_attacks(square);
            king_attacks[square] = mask_king_attacks(square);
        }

        for (int square = 0; square < 64; square++){
            bishop_masks[square] = mask_bishop_attacks(square);
            U64 attack_mask = bishop_masks[square];
            int relevant_bits = count_bits(attack_mask);
            int block_indicies = 1 << relevant_bits;

            for (int i = 0; i < block_indicies; i++){
                U64 block = set_block(i, relevant_bits, attack_mask);
                int magic_index = (int)((block * bishop_magic[square]) >> (64 - bishop_rel_bits[square]));
                bishop_attacks[square][magic_index] = bishop_attacks_otf(square, block);
            }
        }

        for (int square = 0; square < 64; square++){
            rook_masks[square] = mask_rook_attacks(square);
            U64 attack_mask = rook_masks[square];
            int relevant_bits = count_bits(attack_mask);
            int block_indicies = 1 << relevant_bits;

            for (int i = 0; i < block_indicies; i++){
                U64 block = set_block(i, relevant_bits, attack_mask);
                int magic_index = (int)((block * rook_magic[square]) >> (64 - rook_rel_bits[square]));
                rook_attacks[square][magic_index] = rook_attacks_otf(square, block);
            }
        }
        init_nnue((char*)"nn-eba324f53044.nnue");
    }

    void init_nnue(char *filename){
        nnue_init(filename);
    }

    U64 get_bishop_attacks(int square, U64 block){
        block &= bishop_masks[square];
        block *= bishop_magic[square];
        block >>= (64 - bishop_rel_bits[square]);
        return bishop_attacks[square][block];
    }

    U64 get_rook_attacks(int square, U64 block){
        block &= rook_masks[square];
        block *= rook_magic[square];
        block >>= (64 - rook_rel_bits[square]);
        return rook_attacks[square][block];
    }

    U64 get_queen_attacks(int square, U64 block){
        U64 result = 0ULL;
        U64 bishop_block = block;
        U64 rook_block = block;
        bishop_block &= bishop_masks[square];
        rook_block &= rook_masks[square];
        bishop_block *= bishop_magic[square];
        bishop_block >>= (64 - bishop_rel_bits[square]);
        rook_block *= rook_magic[square];
        rook_block >>= (64 - rook_rel_bits[square]);
        result |= bishop_attacks[square][bishop_block];
        result |= rook_attacks[square][rook_block];
        return result;
    }

    int is_square_attacked(int square, int side){
        if ((side  == white) && (pawn_attacks[black][square] & piece_bitboards[P])){return 1;}
        if ((side  == black) && (pawn_attacks[white][square] & piece_bitboards[p])){return 1;}
        if ((side  == white) && (knight_attacks[square] & piece_bitboards[N])){return 1;}
        if ((side  == black) && (knight_attacks[square] & piece_bitboards[n])){return 1;}
        if ((side  == white) && (king_attacks[square] & piece_bitboards[K])){return 1;}
        if ((side  == black) && (king_attacks[square] & piece_bitboards[k])){return 1;}
        if ((side  == white) && (get_bishop_attacks(square, block_bitboards[2]) & piece_bitboards[B])){return 1;}
        if ((side  == black) && (get_bishop_attacks(square, block_bitboards[2]) & piece_bitboards[b])){return 1;}
        if ((side  == white) && (get_rook_attacks(square, block_bitboards[2]) & piece_bitboards[R])){return 1;}
        if ((side  == black) && (get_rook_attacks(square, block_bitboards[2]) & piece_bitboards[r])){return 1;}
        if ((side  == white) && (get_queen_attacks(square, block_bitboards[2]) & piece_bitboards[Q])){return 1;}
        if ((side  == black) && (get_queen_attacks(square, block_bitboards[2]) & piece_bitboards[q])){return 1;}
        return 0;
    }

    int evaluate_nnue(int player, int *pieces, int *squares) {
        return nnue_evaluate(player, pieces, squares);
    }

    int evaluate_fen_nnue(char *fen) {
        return nnue_evaluate_fen(fen);
    }

    int evaluate() {    
        U64 bitboard;
        int piece, square;
        int pieces[33];
        int squares[33];
        int index = 2;
        
        for (int bb_piece = P; bb_piece <= k; bb_piece++) {
            bitboard = piece_bitboards[bb_piece];
            
            while (bitboard) {
                piece = bb_piece;
                
                square = get_lsb_index(bitboard);
                
                if (piece == K) {
                    pieces[0] = nnue_pieces[piece];
                    squares[0] = nnue_squares[square];
                }
                
                else if (piece == k) {
                    pieces[1] = nnue_pieces[piece];
                    squares[1] = nnue_squares[square];
                }
                
                else {
                    pieces[index] = nnue_pieces[piece];
                    squares[index] = nnue_squares[square];
                    index++;    
                }

                pop_bit(bitboard, square);
            }
        }
        
        pieces[index] = 0;
        squares[index] = 0;
        return (evaluate_nnue(side_to_move, pieces, squares) * (100 - fifty) / 100);
    }

    void enable_pv_scoring(move_list *move_list){
        follow_pv = 0;
        for (int i = 0; i < move_list->move_count; i++){
            int move = move_list->moves[i];
            if (pv_table[0][ply] == move){
                score_pv = 1;
                follow_pv = 1;
                break;
            }
        }
    }

    int score_move(int move){
        if (score_pv){
            if (pv_table[0][ply] == move){
                score_pv = 0;
                return 20000;
            }
        }
        if (decode_move_capture(move)){
            int target_piece = P;
            int start_piece, end_piece;
            
            if (side_to_move == white) { start_piece = p; end_piece = k; }
            else { start_piece = P; end_piece = K; }
            
            for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++){
                if (get_bit(piece_bitboards[bb_piece], decode_move_target(move))){
                    target_piece = bb_piece;
                    break;
                }
            }
            return mvv_lva[decode_move_piece(move)][target_piece] + 10000;
        }
        
        else {
            if (move == killer_moves[0][ply])
                return 9000;
            else if (move == killer_moves[1][ply])
                return 8000;
            else 
                return history_moves[decode_move_piece(move)][decode_move_target(move)];
        }
    }

    void sort_moves(move_list *move_list){
        int move_scores[move_list->move_count];
        
        for (int count = 0; count < move_list->move_count; count++)
            move_scores[count] = score_move(move_list->moves[count]);
        
        for (int current_move = 0; current_move < move_list->move_count; current_move++){
            for (int next_move = current_move + 1; next_move < move_list->move_count; next_move++){
                if (move_scores[current_move] < move_scores[next_move]){
                    int temp_score = move_scores[current_move];
                    move_scores[current_move] = move_scores[next_move];
                    move_scores[next_move] = temp_score;
                    
                    int temp_move = move_list->moves[current_move];
                    move_list->moves[current_move] = move_list->moves[next_move];
                    move_list->moves[next_move] = temp_move;
                }
            }
        }
    }

    void clear_hash_table(){
        for (int index = 0; index < hash_size; index++){
            hash_table[index].hash_key = 0;
            hash_table[index].depth = 0;
            hash_table[index].flag = 0;
            hash_table[index].score = 0;
        }
    }

    int read_tt_entry(int alpha, int beta, int depth){
        TT *hash_entry = &hash_table[hash_key % hash_size];
        
        if (hash_entry->hash_key == hash_key){
            if (hash_entry->depth >= depth){
                int score = hash_entry->score;
                if (score < -MATE_SCORE) score += ply;
                if (score > MATE_SCORE) score -= ply;
            
                if (hash_entry->flag == hash_flag_exact)
                    return score;
                
                if ((hash_entry->flag == hash_flag_alpha) &&
                    (score <= alpha))
                    return alpha;
                
                if ((hash_entry->flag == hash_flag_beta) &&
                    (score >= beta))
                    return beta;
            }
        }
        return no_hash_entry;
    }

    void write_tt_entry(int score, int depth, int hash_flag){
        TT *hash_entry = &hash_table[hash_key % hash_size];

        if (score < -MATE_SCORE) score -= ply;
        if (score > MATE_SCORE) score += ply;
        hash_entry->hash_key = hash_key;
        hash_entry->score = score;
        hash_entry->flag = hash_flag;
        hash_entry->depth = depth;
    }

    int is_repetition(){
        for (int index = 0; index < repetition_index; index++)
            if (repetition_table[index] == hash_key)
                return 1;
        return 0;
    }

    int quiescence(int alpha, int beta){
        nodes++;

        if (ply > MAX_VAL - 1)
            return evaluate();

        int evaluation = evaluate();
        
        if (evaluation >= beta){
            return beta;
        }
        
        if (evaluation > alpha){
            alpha = evaluation;
        }
        
        move_list move_list[1];
        generate_moves(move_list);
        sort_moves(move_list);
        
        for (int count = 0; count < move_list->move_count; count++){
            copy_board();
            ply++;
            repetition_index++;
            repetition_table[repetition_index] = hash_key;

            
            if (apply_move(move_list->moves[count], 1) == 0){
                ply--;
                repetition_index--;
                continue;
            }

            int score = -quiescence(-beta, -alpha);
            
            ply--;
            
            repetition_index--;

            take_back();
            
            
            if (score > alpha){
                alpha = score;
                
                if (score >= beta){
                    return beta;
                }
            }
        }
        
        return alpha;
    }

    int negamax(int depth, int alpha, int beta){
        int score;
        int hash_flag = hash_flag_alpha;
        
        if (ply && is_repetition() || fifty >= 100)
            return 0;
        
        int pv_node = beta - alpha > 1;
        
        if (ply && (score = read_tt_entry(alpha, beta, depth)) != no_hash_entry && pv_node == 0)
            return score;

        pv_length[ply] = ply;

        if (depth == 0)
            return quiescence(alpha, beta);

        if (ply > MAX_PLY- 1)
            return evaluate();

        nodes++;

        int is_in_check = is_square_attacked((side_to_move == white) ? get_lsb_index(piece_bitboards[K]) : get_lsb_index(piece_bitboards[k]), side_to_move ^ 1);
        if (is_in_check) depth++;

        int legal_moves_found = 0;

        if (depth >= 3 && is_in_check == 0 && !(ply == 0)){
            copy_board();
            ply++;
            repetition_index++;
            repetition_table[repetition_index] = hash_key;

            if (en_passant_square != -1) hash_key ^= enpassant_keys[en_passant_square];
            en_passant_square = -1;
            side_to_move ^= 1;
            hash_key ^= side_key;
            
            score = -negamax(depth - 1 - 2, -beta, -beta + 1);
            
            ply--;
            repetition_index--;
            take_back();
            
            if (score >= beta)
                return beta;
        }
        
        move_list list[1];
        generate_moves(list);
        
        if (follow_pv) enable_pv_scoring(list);
        
        sort_moves(list);

        int moves_searched = 0;

        for (int count = 0; count < list->move_count; count++){
            int move = list->moves[count];
            copy_board();
            ply++;

            repetition_index++;
            repetition_table[repetition_index] = hash_key;

            if (!apply_move(move, 0)){
                ply--;
                repetition_index--;
                continue;
            }
            

            legal_moves_found++;

            if (moves_searched == 0){
                score = -negamax(depth - 1, -beta, -alpha);
            }
            else {
                if(
                    moves_searched >= FULL_DEPTH_MOVES &&
                    depth >= REDUCTION_LIMIT &&
                    is_in_check == 0 && 
                    decode_move_capture(move) == 0 &&
                    decode_move_promotion(move)  == 0
                    )
                    score = -negamax(depth - 2, -alpha - 1, -alpha);
                
                else score = alpha + 1;
                if (score > alpha){
                    score = -negamax(depth - 1, -alpha - 1, -alpha);
                    if ((score > alpha) && (score < beta))
                        score = -negamax(depth-1, -beta, -alpha);
                }
            }

            ply--;
            repetition_index--;
            take_back();
            moves_searched++;
            
            if (score > alpha){
                hash_flag = hash_flag_exact;
                
                if (decode_move_capture(move) == 0){
                    history_moves[decode_move_piece(move)][decode_move_target(move)] += depth;
                }

                alpha = score;

                pv_table[ply][ply] = move;

                for (int i = ply+1; i < pv_length[ply + 1]; i++){
                    pv_table[ply][i] = pv_table[ply + 1][i];
                }
                pv_length[ply] = pv_length[ply + 1];
            
                if (score >= beta){
                    write_tt_entry(beta, depth, hash_flag_beta);
                    if (!decode_move_capture(move)){
                        killer_moves[1][ply] = killer_moves[0][ply];
                        killer_moves[0][ply] = move;
                    }
                    return beta;
                }

            }
        }

        if (legal_moves_found == 0){
            if (is_in_check){
                return -MATE_VALUE+ ply;
            }
            else {
                return 0;
            }
        }

        write_tt_entry(alpha, depth, hash_flag);
        return alpha;
    }

    void print_move(int move) {
        if (decode_move_promotion(move))
            printf("%s%s%c", square_names[decode_move_source(move)],square_names[decode_move_target(move)],get_promoted_piece_char(decode_move_promotion(move)));
        else
            printf("%s%s", square_names[decode_move_source(move)], square_names[decode_move_target(move)]);
    }
};