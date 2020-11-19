

/*
  CLIENT INCLUDES
*/

#include "tweaks_client.cpp"

#define STB_TRUETYPE_IMPLEMENTATION
//#define STBTT_RASTERIZER_VERSION 1
#include "stb_truetype.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// --

#include "random.cpp"

#include "unicode_db.h"
#include "ucd.gen.cpp"
#include "unicode.cpp"

// --

#include "direction.h"

#include "texture.h"

#include "sprite.h"
#include "font.h"
#include "graphics.h"

#include "texture.cpp"

#include "graphics.cpp"

#include "bitmap.cpp"
#include "sprite.cpp"
#include "font.cpp"

#include "layout.h"
#include "layout.cpp"

#include "input.cpp"

#include "ui.h"

//--

// IMPORTANT: There is one definition for this for the client, and another for the server.
#define Fail_If_True(Condition) \
    if(Condition) { Debug_Print("[FAILURE] Condition met: "); Debug_Print(#Condition); Debug_Print("\n"); return false; }

#include "game_shared.h"
#include "game_client.h"
using namespace Client_Game;

#include "packets.cpp"
#include "net_client.h"
#include "client.h"
#include "net_client.cpp"

//--


#include "body_text.h"

#include "ui.cpp"

#include "draw.cpp"

#include "sprite_draw.cpp"
#include "font_draw.cpp"

#include "body_text.cpp"

#include "draw_client.cpp"
