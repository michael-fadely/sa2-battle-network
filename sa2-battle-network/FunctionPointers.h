#pragma once

// Non-static non-const versions of the mod loader function pointer macros.

#define _FunctionPointer(RETURN_TYPE, NAME, ARGS, ADDRESS) \
	RETURN_TYPE (__cdecl *NAME)ARGS = (RETURN_TYPE (__cdecl *)ARGS)ADDRESS
#define _StdcallFunctionPointer(RETURN_TYPE, NAME, ARGS, ADDRESS) \
	RETURN_TYPE (__stdcall *NAME)ARGS = (RETURN_TYPE (__stdcall *)ARGS)ADDRESS
#define _FastcallFunctionPointer(RETURN_TYPE, NAME, ARGS, ADDRESS) \
	RETURN_TYPE (__fastcall *NAME)ARGS = (RETURN_TYPE (__fastcall *)ARGS)ADDRESS
#define _ThiscallFunctionPointer(RETURN_TYPE, NAME, ARGS, ADDRESS) \
	RETURN_TYPE (__thiscall *NAME)ARGS = (RETURN_TYPE (__thiscall *)ARGS)ADDRESS
#define _VoidFunc(NAME, ADDRESS) _FunctionPointer(void,NAME,(void),ADDRESS)
#define _ObjectFunc(NAME, ADDRESS) _FunctionPointer(void,NAME,(ObjectMaster *obj),ADDRESS)
