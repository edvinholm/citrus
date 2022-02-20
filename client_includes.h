

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

#include "mesh.h"
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


#include "user_pre.h"

// --

#include "chess.h"
#include "chess.cpp"

// --

#include "game_shared_pre.h"

#include "pathfinding.h"

#include "color.h"

#include "recipe.h"

#include "game_shared.h"
#include "world_shared.h"
#include "user.h"
#include "market.h"

#include "recipe.cpp"

#include "transaction.h"

#include "network.cpp"
#include "room_client.cpp"
#include "user_client.cpp"
#include "market_client.cpp"

#include "net_client.h"

#include "ui.h"


#include "game_client.h"
using namespace Client_Game;

using namespace Client_User;
using namespace Client_Market;

#include "user_shared.cpp"

#include "plant.cpp"
#include "world_shared.cpp"


#include "pathfinding_shared.cpp"


#include "machine_shared.cpp"

#include "view.h"
#include "ui_dock.h"

#include "sprite_editor.h"
#include "ui_dev_client.h"

#include "assets.cpp"


#include "user_utilities.cpp"
#include "tool.h"

#if DEVELOPER
#include "room_editor.h"
#endif

#include "client.h"


#include "world.cpp"

#include "net_client.cpp"

#include "body_text.h"

#include "color.cpp"

#include "animation.cpp"

#include "draw.cpp"

#include "sprite_draw.cpp"
#include "font_draw.cpp"

#include "body_text.cpp"

#include "chess_draw.cpp"
#include "world_draw.cpp"

#if DEBUG
#include "profile_draw.cpp"
#endif

#include "command.cpp"

#include "ui.cpp"
#include "view.cpp"
#include "ui_dock.cpp"

#if DEVELOPER
#include "room_editor.cpp"
#include "sprite_editor.cpp"
#endif


#if DEVELOPER
#include "ui_dev_client.cpp"
#endif

#include "client_draw.h"
#include "previews.cpp"
#include "client_draw.cpp"


#include "tool.cpp"
