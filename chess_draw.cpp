


// @Cleanup: Should send in which squares to highlight here, and not a user.

void draw_chess_board(Chess_Board *board, Rect a, Graphics *gfx,
                      User_ID user = NO_USER, s8 selected_square_ix = -1, Chess_Move *queued_move = NULL)
{
    auto draw_mode = current(gfx->draw_mode);
    
    static_assert(ARRLEN(Chess_Board::squares) == 8*8);

    Assert(selected_square_ix < (s8)ARRLEN(Chess_Board::squares));

    
    bool white_king_in_check, black_king_in_check;
    bool check = false;
    
    bool did_check_for_check = false;
    if(draw_mode == DRAW_2D)
    {
        check = is_check(board, &white_king_in_check, &black_king_in_check);
        did_check_for_check = true;
    }

    Rect first_square_a = a;
    first_square_a.s /= 8;

    // SQUARES //
    Rect square_a = first_square_a;
    bool square_is_black = true;
    for(int y = 0; y < 8; y++)
    {
        for(int x = 0; x < 8; x++)
        {    
            v4 square_color;
            if(square_is_black) square_color = { 0.33,0.30,0.29, 1 }; //{ 0.18, 0.11, 0.11, 1 };
            else                square_color = { 0.76, 0.73, 0.65, 1 }; //{ 0.96, 0.93, 0.87, 1 };

            u8 square_ix = (y * 8 + x);
            
            if(selected_square_ix == square_ix) {
                square_color.b += 0.2f;
            }
            
            if(user != NO_USER && 
               (user == board->white_player.user || user == board->black_player.user))
            {
                bool is_black_player = (user == board->black_player.user);
                
                if(selected_square_ix >= 0)
                {
                    Chess_Move move = {0};
                    move.from = selected_square_ix;
                    move.to   = square_ix;
                    if(chess_move_possible(move, is_black_player, board)) // @SpeedMini
                        square_color.g += 0.2f;
                }
            }

            auto piece = extract_chess_piece(board->squares[square_ix]);
            
            if(did_check_for_check && check && piece.type == CHESS_KING)
            {
                if((piece.is_black  && black_king_in_check) ||
                   (!piece.is_black && white_king_in_check))
                {
                    square_color.r += 0.2f;
                }
            }
            
            draw_rect(square_a, square_color, gfx);
            
            square_a.x += square_a.w;
            square_is_black = !square_is_black;
        }
        
        square_a.y += square_a.h;
        square_a.x  = a.x;
        square_is_black = !square_is_black;
    }

    if(queued_move && draw_mode == DRAW_2D)
    {
        u8 from_x = queued_move->from % 8;
        u8 from_y = queued_move->from / 8;
        
        u8 to_x = queued_move->to % 8;
        u8 to_y = queued_move->to / 8;

        v2 p0 = { (float)from_x, (float)from_y };
        v2 p1 = { (float)to_x,   (float)to_y };

        p0 = compmul(p0, first_square_a.s);
        p1 = compmul(p1, first_square_a.s);

        p0 += first_square_a.p + first_square_a.s * 0.5f;
        p1 += first_square_a.p + first_square_a.s * 0.5f;

        draw_line(p0, p1, first_square_a.w * 0.2f, C_RED, gfx);
    }

    // PIECES //
    square_a = first_square_a;
    for(int y = 0; y < 8; y++)
    {
        for(int x = 0; x < 8; x++)
        {
            u8 square_ix = (y * 8 + x);
            auto piece = extract_chess_piece(board->squares[square_ix]);
            
            if(piece.type != CHESS_NONE) {

                const v4 black_piece_color = C_BLACK;
                const v4 white_piece_color = { 0.96, 0.96, 0.96, 1};
                
                v4 piece_color;
                if(piece.is_black) piece_color = black_piece_color;
                else               piece_color = white_piece_color;

                String str = STRING("?");
                float h_factor = 1.0f;
                switch(piece.type) {
                    case CHESS_KING:   str = STRING("K"); break;
                    case CHESS_QUEEN:  str = STRING("Q"); break;
                    case CHESS_ROOK:   str = STRING("R"); h_factor = 0.60f; break;
                    case CHESS_KNIGHT: str = STRING("k"); h_factor = 0.70f; break;
                    case CHESS_BISHOP: str = STRING("B"); h_factor = 0.85f; break;
                    case CHESS_PAWN:   str = STRING("P"); h_factor = 0.55f; break;

                    default: Assert(false); break;
                }

                if(draw_mode == DRAW_2D)
                {
                    Rect piece_a = bottom_of(center_of(square_a, { square_a.w * 0.4f, square_a.h * 0.8f }), square_a.h * 0.8f * h_factor);
                    draw_rect(piece_a, C_BLACK, gfx);
                    draw_rect(shrunken(piece_a, a.w * 0.005f), piece_color, gfx);

                    v4 text_color;
                    if(!piece.is_black) text_color = black_piece_color;
                    else                text_color = white_piece_color;
                
                    draw_string(str, center_of(piece_a), FS_12, FONT_TITLE, text_color, gfx, HA_CENTER, VA_CENTER);
                } else {

                    Rect piece_footprint = center_of(square_a, square_a.s * 0.4f);
                    draw_cube_ps(V3(piece_footprint.p), V3(piece_footprint.s, square_a.h * h_factor * 1.5f), piece_color, gfx);
                    
                }
            }

            square_a.x += square_a.w;
        }
        
        square_a.y += square_a.h;
        square_a.x  = a.x;
    }
        
}
