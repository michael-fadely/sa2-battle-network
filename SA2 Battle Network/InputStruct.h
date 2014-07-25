#pragma once

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

struct AbstractInput
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
	InputStruct(const unsigned int baseAddress);
	//~InputStruct();

	// Reads input data from game memory and stores in the object.
	void read();

	// Writes input data to game memory
	void write(AbstractInput* recvr);

	// Write Button Buffer to game memory
	void writeButtons(AbstractInput* recvr);
	// Write Analog Buffer to game memory
	void writeAnalog(AbstractInput* recvr, unsigned int gamestate);

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