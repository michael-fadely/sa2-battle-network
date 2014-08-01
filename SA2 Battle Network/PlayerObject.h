#pragma once

#include "GameObject.h"

enum class CharacterID : unsigned char
{
	SonicAmy,
	ShadowMetal,
	MechlessTails,
	MechlessEggman,
	KnucklesTikal,
	RougeChaos,
	TailsChao,
	EggmanDarkChao,
	SuperSonic,
	SuperShadow
};

enum class CharacterID2 : unsigned char
{
	Sonic,
	Shadow,
	MechlessTails,
	MechlessEggman,
	Knuckles,
	Rouge,
	Tails,
	Eggman,
	Amy,
	SuperSonic,
	SuperShadow,
	MetalSonic,
	ChaoWalker,
	DarkChaoWalker,
	Tikal,
	Chaos
};

struct AbstractPlayer
{
public:
	// Character Object 1
	unsigned char	Action;
	short	Status;
	int		Angle[3];
	float	Position[3];

	// Character Object 2
	unsigned char	CharID[2];
	short	Powerups;
	unsigned int	Upgrades;
	float	MechHP;
	float	Speed[2];
	float	baseSpeed;

	unsigned char	AnimPlayback;
	unsigned char	AnimUnknown;
	unsigned short	Animation[3];

	// Sonic/Shadow specific:
	unsigned short SpinTimer;

	const CharacterID characterID() { return (CharacterID)CharID[0]; }
	const CharacterID2 characterID2() { return (CharacterID2)CharID[1]; }
};

class PlayerObject: public AbstractPlayer, public GameObject
{
public:
	// Constructor/Destructor
	PlayerObject(unsigned int address);// : GameObject(address) {}

	// Player pointer evaluator
	// Gets the pointer for the next loaded object in chain
	// and populates the objData arrays with addresses.
	void pointerEval();

	// Player memory reader
	// Reads player data from game memory and stores in the object.
	void read();

	// Player memory writer
	// Copies data from an AbstractPlayer object and writes it to game memory.
	void write(AbstractPlayer* recvr);

	// Player teleporter
	// Writes the position of this player to recvr's and stops all momentum.
	void Teleport(AbstractPlayer* recvr);

private:
	// Data pointers
	// Object Data 1 and 2 refers to Character Object 1 and 2. Details:
	// http://info.sonicretro.org/SCHG:Sonic_Adventure_2_(PC)/RAM_Editing#Pointer_Area
	unsigned int objData1ptr, objData2ptr;

	// The last pointer to the player object.
	// Used for deciding whether or not to re-evaluate.
	unsigned int lastPointer;

	static const struct Master
	{
		enum Offset : short
		{
			Previous	= 0x0000,
			Next		= 0x0004,
			Main		= 0x0010,
			Display		= 0x0014,
			objData1	= 0x0034,
			objData2	= 0x0040,
			objName		= 0x0044
		};
	};

	// Offsets in objData1
	static const struct objData1
	{
		enum Offset : short
		{
			Action	= 0x0000,
			Status	= 0x0004,
			RotX	= 0x0008,
			RotY	= 0x000C,
			RotZ	= 0x0010,
			PosX	= 0x0014,
			PosY	= 0x0018,
			PosZ	= 0x001C,
			ScaleX	= 0x0020,
			ScaleY	= 0x0024,
			ScaleZ	= 0x0028
		};
	};

	// Offsets in objData2
	static const struct objData2
	{
		enum Offset : short
		{
			PlayerNum		= 0x0000,
			CharID1			= 0x0001,
			CharID2			= 0x0003,
			ActionWindow	= 0x0004,
			ActionWindowNum = 0x000C,
			Powerups		= 0x0010,
			HangTime		= 0x0012,
			Upgrades		= 0x0024,
			MechHP			= 0x0048,
			HSpeed			= 0x0064,
			VSpeed			= 0x0068,
			Physics			= 0x00C0,
			BaseSpeed		= 0x00D0,
			AnimPlayback	= 0x0174,
			AnimUnknown		= 0x0175,
			NextAnim		= 0x0176,
			CurrentAnim		= 0x0178,
			dCurrentAnim	= 0x017A
		};
	};

	static const struct objSonic
	{
		enum Offset : short
		{
			HomingAttackTimer	= 0x035C,
			HomingRangeTimer	= 0x0364,
			SpindashTimer		= 0x0368
		};
	};
};