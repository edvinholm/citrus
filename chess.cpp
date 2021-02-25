
// @Norelease: @Incomplete: Check check when castling.
// @Norelease: @Incomplete: Stalemate
// @Norelease: @Incomplete: Insufficient Material
// @Norelease: @Incomplete: Resignation
// @Norelease: @Incomplete: Offer draw, accept draw

// @Norelease: @Incomplete: Pawn promotion: Choice
// @Norelease: @Incomplete: Repeatance stuff (https://en.wikipedia.org/wiki/Rules_of_chess#Draws)
// @Norelease: @Incomplete: Clock, loss when time runs out.


Chess_Square piece_to_square(Chess_Piece piece)
{
    if(piece.type == CHESS_NONE) return 0;
    
    Chess_Square sq = piece.type;
    if(piece.is_black) sq |= CHESS_BLACK;
    else               sq |= CHESS_WHITE;
    
    return sq;
}

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

bool get_chess_piece_at(u8 square_ix, Chess_Board *board, Chess_Piece *_piece)
{
    Assert(square_ix >= 0 && square_ix < ARRLEN(board->squares));

    Chess_Square sq = board->squares[square_ix];
    Chess_Piece  piece = extract_chess_piece(sq);

    if(piece.type == CHESS_NONE) return false;

    *_piece = piece;
    return true;
}


void reset_chess_board(Chess_Board *board)
{
    Zero(*board);
    
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
        
        board->squares[y * 8 + 3] = CHESS_KING   | color;
        board->squares[y * 8 + 4] = CHESS_QUEEN  | color;
        
        board->squares[y * 8 + 5] = CHESS_BISHOP | color;
        board->squares[y * 8 + 6] = CHESS_KNIGHT | color;
        board->squares[y * 8 + 7] = CHESS_ROOK   | color;
    }

    // 
}

// NOTE: start and end squares are not checked.
bool chess_line_blocked(u8 start, s8 dx, s8 dy, Chess_Board *board)
{    
    static_assert(ARRLEN(board->squares) == 8*8);
    const u8 board_sx = 8;
    const u8 board_sy = 8;
    
    s8 dir_x = (dx == 0) ? 0 : ((dx > 0) ? 1 : -1);
    s8 dir_y = (dy == 0) ? 0 : ((dy > 0) ? 1 : -1);

    u8 end = start + (dy * board_sx + dx);
    
    u8 at = start;
    while(true) {
        at += (dir_y * board_sx + dir_x);
        if(at == end) break;

        Assert(at >= 0 && at < ARRLEN(Chess_Board::squares));
                
        if(board->squares[at] != 0) return true;
    }
    
    return false;
}


bool is_check(Chess_Board *board, bool *_white_king_in_check, bool *_black_king_in_check);

void make_chess_move(Chess_Move move, bool as_black_player, Chess_Board *board, bool force = false);


// NOTE: This does not check if it is this player's turn.
// NOTE: *_extra_move is only valid if *_special_move == CHESS_SPECIAL_CASTLE
bool chess_move_possible(Chess_Move move, bool as_black_player, Chess_Board *board, bool is_check_test = false, bool check_for_check = true, Chess_Special_Move *_special_move = NULL, Chess_Move *_extra_move = NULL)
{
    // @Jai: using move
    
    if(_special_move)
        *_special_move = CHESS_SPECIAL_NONE;

    static_assert(ARRLEN(board->squares) == 8*8);
    const u8 board_sx = 8;
    const u8 board_sy = 8;
    
    if(move.from == move.to) return false;

    Chess_Piece piece;
    if(!get_chess_piece_at(move.from, board, &piece)) return false;

    Chess_Player *player = (as_black_player) ? &board->black_player : &board->white_player;

    if (piece.is_black != as_black_player) return false;
    
    if(check_for_check && !is_check_test) { // !is_chess_test here, because "A piece unable to move because it would place its own king in check (it is pinned against its own king) may still deliver check to the opposing player"
        Chess_Board board_copy = *board;
        make_chess_move(move, as_black_player, &board_copy, true);

        bool white_king_in_check;
        bool black_king_in_check;
        if(is_check(&board_copy, &white_king_in_check, &black_king_in_check)) {
            if(piece.is_black  && black_king_in_check) return false;
            if(!piece.is_black && white_king_in_check) return false;
        }
    }

    s8 from_x = move.from % board_sx;
    s8 from_y = move.from / board_sx;
    
    s8 to_x = move.to % board_sx;
    s8 to_y = move.to / board_sx;

    s8 dx = (to_x - from_x);
    s8 dy = (to_y - from_y);

    
    u8 bottom_y = (piece.is_black) ? 0 : 7;
    u8 top_y    = (piece.is_black) ? 7 : 0;
    u8 left_x   = (piece.is_black) ? 0 : 7;
    u8 right_x  = (piece.is_black) ? 7 : 0;
    
    switch(piece.type)
    {
        case CHESS_KING:
        {
            if(abs(dx) > 1 || abs(dy) > 1) {

                if(dy == 0 && !(player->flags & KING_MOVED)) {
                    
                    // Castling
                    bool is_right;

                    // @Incomplete: Check if we're in check now.
                    // @Incomplete: Check if king would pass or end up in a square that is threatened by a piece of the opponent's.
                    
                    if     (dx ==  2) is_right =  piece.is_black;
                    else if(dx == -2) is_right = !piece.is_black;
                    else return false;

                    s8 rook_x;
                    
                    if     ( is_right && !(player->flags & BOTTOM_RIGHT_MODIFIED)) rook_x = right_x;
                    else if(!is_right && !(player->flags & BOTTOM_LEFT_MODIFIED))  rook_x = left_x;
                    else return false;

                    if(chess_line_blocked(move.from, rook_x - from_x, 0, board)) return false;

                    if(_extra_move) {
                        Assert(abs(dx) == 2);
                        
                        Zero(*_extra_move);
                        _extra_move->from = (from_y * 8 + rook_x);
                        _extra_move->to   = (move.from + dx/2);
                    }

                    if(_special_move) *_special_move = CHESS_SPECIAL_CASTLE;

                    return true;
                }
                
                return false;
            }
            
        } break;

        case CHESS_QUEEN:
            if(abs(dx) != abs(dy) && dx != 0 && dy != 0) return false;
            break;

        case CHESS_ROOK:
            if(dx != 0 && dy != 0) return false;
            break;

        case CHESS_KNIGHT: {
            bool can_move = ((abs(dx) == 2 && abs(dy) == 1) ||
                             (abs(dy) == 2 && abs(dx) == 1));
            
            if(!can_move) return false;
            
            Chess_Piece piece_at_dest;
            if(!get_chess_piece_at(move.to, board, &piece_at_dest)) return true;
                
            return (piece_at_dest.is_black != piece.is_black);
            
        } break;

        case CHESS_BISHOP:
            if(abs(dx) != abs(dy)) return false;
            break;
        
        case CHESS_PAWN: 
        {
            s8 forward_dy = (piece.is_black) ? 1 : -1;
            u8 fwd1 = move.from + forward_dy * 1 * board_sx;
            u8 fwd2 = move.from + forward_dy * 2 * board_sx;
            
            if(move.to == fwd1 || move.to == fwd2)
            {
                if(is_check_test) return false;
                
                if(move.to == fwd2) {
                    if      (piece.is_black  && from_y != 1)   return false;
                    else if (!piece.is_black && from_y != 8-2) return false;

                    if(_special_move) *_special_move = CHESS_SPECIAL_TWO_SQUARE_PAWN;
                }
                
                // Straight forward (Can't capture)
                if(move.to == fwd2 && board->squares[fwd1] != 0) return false;
                return (board->squares[move.to] == 0);
            }
            else if((from_x < 7 && move.to == fwd1 + 1) || (from_x > 0 && move.to == fwd1 - 1))
            {
                // Diagonal capture
                Chess_Piece piece_at_dest;
                if(!get_chess_piece_at(move.to, board, &piece_at_dest) && !is_check_test) {

                    if(is_check_test) return false;
                    
                    if(board->num_moves > 0 &&
                       board->last_move_special == CHESS_SPECIAL_TWO_SQUARE_PAWN)
                    {
                        // En passant
                        u8 last_to = board->last_move.to;
                        if(last_to == move.to - (forward_dy * board_sx))
                        {
                            // NOTE: We know that it is our opponent's piece in last_to,
                            //       since the last move cannot have been made by us.
                            if(_special_move) *_special_move = CHESS_SPECIAL_EN_PASSANT;
                            return true;
                        }
                    }
                       
                    return false;
                }
                
                return (is_check_test || piece_at_dest.is_black != piece.is_black);
            }

            return false;
            
        } break;

        default: Assert(false); return false;
    }

    // NOTE: For knights, pawns and castling we (should) always return before we get here.
    
    
    if(chess_line_blocked(move.from, dx, dy, board)) return false;

    Chess_Piece piece_at_dest;
    if(!get_chess_piece_at(move.to, board, &piece_at_dest)) return true;

    return (is_check_test || piece_at_dest.is_black != piece.is_black);

}

// IMPORTANT: @Incomplete: This does not take En Passant into account!
bool is_check(Chess_Board *board, bool *_white_king_in_check, bool *_black_king_in_check)
{
    *_white_king_in_check = false;
    *_black_king_in_check = false;

    bool check = false;
    
    for(u8 j = 0; j < ARRLEN(board->squares); j++)
    {
        Chess_Piece king;
        if(!get_chess_piece_at(j, board, &king)) continue;
        if(king.type != CHESS_KING) continue;
        
        for(u8 i = 0; i < ARRLEN(board->squares); i++)
        {
            Chess_Piece piece;
            if(!get_chess_piece_at(i, board, &piece)) continue;
            if(piece.is_black == king.is_black) continue;

            Chess_Move move = {0};
            move.from = i;
            move.to   = j;
            if(chess_move_possible(move, piece.is_black, board, true))
            {
                if(king.is_black) *_black_king_in_check = true;
                else              *_white_king_in_check = true;
                check = true;
            }
        }
    }
    
    return check;
}

void make_chess_move(Chess_Move move, bool as_black_player, Chess_Board *board, bool force/* = false*/)
{
    bool check_for_check = !force;
    
    Chess_Special_Move special_move;
    Chess_Move extra_move;
    bool possible = chess_move_possible(move, as_black_player, board, false, check_for_check, &special_move, &extra_move);
    Assert(force || possible);

    Assert(force || board->black_players_turn == as_black_player);
    Chess_Player *player       = (board->black_players_turn)  ? &board->black_player : &board->white_player;    
    Chess_Player *other_player = (!board->black_players_turn) ? &board->black_player : &board->white_player;
    
    u8 to_x = move.to % 8;
    u8 to_y = move.to / 8;

    u8 from_x = move.from % 8;
    u8 from_y = move.from / 8;
    
    Chess_Square *sq0 = &board->squares[move.from];
    Chess_Square *sq1 = &board->squares[move.to];

    Chess_Piece moved_piece = extract_chess_piece(*sq0);
    Assert(moved_piece.type != CHESS_NONE);

    u8 bottom_y = (moved_piece.is_black) ? 0 : 7;
    u8 top_y    = (moved_piece.is_black) ? 7 : 0;
    u8 left_x   = (moved_piece.is_black) ? 0 : 7;
    u8 right_x  = (moved_piece.is_black) ? 7 : 0;
    
    *sq1 = *sq0;
    *sq0 = 0;

    if(special_move == CHESS_SPECIAL_CASTLE)
    {
        board->squares[extra_move.to]   = board->squares[extra_move.from];
        board->squares[extra_move.from] = 0;
    }
    
    if(special_move == CHESS_SPECIAL_EN_PASSANT)
    {
        s8 dx = to_x - from_x;
        s8 dy = to_y - from_y;
        Assert(abs(dy) == 1 && abs(dx) == 1);

        u8 capture_square_ix = move.from + dx;
        Assert(capture_square_ix >= 0 && capture_square_ix < 8*8);
        
        Chess_Square *capture_sq = &board->squares[capture_square_ix];
        *capture_sq = 0;
    }

    if(moved_piece.type == CHESS_KING) {
        player->flags |= KING_MOVED;
    }

    // This player
    if((from_y == bottom_y && from_x == left_x) ||
       (to_y   == bottom_y && to_x   == left_x)) player->flags |= BOTTOM_LEFT_MODIFIED;
       
    if((from_y == bottom_y && from_x == right_x) ||
       (to_y   == bottom_y && to_x   == right_x)) player->flags |= BOTTOM_RIGHT_MODIFIED;
    // --

    // Other player
    if((from_y == top_y && from_x == right_x) ||
       (to_y   == top_y && to_x   == right_x)) other_player->flags |= BOTTOM_LEFT_MODIFIED;
       
    if((from_y == top_y && from_x == left_x) ||
       (to_y   == top_y && to_x   == left_x)) other_player->flags |= BOTTOM_RIGHT_MODIFIED;
    // --

    
    board->last_move         = move;
    board->last_move_special = special_move;
    board->num_moves++;

    board->black_players_turn = !board->black_players_turn;

    if(moved_piece.type == CHESS_PAWN) {
        if(to_y == top_y) {
            Chess_Piece subst_piece = moved_piece;
            subst_piece.type = CHESS_QUEEN; // @Incomplete: Let player choose type of piece.
            *sq1 = piece_to_square(subst_piece);
        }
    }



    // CHECK FOR CHECK MATE //
    
    if (!force) // @Cleanup: force maybe should be named something like 'test_mode'
    {
        bool white_king_in_check, black_king_in_check;
        if (is_check(board, &white_king_in_check, &black_king_in_check))
        {
            bool check_mate = true;

            Assert(white_king_in_check != black_king_in_check); // Only one king can be in checked at a time!

            for (u8 i = 0; i < ARRLEN(board->squares); i++)
            {
                Chess_Piece piece;
                if (!get_chess_piece_at(i, board, &piece)) continue;
                if (piece.is_black != board->black_players_turn) continue; // IMPORTANT that this check is after we toggle board->black_players_turn.

                for (u8 j = 0; j < ARRLEN(board->squares); j++)
                {
                    Chess_Move test_move = { 0 };
                    test_move.from = i;
                    test_move.to = j;
                    if (chess_move_possible(test_move, piece.is_black, board)) {
                        check_mate = false;
                        break;
                    }
                }
            }

            if(check_mate) {
                // @Incomplete: What happens now? :)
                // Debug_Print("CHESS: Check mate!\n");
            }
        }
    }

    
    // @Norelease @Incomplete: Check stuff.
}



bool chess_move_possible_for_user(Chess_Move move, User_ID user, Chess_Board *board, bool *_would_be_possible_if_their_turn = NULL)
{
    if(_would_be_possible_if_their_turn) *_would_be_possible_if_their_turn = false;
    
    bool is_black_player;
    if     (board->black_player.user == user) is_black_player = true;
    else if(board->white_player.user == user) is_black_player = false;
    else return false;

    bool possible = chess_move_possible(move, is_black_player, board);
    
    if(board->black_players_turn != is_black_player) {
        if(_would_be_possible_if_their_turn) *_would_be_possible_if_their_turn = possible;
        return false;
    }
    
    return possible;
}
    
bool chess_action_possible(Chess_Action *action, User_ID user, Chess_Board *board)
{
    Assert(user != NO_USER);
    
    switch(action->type) {
        case CHESS_ACT_MOVE:           
            return chess_move_possible_for_user(action->move, user, board);

        case CHESS_ACT_JOIN: {
            auto *join = &action->join;
            
            auto *target_cp =  (join->as_black) ? &board->black_player : &board->white_player;
            auto *other_cp  = (!join->as_black) ? &board->black_player : &board->white_player;
            
            return (target_cp->user == NO_USER && other_cp->user != user);
        }

        default: Assert(false); return false;
    }
}

bool perform_chess_action_if_possible(Chess_Action *action, User_ID user, Chess_Board *board)
{
    Assert(user != NO_USER);
    
    if(!chess_action_possible(action, user, board)) return false;

    switch(action->type) {
        case CHESS_ACT_MOVE: {

            bool is_black_player;
            if     (board->black_player.user == user) is_black_player = true;
            else if(board->white_player.user == user) is_black_player = false;
            else { Assert(false); return false; }
            
            make_chess_move(action->move, is_black_player, board);
               
        } break;

        case CHESS_ACT_JOIN: {
            auto *join = &action->join;

            auto *cp = (join->as_black) ? &board->black_player : &board->white_player;

            if(cp->user == NO_USER) {
                cp->user = user;
                return true;
            }
            
            Assert(false);
            return false;
            
        } break;

        default: Assert(false); return false;
    }
    
    return true;
}
            
