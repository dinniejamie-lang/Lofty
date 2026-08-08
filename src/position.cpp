// position.cpp — Zobrist init, FEN, make_move (copy-make), attack queries.
// Updated to maintain Incremental Evaluation (PSQT) state.
#include "position.h"

#include <array>
#include <cassert>
#include <sstream>
#include <string>
#include <charconv>

namespace lofty {

static constexpr std::array<CastleRights, SQUARE_NB> CastlingRightsMask = []() constexpr {
    std::array<CastleRights, SQUARE_NB> m{};
    for (int i = 0; i < SQUARE_NB; ++i) m[i] = ANY_CASTLING;
    m[SQ_E1] = CastleRights(int(ANY_CASTLING) ^ (WHITE_OO  | WHITE_OOO));
    m[SQ_A1] = CastleRights(int(ANY_CASTLING) ^  WHITE_OOO);
    m[SQ_H1] = CastleRights(int(ANY_CASTLING) ^  WHITE_OO);
    m[SQ_E8] = CastleRights(int(ANY_CASTLING) ^ (BLACK_OO  | BLACK_OOO));
    m[SQ_A8] = CastleRights(int(ANY_CASTLING) ^  BLACK_OOO);
    m[SQ_H8] = CastleRights(int(ANY_CASTLING) ^  BLACK_OO);
    return m;
}();

namespace Zobrist {
    Key psq[PIECE_NB][SQUARE_NB];
    Key side;
    Key castling[16];
    Key enpassant[SQUARE_NB];

    namespace {
        uint64_t sm_state;
        uint64_t sm_next() {
            uint64_t z = (sm_state += 0x9E3779B97F4A7C15ULL);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            return z ^ (z >> 31);
        }
    }

    void init() {
        sm_state = 0x6C62272E07BB0142ULL;
        for (int p = 0; p < PIECE_NB; ++p)
            for (int s = 0; s < SQUARE_NB; ++s)
                psq[p][s] = sm_next();
        side = sm_next();

        Key base[4];
        for (int i = 0; i < 4; ++i) base[i] = sm_next();
        for (int r = 0; r < 16; ++r) {
            castling[r] = 0;
            if (r & 1) castling[r] ^= base[0];
            if (r & 2) castling[r] ^= base[1];
            if (r & 4) castling[r] ^= base[2];
            if (r & 8) castling[r] ^= base[3];
        }
        for (int s = 0; s < SQUARE_NB; ++s)
            enpassant[s] = sm_next();
    }
}

static Piece char_to_piece(char c) {
    switch (c) {
        case 'P': return W_PAWN;   case 'p': return B_PAWN;
        case 'N': return W_KNIGHT; case 'n': return B_KNIGHT;
        case 'B': return W_BISHOP; case 'b': return B_BISHOP;
        case 'R': return W_ROOK;   case 'r': return B_ROOK;
        case 'Q': return W_QUEEN;  case 'q': return B_QUEEN;
        case 'K': return W_KING;   case 'k': return B_KING;
        default:  return NO_PIECE;
    }
}

static char piece_to_char(Piece p) {
    switch (p) {
        case W_PAWN:   return 'P'; case B_PAWN:   return 'p';
        case W_KNIGHT: return 'N'; case B_KNIGHT: return 'n';
        case W_BISHOP: return 'B'; case B_BISHOP: return 'b';
        case W_ROOK:   return 'R'; case B_ROOK:   return 'r';
        case W_QUEEN:  return 'Q'; case B_QUEEN:  return 'q';
        case W_KING:   return 'K'; case B_KING:   return 'k';
        default:       return '.';
    }
}

static Key compute_key(const Position& pos) {
    Key k = 0;
    Bitboard occ = pos.pieces();
    while (occ) {
        Square s = pop_lsb(occ);
        k ^= Zobrist::psq[pos.piece_on(s)][s];
    }
    if (pos.side_to_move() == BLACK) k ^= Zobrist::side;
    k ^= Zobrist::castling[pos.castling_rights()];

    Square ep = pos.ep_square();
    if (ep != SQ_NONE) {
        Color stm = pos.side_to_move();
        if (pawn_attacks_bb(~stm, ep) & pos.pieces(stm, PAWN))
            k ^= Zobrist::enpassant[ep];
    }
    return k;
}

void Position::clear() {
    byColor[WHITE] = byColor[BLACK] = 0;
    for (int t = 0; t < PIECE_TYPE_NB; ++t) byType[t] = 0;
    for (int s = 0; s < SQUARE_NB; ++s) board[s] = NO_PIECE;
    side_           = WHITE;
    castling_       = NO_CASTLING;
    epSquare_       = SQ_NONE;
    halfmoveClock_  = 0;
    fullmoveNumber_ = 1;
    gamePly_        = 0;
    key_            = 0;
    psqt[0]         = 0; // Reset MG eval
    psqt[1]         = 0; // Reset EG eval
}

// --- Low-level board manipulation: NOW UPDATES PSQT INCREMENTALLY ---
void Position::put_piece(Piece p, Square s) {
    assert(p != NO_PIECE);
    assert(board[int(s)] == NO_PIECE);
    Bitboard bb = square_bb(s);
    byColor[color_of(p)] |= bb;
    byType[type_of(p)]  |= bb;
    byType[0]           |= bb;
    board[int(s)] = int8_t(p);
    
    key_ ^= Zobrist::psq[p][s];

    // Incremental Eval Update
    Color c = color_of(p);
    PieceType pt = type_of(p);
    int sign = (c == WHITE) ? 1 : -1;
    int psq = (c == WHITE) ? int(s) : int(s) ^ 56; // Flip for black
    psqt[0] += sign * (MaterialTable[pt] + PstMg[pt][psq]);
    psqt[1] += sign * (MaterialTable[pt] + PstEg[pt][psq]);
}

void Position::remove_piece(Square s) {
    assert(board[int(s)] != NO_PIECE);
    Piece p = Piece(board[int(s)]);
    Bitboard bb = square_bb(s);
    byColor[color_of(p)] ^= bb;
    byType[type_of(p)]  ^= bb;
    byType[0]           ^= bb;
    board[int(s)] = NO_PIECE;
    
    key_ ^= Zobrist::psq[p][s];

    // Incremental Eval Update
    Color c = color_of(p);
    PieceType pt = type_of(p);
    int sign = (c == WHITE) ? 1 : -1;
    int psq = (c == WHITE) ? int(s) : int(s) ^ 56; // Flip for black
    psqt[0] -= sign * (MaterialTable[pt] + PstMg[pt][psq]);
    psqt[1] -= sign * (MaterialTable[pt] + PstEg[pt][psq]);
}

void Position::move_piece(Square from, Square to) {
    assert(board[int(from)] != NO_PIECE);
    assert(board[int(to)]   == NO_PIECE);
    Piece p = Piece(board[int(from)]);
    Bitboard ft = square_bb(from) | square_bb(to);
    byColor[color_of(p)] ^= ft;
    byType[type_of(p)]  ^= ft;
    byType[0]           ^= ft;
    board[int(from)] = NO_PIECE;
    board[int(to)]   = int8_t(p);
    
    key_ ^= Zobrist::psq[p][from] ^ Zobrist::psq[p][to];

    // Incremental Eval Update (Material doesn't change, only PST)
    Color c = color_of(p);
    PieceType pt = type_of(p);
    int sign = (c == WHITE) ? 1 : -1;
    int psqF = (c == WHITE) ? int(from) : int(from) ^ 56;
    int psqT = (c == WHITE) ? int(to)   : int(to)   ^ 56;
    psqt[0] += sign * (PstMg[pt][psqT] - PstMg[pt][psqF]);
    psqt[1] += sign * (PstEg[pt][psqT] - PstEg[pt][psqF]);
}

// --- FEN Parsing ---
bool Position::set_fen(const std::string& fen) {
    clear();
    std::istringstream iss(fen);
    std::string token;

    if (!(iss >> token)) return false;
    {
        Rank r = RANK_8;
        File f = FILE_A;
        for (char c : token) {
            if (c == '/') {
                r = Rank(int(r) - 1);
                f = FILE_A;
            } else if (c >= '1' && c <= '8') {
                f = File(int(f) + (c - '0'));
            } else {
                Piece p = char_to_piece(c);
                if (p == NO_PIECE) return false;
                if (int(r) < 0 || int(r) > 7 || int(f) < 0 || int(f) > 7) return false;
                put_piece(p, make_square(f, r));
                f = File(int(f) + 1);
            }
        }
    }

    if (!(iss >> token)) return false;
    if (token == "w")      side_ = WHITE;
    else if (token == "b") side_ = BLACK;
    else                   return false;

    if (!(iss >> token)) return false;
    castling_ = NO_CASTLING;
    if (token != "-") {
        for (char c : token) {
            switch (c) {
                case 'K': castling_ |= WHITE_OO;  break;
                case 'Q': castling_ |= WHITE_OOO; break;
                case 'k': castling_ |= BLACK_OO;  break;
                case 'q': castling_ |= BLACK_OOO; break;
                default:  return false;
            }
        }
    }

    if (!(iss >> token)) return false;
    epSquare_ = SQ_NONE;
    if (token != "-") {
        if (token.size() != 2) return false;
        int ff = token[0] - 'a';
        int rr = token[1] - '1';
        if (ff < 0 || ff > 7 || rr < 0 || rr > 7) return false;
        epSquare_ = make_square(File(ff), Rank(rr));
    }

    halfmoveClock_ = 0;
    if (iss >> token) {
        int val = 0;
        auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), val);
        if (ec == std::errc()) halfmoveClock_ = val;
        if (halfmoveClock_ < 0) halfmoveClock_ = 0;
    }

    fullmoveNumber_ = 1;
    if (iss >> token) {
        int val = 1;
        auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), val);
        if (ec == std::errc()) fullmoveNumber_ = val;
        if (fullmoveNumber_ < 1) fullmoveNumber_ = 1;
    }

    gamePly_ = 2 * (fullmoveNumber_ - 1) + (side_ == BLACK ? 1 : 0);

    key_ = compute_key(*this);
    // psqt is already built incrementally by put_piece!
    return true;
}

std::string Position::fen() const {
    std::ostringstream oss;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Square s = Square(r * 8 + f);
            if (board[int(s)] == NO_PIECE) {
                ++empty;
            } else {
                if (empty) { oss << empty; empty = 0; }
                oss << piece_to_char(Piece(board[int(s)]));
            }
        }
        if (empty) oss << empty;
        if (r > 0) oss << '/';
    }

    oss << ' ' << (side_ == WHITE ? 'w' : 'b') << ' ';

    if (castling_ == NO_CASTLING) oss << '-';
    else {
        if (castling_ & WHITE_OO)  oss << 'K';
        if (castling_ & WHITE_OOO) oss << 'Q';
        if (castling_ & BLACK_OO)  oss << 'k';
        if (castling_ & BLACK_OOO) oss << 'q';
    }

    oss << ' ';
    if (epSquare_ == SQ_NONE) {
        oss << '-';
    } else {
        oss << char('a' + int(file_of(epSquare_)));
        oss << char('1' + int(rank_of(epSquare_)));
    }

    oss << ' ' << halfmoveClock_ << ' ' << fullmoveNumber_;
    return oss.str();
}

void Position::make_move(Move m) {
    assert(m != MOVE_NONE);

    Square   from = m.from();
    Square   to   = m.to();
    MoveFlag fl   = m.flag();
    Color    us   = side_;
    Color    them = ~us;
    Piece    pc   = Piece(board[int(from)]);
    PieceType pt  = type_of(pc);
    assert(pc != NO_PIECE);
    assert(color_of(pc) == us);

    if (epSquare_ != SQ_NONE
        && (pawn_attacks_bb(~us, epSquare_) & pieces(us, PAWN)))
    {
        key_ ^= Zobrist::enpassant[epSquare_];
    }

    gamePly_++;
    if (us == BLACK) fullmoveNumber_++;
    bool resets = (pt == PAWN) || is_capture(fl);

    if (is_ep(fl)) {
        Square capSq = Square(int(to) + (us == WHITE ? SOUTH : NORTH));
        assert(piece_on(capSq) == make_piece(them, PAWN));
        remove_piece(capSq);
    } else if (is_capture(fl)) {
        assert(board[int(to)] != NO_PIECE);
        assert(color_of(Piece(board[int(to)])) == them);
        remove_piece(to);
    }
    halfmoveClock_ = resets ? 0 : halfmoveClock_ + 1;

    if (is_promotion(fl)) {
        remove_piece(from);  // remove the pawn
        Piece promo = make_piece(us, promo_type(fl));
        put_piece(promo, to);
    } else {
        move_piece(from, to);
    }

    if (is_castle(fl)) {
        Square rFrom, rTo;
        if (to == SQ_G1)      { rFrom = SQ_H1; rTo = SQ_F1; }
        else if (to == SQ_C1) { rFrom = SQ_A1; rTo = SQ_D1; }
        else if (to == SQ_G8) { rFrom = SQ_H8; rTo = SQ_F8; }
        else                  { rFrom = SQ_A8; rTo = SQ_D8; } 
        move_piece(rFrom, rTo);
    }

    if (fl == FLAG_DOUBLE_PUSH) {
        epSquare_ = Square(int(from) + (us == WHITE ? NORTH : SOUTH));
    } else {
        epSquare_ = SQ_NONE;
    }

    CastleRights newCR = castling_
                       & CastlingRightsMask[int(from)]
                       & CastlingRightsMask[int(to)];

    side_ = them;

    key_ ^= Zobrist::castling[castling_] ^ Zobrist::castling[newCR];
    key_ ^= Zobrist::side;
    castling_ = newCR;

    if (epSquare_ != SQ_NONE
        && (pawn_attacks_bb(~them, epSquare_) & pieces(them, PAWN)))
    {
        key_ ^= Zobrist::enpassant[epSquare_];
    }
}

void Position::do_null_move() {
    assert(!in_check());
    if (epSquare_ != SQ_NONE
        && (pawn_attacks_bb(~side_, epSquare_) & pieces(side_, PAWN)))
    {
        key_ ^= Zobrist::enpassant[epSquare_];
    }
    key_ ^= Zobrist::side;
    epSquare_ = SQ_NONE;
    side_ = ~side_;
    gamePly_++;
}

Bitboard Position::attackers_to(Square s, Color by) const {
    Bitboard occ = pieces();
    return (pawn_attacks_bb(~by, s)    & pieces(by, PAWN))
         | (knight_attacks_bb(s)       & pieces(by, KNIGHT))
         | (king_attacks_bb(s)         & pieces(by, KING))
         | (attacks_bb(BISHOP, s, occ) & (pieces(by, BISHOP) | pieces(by, QUEEN)))
         | (attacks_bb(ROOK,   s, occ) & (pieces(by, ROOK)   | pieces(by, QUEEN)));
}

Bitboard Position::checkers() const {
    return attackers_to(king_square(side_), ~side_);
}

bool Position::in_check(Color c) const {
    return attackers_to(king_square(c), ~c) != 0;
}

} // namespace lofty