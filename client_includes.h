

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

#include "misc.cpp"

// --

#include "obj.cpp"

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

#include "world.h"

//--

#include "game_shared.h"
#include "user.h"

#include "transaction.h"

#include "network.cpp"
#include "room_client.cpp"
#include "user_client.cpp"

#include "net_client.h"


#include "ui.h"


#include "game_client.h"
using namespace Client_Game;

using namespace Client_User;

#include "client.h"


#include "world.cpp"

#include "net_client.cpp"

#include "body_text.h"

#include "ui.cpp"

#if DEVELOPER
#include "ui_dev_client.cpp"
#endif

#include "draw.cpp"

#include "sprite_draw.cpp"
#include "font_draw.cpp"

#include "body_text.cpp"

#include "world_draw.cpp"
#include "draw_client.cpp"
