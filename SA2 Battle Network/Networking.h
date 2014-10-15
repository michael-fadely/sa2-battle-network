#pragma once

enum MsgTypes : unsigned char
{
	MSG_NULL,				// No message
	MSG_VERSION_CHECK,		// Initial version check upon connection
	MSG_VERSION_MISMATCH,	// Client version mismatch
	MSG_VERSION_OK,			// Client version match
	MSG_BIND,				// UDP bind request/confirm
	MSG_DISCONNECT,			// Request disconnect

	MSG_I_ANALOG,
	MSG_I_BUTTONS,

	MSG_M_ALTCHAR,
	MSG_M_ATMENU,	// Deprecated
	MSG_M_BATTLESEL,
	MSG_M_BATTLEOPTSEL,
	MSG_M_CHARCHOSEN,
	MSG_M_CHARSEL,
	MSG_M_STAGESEL,

	// TODO: Re-organize these based on actual order

	MSG_P_ACTION,
	MSG_P_ANIMATION,
	MSG_P_CHARACTER,
	MSG_P_HP,
	MSG_P_POSITION,
	MSG_P_POWERUPS,
	MSG_P_ROTATION,
	MSG_P_SCALE,
	MSG_P_SPEED,
	MSG_P_SPINTIMER,
	MSG_P_STATUS,
	MSG_P_UPGRADES,

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

	MSG_COUNT
};
