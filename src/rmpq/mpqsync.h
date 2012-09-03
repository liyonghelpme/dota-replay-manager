#ifndef __RMPQ_MPQSYNC__
#define __RMPQ_MPQSYNC__

uint32 createCriticalSection();
void deleteCriticalSection(uint32 lock);
void enterCriticalSection(uint32 lock);
bool tryEnterCriticalSection(uint32 lock);
void leaveCriticalSection(uint32 lock);

class MPQLocker
{
  uint32 _lock;
public:
  MPQLocker(uint32 lock)
  {
    _lock = lock;
    enterCriticalSection(lock);
  }
  ~MPQLocker()
  {
    leaveCriticalSection(_lock);
  }
};

#endif // __RMPQ_MPQSYNC__
