#include	"MemPool.h"



int main()
{
	MemPool	mp;
	mp.Init(1024000);
	char* p1 = mp.Alloc(23);
	char* p2 = mp.Alloc(36);
	mp.Free(p1);
	mp.Alloc(50);
	mp.PrintStatistic();	
	return 0;
}
