


// IMPORTANT: squares must be at least ARRLEN(Chess_Board::squares) long.
void draw_chess_board(Chess_Square *squares, Rect a, Graphics *gfx, s8 highlighted_square_ix = -1)
{
    auto draw_mode = current(gfx->draw_mode);
    
    static_assert(ARRLEN(Chess_Board::squares) == 8*8);

    Rect square_a = a; square_a.s /= 8;
    bool square_is_black = false; // @Norelease: This depends on if you're playing white or black.... (And the order in which we draw the squares also depends on it...)
    for(int y = 0; y < 8; y++)
    {
        for(int x = 0; x < 8; x++)
        {    
            v4 square_color;
            if(square_is_black) square_color = { 0.33,0.30,0.29, 1 }; //{ 0.18, 0.11, 0.11, 1 };
            else                square_color = { 0.76, 0.73, 0.65, 1 }; //{ 0.96, 0.93, 0.87, 1 };

            if(highlighted_square_ix == (y * 8 + x)) {
                square_color.r += 0.2f;
            }
            
            draw_rect(square_a, square_color, gfx);

            auto piece = extract_chess_piece(squares[y * 8 + x]);
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
            square_is_black = !square_is_black;
        }
        
        square_a.y += square_a.h;
        square_a.x  = a.x;
        square_is_black = !square_is_black;
    }
        
}
