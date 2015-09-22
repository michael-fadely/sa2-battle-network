#pragma once

/*
	Message types with the naming convention <group>_<START/END> are never sent;
	they're used to verify that a received message is within that group's range.

	Example:
	When receiving player messages, we check if the type is > P_START and < P_END.
*/

namespace nethax
{
	namespace Message
	{
		enum _Message : unsigned char
		{
			None,

			N_START,			// Marker: Start of Networking messages

			N_VersionCheck,		// Initial version check upon connection
			N_VersionMismatch,	// Client version mismatch
			N_VersionOK,		// Client version match
			N_Bind,				// UDP bind request/confirm
			N_Settings,			// Used for synchronizing settings
			N_Connected,		// Connection setup successful
			N_Ready,			// Inidicates stage load completion
			N_Disconnect,		// Request disconnect

			N_END,				// Marker: End of Networking messages
			I_START,			// Marker: Start of input messages

			I_Analog,
			I_Buttons,

			I_END,				// Marker: End of input messages
			M_START,			// Marker: Start of menu messages

			M_AltCharacter,
			M_BattleSelection,
			M_BattleConfigSelection,
			M_CharacterChosen,
			M_CharacterSelection,
			M_StageSelection,

			M_END,				// Marker: End of menu messages
			P_START,			// Marker: Start of player messages

			// CharObj1
			P_Action,
			P_Status,
			P_Rotation,
			P_Position,
			P_Scale,
			// CharObj2
			P_Powerups,
			P_Upgrades,
			P_HP,
			P_Speed,
			P_Animation,
			// Sonic
			P_SpinTimer,

			P_END,				// Marker: End of player messages
			S_START,			// Marker: Start of system messages

			S_KeepAlive,
			S_Stage,
			S_2PReady,
			S_2PSpecials,
			S_BattleOptions,
			S_GameState,
			S_PauseSelection,
			S_Rings,
			S_Time,
			S_TimeStop,

			S_END,				// Marker: End of system messages
			Count
		};
	}
}
