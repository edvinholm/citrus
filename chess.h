
enum Chess_Piece_Type: u8
{
    CHESS_NONE = 0x00,
    
    CHESS_KING   = 0x01,
    CHESS_QUEEN  = 0x02,
    CHESS_ROOK   = 0x03,
    CHESS_KNIGHT = 0x04,
    CHESS_BISHOP = 0x05,
    CHESS_PAWN   = 0x06,

    NUM_CHESS_PIECE_TYPES_PLUS_NONE
};

const u8 CHESS_WHITE = 0x80; // MSB   set.
const u8 CHESS_BLACK = 0x40; // MSB-1 set.

// Empty square = CHESS_PIECE_NONE.
// White rook   = CHESS_ROOK | CHESS_WHITE.
// Black king   = CHESS_KING | CHESS_BLACK.
// CHESS_BISHOP | 0, for example, is not a valid value for a square on a valid board.
typedef u8 Chess_Square;

struct Chess_Board
{
    Chess_Square squares[8*8];
};

struct Chess_Piece
{
    Chess_Piece_Type type;
    bool is_black; // Otherwise white
};

