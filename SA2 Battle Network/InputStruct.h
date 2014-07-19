#pragma once
/*
//	This is a thing I should do
struct Buttons
{
	unsigned int B		: 1;
	unsigned int A		: 1;
	unsigned int Start	: 1;
	unsigned int Up		: 1;
	unsigned int Down	: 1;
	unsigned int Left	: 1;
	unsigned int Right	: 1;
	unsigned int Z		: 1;
	unsigned int Y		: 1;
	unsigned int X		: 1;
	unsigned int		: 5;	// Padding
	unsigned int R		: 1;
	unsigned int L		: 1;
	unsigned int		: 15;	// Padding
};
*/

struct inputButtons
{
	int Held;
	int Pressed;
	int LastPressed;
	int NotHeld;
	int Clear;
};

struct inputAnalog
{
	short x;
	short y;
};

struct abstractInput
{
	inputButtons buttons;
	inputAnalog analog;
};

class InputStruct
{
public:
	unsigned int baseAddress;
	inputButtons buttons;
	inputAnalog analog;

	// Constructor/Destructor
	InputStruct(unsigned int baseAddress);
	//~InputStruct();

	// Reads input data from game memory and stores in the object.
	void read();

	// Writes input data to game memory
	void write(abstractInput* recvr);

	// Write Button Buffer to game memory
	void writeButtons(abstractInput* recvr);
	// Write analog Buffer to game memory
	void writeAnalog(abstractInput* recvr, unsigned int gamestate);

	// Button Press Simulator
	// Uses Held Buttons to simulate button presses.
	void press();

private:
	static const struct Offset
	{
		enum
		{
			Held		= 0x08,
			NotHeld		= 0x0C,
			Pressed		= 0x10,
			Released	= 0x14,
			AnalogX		= 0x1C,
			AnalogY		= 0x1E,
		};
	};
};