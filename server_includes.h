
/*
  SERVER INCLUDES
*/

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//--

#include "misc.cpp"


#include "random.cpp"

#include "game_shared_pre.h"

#include "pathfinding.h"

#include "user_pre.h"

//--
#include "chess.h"
//--

#include "recipe.h"

#include "game_shared.h"
#include "game_server.h"
using namespace Server_Game;

#include "world_shared.h"

#include "user.h"
using namespace Server_User;

#include "market.h"
using namespace Server_Market;

//--
#include "chess.cpp"
//--

#include "user_shared.cpp"
#include "world_shared.cpp"
#include "pathfinding_shared.cpp"

#include "recipe.cpp"

#include "log.cpp"

#include "transaction.h"

#include "net_server.h"

#include "network.cpp"
#include "market_client.cpp"
#include "user_client.cpp"

#include "pathfinding.cpp"

#include "market_server.h"
#include "room_server.h"
#include "user_server.h"

#include "server.h"

#include "net_server.cpp"

#include "machines.cpp"

#include "market_server.cpp"
#include "room_server.cpp"
#include "user_server.cpp"
