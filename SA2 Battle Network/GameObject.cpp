#include "LazyMemory.h"
#include "GameObject.h"

GameObject::GameObject(int baseAddress)
{
	this->baseAddress = baseAddress;
	this->ptrAddress = 0;
	this->exists = false;
}

const bool GameObject::CheckInitialized()
{
	ReadMemory(baseAddress, &ptrAddress, sizeof(int));

	return exists = (ptrAddress != 0);
}