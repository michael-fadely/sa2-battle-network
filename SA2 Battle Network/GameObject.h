#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

class GameObject
{
public:
	GameObject(int baseAddress);

	// Re-evaluate initialized state.
	// Returns true if the object has been initialized,
	// else false.
	const bool CheckInitialized();
	// Check last initialized state.
	const bool Initialized() { return exists; }

protected:
	// The base address of which to check from
	int baseAddress;
	// The address pointed to by the base address
	int ptrAddress;
	// Defines whether or not the Game Object exists in memory
	// as defined by CheckInitialized() and returned by Initialized()
	bool exists;

};

#endif // GAMEOBJECT_H