#ifndef __REPLAY_ORDERS__
#define __REPLAY_ORDERS__

#include "base/types.h"
#include "base/string.h"

char const* orderId2String(uint32 id);
String id2String(uint32 id);
bool isGoodId(uint32 id);

#endif // __REPLAY_ORDERS__
