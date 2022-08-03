#include "stdafx.h"
#include "operator_overloads.h"

bool operator==(const NJS_VECTOR& lhs, const NJS_VECTOR& rhs)
{
	return lhs.x == rhs.x &&
	       lhs.y == rhs.y &&
	       lhs.z == rhs.z;
}

bool operator!=(const NJS_VECTOR& lhs, const NJS_VECTOR& rhs)
{
	return lhs.x != rhs.x ||
	       lhs.y != rhs.y ||
	       lhs.z != rhs.z;
}

bool operator==(const Rotation& lhs, const Rotation& rhs)
{
	return lhs.x == rhs.x &&
	       lhs.y == rhs.y &&
	       lhs.z == rhs.z;
}

bool operator!=(const Rotation& lhs, const Rotation& rhs)
{
	return lhs.x != rhs.x ||
	       lhs.y != rhs.y ||
	       lhs.z != rhs.z;
}
