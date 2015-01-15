#pragma once

/*
Message types with the naming convention MSG_<group>_<START/END> are never sent;
they're used to verify that a received message is within that group's range.

Example:
When receiving player messages, we check if the type is > MSG_P_START and < MSG_P_END.
*/

enum MsgTypes : unsigned char
{
	MSG_NULL,				// No message
	MSG_VERSION_CHECK,		// Initial version check upon connection
	MSG_VERSION_MISMATCH,	// Client version mismatch
	MSG_VERSION_OK,			// Client version match
	MSG_BIND,				// UDP bind request/confirm
	MSG_DISCONNECT,			// Request disconnect

	MSG_I_START,			// Marker: Start of input messages

	MSG_I_ANALOG,
	MSG_I_BUTTONS,

	MSG_I_END,				// Marker: End of input messages

	MSG_M_START,			// Marker: Start of menu messages

	MSG_M_ALTCHAR,
	MSG_M_BATTLESEL,
	MSG_M_BATTLEOPTSEL,
	MSG_M_CHARCHOSEN,
	MSG_M_CHARSEL,
	MSG_M_STAGESEL,

	MSG_M_END,				// Marker: End of menu messages

	MSG_P_START,			// Marker: Start of player messages

	// CharObj1
	MSG_P_ACTION,
	MSG_P_STATUS,
	MSG_P_ROTATION,
	MSG_P_POSITION,
	MSG_P_SCALE,
	// CharObj2
	MSG_P_CHARACTER,
	MSG_P_POWERUPS,
	MSG_P_UPGRADES,
	MSG_P_HP,
	MSG_P_SPEED,
	MSG_P_ANIMATION,
	// Sonic
	MSG_P_SPINTIMER,

	MSG_P_END,				// Marker: End of player messages

	MSG_S_START,			// Marker: Start of system messages

	MSG_S_2PMODE,
	MSG_S_2PREADY,
	MSG_S_2PSPECIALS,
	MSG_S_BATTLEOPT,
	MSG_S_GAMESTATE,
	MSG_S_LEVEL,
	MSG_S_PAUSESEL,
	MSG_S_RINGS,
	MSG_S_TIME,
	MSG_S_TIMESTOP,

	MSG_S_END,				// Marker: End of system messages

	MSG_COUNT
};
