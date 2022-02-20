


s64 sprite_editor_coordinate_control(UI_Context ctx, String label, s64 value, Input_Manager *input)
{
    U(ctx);

    _AREA_COPY_(); // This is stupid. We should be able to do _LEFT_CUT_ in cells. @Cleanup

    { _LEFT_CUT_(20);
        ui_text(P(ctx), label, FS_14, FONT_BODY, HA_RIGHT, VA_CENTER, opt(C_WHITE));
    }

    return textfield_s64(P(ctx), value, input, true);
}

void sprite_editor(UI_Context ctx, Input_Manager *input, Sprite_Editor *editor)
{   
    U(ctx);

    // EXIT //
    if(in_array(input->keys_down, VKEY_ESCAPE) &&
       in_array(input->keys,      VKEY_SHIFT))
    {
        editor->open = false;
        return;
    }

    // SCALE //
    if(is_zero(editor->scale))
        editor->scale = 1;
    
    if(in_array(input->key_hits, VKEY_ADD)) {
        editor->scale *= 2;
    }

    if(in_array(input->key_hits, VKEY_SUBTRACT)) {
        editor->scale /= 2;
    }

    editor->scale = clamp(editor->scale, 0.125f, 32.0f);
    
    // LEFT BAR //
    { _LEFT_CUT_(128);

        { _AREA_COPY_();

            // UNDO, REDO
            { _TOP_CUT_(32);
                { _LEFT_HALF_();
                    button(P(ctx), STRING("UNDO"));
                }
                { _RIGHT_HALF_();
                    button(P(ctx), STRING("REDO"));
                }
            }
            
            // SAVE BUTTON
            { _BOTTOM_CUT_(32);
                button(P(ctx), STRING("SAVE ALL"));
            }

            { _BOTTOM_CUT_(8); } // Spacing

            // BACKGROUND COLOR SELECTION
            int   color_cols = 4;
            float color_s    = area(ctx.layout).w/color_cols;
            auto  num_colors = ARRLEN(sprite_editor_background_colors);
            int   color_rows = ceil((float)num_colors / color_cols);
            { _BOTTOM_CUT_(color_s * color_rows);
                _GRID_(color_cols, color_rows, 0);
                for(int i = 0; i < num_colors; i++) {
                    _CELL_();
                    v4 c = sprite_editor_background_colors[i];
                    if(button(PC(ctx, i), EMPTY_STRING, true, false, opt<v4>(c)) & CLICKED_ENABLED) {
                        editor->background_color_index = i;
                    }
                }
            }
            { _BOTTOM_CUT_(16);
                ui_text(P(ctx), STRING("BG COLOR:"), FS_14, FONT_TITLE, HA_CENTER);
            }
        
            { _BOTTOM_CUT_(8); } // Spacing
        }

        panel(P(ctx), opt(C_GRAY));
    }

    // RIGHT BAR //
    { _RIGHT_CUT_(128);

        { _AREA_COPY_();
            // SELECTED SPRITE OPTIONS
            {
                
                // MOVE TO
                { _BOTTOM_CUT_(32);
                    button(P(ctx), STRING("MOVE TO"));
                }
                
                { _BOTTOM_CUT_(8); } // Spacing

                bool has_slicing = sprites[editor->selected_sprite].has_slicing;

                if(has_slicing) {
                
                    { _BOTTOM_CUT_(48);
                        ui_text(P(ctx), STRING("Coordinates are relative to the sprite's origin"), HA_CENTER);
                    }
                
                    // SLICE COORDINATES
                    { _BOTTOM_CUT_(64);
                        _GRID_(2, 2, 8);
                        { _CELL_(); sprite_editor_coordinate_control(P(ctx), STRING("x:"), 0, input); }
                        { _CELL_(); sprite_editor_coordinate_control(P(ctx), STRING("y:"), 0, input); }
                        { _CELL_(); sprite_editor_coordinate_control(P(ctx), STRING("w:"), 0, input); }
                        { _CELL_(); sprite_editor_coordinate_control(P(ctx), STRING("h:"), 0, input); }
                    }
                }
            
                // SLICING ENABLED/DISABLED
                { _BOTTOM_CUT_(32);
                    String slicing_status_text = concat_tmp("SLICING ", (has_slicing) ? "ENABLED" : "DISABLED");
                    // NOTE: This button should toggle slicing enabled/disabled.
                    button(P(ctx), slicing_status_text);
                }

                { _BOTTOM_CUT_(8); } // Spacing
                
                // EXPAND/TRIM
                { _BOTTOM_CUT_(32);
                    { _LEFT_HALF_();
                        button(P(ctx), STRING("EXPAND"));
                    }
                    { _RIGHT_HALF_();
                        button(P(ctx), STRING("TRIM"));
                    }
                }
            
                // FRAME COORDINATES
                { _BOTTOM_CUT_(64);
                    _GRID_(2, 2, 8);
                    { _CELL_(); sprite_editor_coordinate_control(P(ctx), STRING("x:"), 0, input); }
                    { _CELL_(); sprite_editor_coordinate_control(P(ctx), STRING("y:"), 0, input); }
                    { _CELL_(); sprite_editor_coordinate_control(P(ctx), STRING("w:"), 0, input); }
                    { _CELL_(); sprite_editor_coordinate_control(P(ctx), STRING("h:"), 0, input); }
                }
            
                { _BOTTOM_CUT_(8); } // Spacing

                // TEXTURE SELECTION
                { _BOTTOM_CUT_(32);
                    button(P(ctx), STRING("<TEXTURE NAME>"));
                }
                
                { _BOTTOM_CUT_(8); } // Spacing

                // NAME
                { _BOTTOM_CUT_(28);
                    bool text_did_change;
                    textfield_tmp(P(ctx), sprite_names[editor->selected_sprite], input, &text_did_change, true);
                }
            }
        
            // SPRITE LIST
            { _VIEWPORT_(P(ctx)); // TODO: Make the viewport scrollable.   
                for(int i = 0; i < SPRITE_NONE_OR_NUM; i++) {
                    _TOP_SLIDE_(16);

                    Sprite_ID sprite = (Sprite_ID)i;
                
                    bool selected = (editor->selected_sprite == sprite);
                    if(button(PC(ctx, i), sprite_names[i], true, selected) & CLICKED_ENABLED) {
                        editor->selected_sprite = sprite;
                    }
                }
            }
        }
            
        
        panel(P(ctx), opt(C_GRAY));
    }


    // BOTTOM BAR //
    { _BOTTOM_CUT_(18);
        { _SHRINK_(8, 8, 0, 0);
            String info = concat_tmp("SHIFT-ESCAPE TO EXIT | SCALE: ", editor->scale * 100.0f, "%");
            ui_text(P(ctx), info, FS_14, FONT_BODY, HA_LEFT, VA_CENTER, opt(C_WHITE));
        }
        panel(P(ctx), opt<v4>({0.2, 0.2, 0.2, 1}));
    }


    { _VIEWPORT_(P(ctx));
        // TEXTURE //
        Rect tex_a = area(ctx.layout);
        tex_a.w = tex_a.h; // Assuming the texture is 1:1
        tex_a.s *= editor->scale;
        { _AREA_(tex_a);
            ui_texture_image(P(ctx), TEX_PREVIEWS);
        }
    }
    
    panel(P(ctx), opt(sprite_editor_background_colors[editor->background_color_index]));
}



