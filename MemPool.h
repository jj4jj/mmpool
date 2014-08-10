#include "MemPoolStrategy.h"

class MemPoolPtr
{
public:
MemPoolPtr(char* pt,MemPool* p):
	pointer(pt),mp(p){}
void operator delete()
{
	mp->Free(pointer);
}

private:
char*	pointer;
MemPool	* mp;
};

class MemPool
{
public:
	//return 0 is ok , otherwise is error.
	int	Init();
	//alloc sz size and return 
	char*	Alloc(uint64_t sz);
	//free a pointer 
	int	Free(char* p);
//some statistic function
	uint64_t	GetAvailableSize();
	void		PrintStatistic();
//buffer and manageing function
private:
	char*		buffer;
	uint64_t	cap;
};
