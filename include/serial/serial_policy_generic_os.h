#pragma once

// the implicit interface class 'SerialPolicyGenericOS' must implement:
// openSerial(...), closeSerial(), tx(...), rx(...)

#if defined(_WIN32) || defined(_WIN64)
#include "serial_policy_win.h"
typedef SerialPolicyWin SerialPolicyGenericOS;
#elif defined(__linux__) || defined(__APPLE__)
#include "serial_policy_unix.h"
typedef SerialPolicyUnix SerialPolicyGenericOS;
#else
#error sistema operacional não suportado e/ou não identificado.
#endif