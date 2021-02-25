
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

struct Chess_Move
{
    u8 from; // square index
    u8 to;

    bool operator == (Chess_Move &b) { // NOTE: This is only used by ui_set() as of 2021-02-17.
        if(this->from != b.from) return false;
        if(this->to   != b.to)   return false;
        return true;
    }
};

enum Chess_Special_Move
{
    CHESS_SPECIAL_NONE,
    CHESS_SPECIAL_TWO_SQUARE_PAWN,
    CHESS_SPECIAL_EN_PASSANT,
    CHESS_SPECIAL_CASTLE
};

enum Chess_Player_Flag_
{
    KING_MOVED           = 0x01,

    BOTTOM_LEFT_MODIFIED  = 0x02,
    BOTTOM_RIGHT_MODIFIED = 0x04
};

typedef u8 Chess_Player_Flags;

struct Chess_Player
{
    User_ID user;
    Chess_Player_Flags flags;
};

struct Chess_Board
{
    Chess_Player white_player;
    Chess_Player black_player;

    bool black_players_turn;

    u32 num_moves;
    Chess_Move         last_move;
    Chess_Special_Move last_move_special;
    
    Chess_Square squares[8*8];
};

struct Chess_Piece
{
    Chess_Piece_Type type;
    bool is_black; // Otherwise white
};

enum Chess_Action_Type
{
    CHESS_ACT_MOVE = 1,
    CHESS_ACT_JOIN = 2
};

struct Chess_Action
{
    Chess_Action_Type type;
    
    union {
        Chess_Move move;

        struct {
            bool as_black;
        } join;
    };
};
