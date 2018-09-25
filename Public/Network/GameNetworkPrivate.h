//$ Copyright 2015-18 Code Respawn Pvt Ltd, All rights reserved $//

#pragma once

#include "CoreMinimal.h"


/** Defines the desired size of socket send buffers (in bytes). */
#define GAMENETWORK_SEND_BUFFER_SIZE 2 * 1024 * 1024

/** Defines the desired size of socket receive buffers (in bytes). */
#define GAMENETWORK_RECEIVE_BUFFER_SIZE 2 * 1024 * 1024

/** Defines a magic number for the the game network message transport. */
#define GAMENETWORK_TRANSPORT_PROTOCOL_MAGIC 0x82F2C74F

/** Defines the protocol version of the game network message transport. */
namespace EGameNetworkVersion
{
	enum Type
	{
		Initial,

		// -----<new versions can be added before this line>-------------------------------------------------
		// - this needs to be the last line (see note below)
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1,
		// bump this when break need to break compatibility.
		OldestSupportedVersion = Initial
	};
}

