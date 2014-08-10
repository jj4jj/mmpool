#include "MemPoolStrategy.h"
#include "Buffer.h"

//for smart pointer surporting
class MemPoolPtr
{
public:
MemPoolPtr(char* pt,MemPool* p):
	pointer(pt),mp(p){}
void operator delete()
{
	mp->Free(pointer);
}
explicit
char* operator (char*)()
{
	return pointer;
}

private:
	char*	pointer;
	MemPool	* mp;
};

class MemPool
{
public:
	//return 0 is ok , otherwise is error.
	int	Init(uint64_t cap,MemStrategy * pstrategy = NULL);
	//alloc sz size and return 
	char*	Alloc(uint64_t sz);
	//free a pointer 
	int	Free(char* p);
//some statistic function
	uint64_t	GetAvailableSize();
	void		PrintStatistic();
//buffer and manageing function
private:
	Buffer			buffer;
	MemPoolStrategy*	strategy;
};
