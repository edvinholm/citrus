


Chess_Piece extract_chess_piece(Chess_Square square)
{
    Chess_Piece piece;
    Zero(piece);
    
    if(square == 0) {
        static_assert(CHESS_NONE == 0); // So we don't need to set piece.type here, since piece is zeroed.
        return piece;
    }

    if(square & CHESS_BLACK) {
        Assert(!(square & CHESS_WHITE));
        piece.is_black = true;
    }
    else {
        Assert(square & CHESS_WHITE);
        Assert(!piece.is_black); // Since piece is zeroed.
    }

    static_assert(sizeof(Chess_Square)      == 1);
    static_assert(sizeof(Chess_Piece_Type)  == 1);
    
    piece.type = (Chess_Piece_Type)(square & ~(CHESS_BLACK | CHESS_WHITE));
    Assert(piece.type > 0 && piece.type < NUM_CHESS_PIECE_TYPES_PLUS_NONE);

    return piece;
}


void reset_chess_board(Chess_Board *board)
{
    Zero(board->squares);
    
    for(int c = 0; c < 2; c++)
    {
        Chess_Square color = (c == 0) ? CHESS_WHITE : CHESS_BLACK;

        // Pawns
        {
            Chess_Square pawn = CHESS_PAWN | color;
            int y = (c == 0) ? 8-2 : 1;
            for(int x = 0; x < 8; x++)
            {
                auto *square = &board->squares[y * 8 + x];
                Assert(*square == 0);
                
                *square = pawn;
            }
        }

        int y = (c == 0) ? 8-1 : 0;
        board->squares[y * 8 + 0] = CHESS_ROOK   | color;
        board->squares[y * 8 + 1] = CHESS_KNIGHT | color;
        board->squares[y * 8 + 2] = CHESS_BISHOP | color;

        if(c == 0) {
            board->squares[y * 8 + 3] = CHESS_KING  | color;
            board->squares[y * 8 + 4] = CHESS_QUEEN | color;
        } else {
            board->squares[y * 8 + 3] = CHESS_QUEEN | color;
            board->squares[y * 8 + 4] = CHESS_KING  | color;
        }
        
        board->squares[y * 8 + 5] = CHESS_BISHOP | color;
        board->squares[y * 8 + 6] = CHESS_KNIGHT | color;
        board->squares[y * 8 + 7] = CHESS_ROOK   | color;
    }

    // 
}


bool chess_move_possible(Chess_Board *board, u8 from, u8 to)
{
    // @Norelease @Incomplete: Take a parameter that specifies which player this is about.
    // @Norelease @Incomplete: Check if move is possible.
    
    return true;
}

void make_chess_move(Chess_Board *board, u8 from, u8 to)
{
    Assert(chess_move_possible(board, from, to));

    Chess_Square *sq0 = &board->squares[from];
    Chess_Square *sq1 = &board->squares[to];

    *sq1 = *sq0;
    *sq0 = 0;

    // @Norelease @Incomplete: Check stuff.
}


    
