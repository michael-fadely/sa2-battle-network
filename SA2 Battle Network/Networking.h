#pragma once

enum MsgTypes : unsigned char
{
	MSG_NULL,

	MSG_I_ANALOG,
	MSG_I_BUTTONS,

	MSG_M_ALTCHAR,
	MSG_M_ATMENU,
	MSG_M_BATTLEMODESEL,
	MSG_M_BATTLEOPTSEL,
	MSG_M_CHARCHOSEN,
	MSG_M_CHARSEL,
	MSG_M_STAGESEL,

	MSG_P_ACTION,
	MSG_P_ANIMATION,
	MSG_P_CHARACTER,
	MSG_P_HP,
	MSG_P_POSITION,
	MSG_P_POWERUPS,
	MSG_P_RINGS,
	MSG_P_ROTATION,
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
	MSG_S_TIME,
	MSG_S_TIMESTOP,

	RELIABLE_RECV = 50,
	RELIABLE_SEND,
	MSG_ESTABLISH,
	MSG_KEEPALIVE,
	MSG_SCREWYOU,
	MSG_DISCONNECT,
	MSG_COUNT
};

struct clientAddress
{
	std::string address;
	ushort port;
};