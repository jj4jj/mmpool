
class MemPoolStrategy
{
public:
	virtual char* Alloc(uint64_t sz);
	virtual	char* Free(char* p);
	virtual	uint64_t GetAvailableSize();
	virtual PrintStatistic();	
public:
	virtual ~MemPoolStrategy(){}
private:
	Buffer & buffer;
};
