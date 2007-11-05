#ifndef DECLAREFACTORYENTRIES_H
#define DECLAREFACTORYENTRIES_H

class Base;

struct Entry
{
	const char* name;
	Base* (*constructor)();
	void (*destructor)(Base*);
};

#if _WIN32

#define DECLARE_FACTORY_ENTRIES(x) \
Entry EntriesList[1024]; \
extern "C" __declspec (dllexport) Entry* GetEntries(); \
Entry* GetEntries(){ \
	int count = 0; \
	Entry dummy;

#else

#define DECLARE_FACTORY_ENTRIES(x) \
Entry EntriesList[1024]; \
extern "C" Entry* GetEntries(); \
Entry* GetEntries(){ \
	int count = 0; \
	Entry dummy;

#endif

#define DECLARE_ALGORITHM(x) \
	extern Base* x##_create(); \
	extern void x##_destroy(Base*); \
	dummy.name = #x; \
	dummy.constructor = &x##_create; \
	dummy.destructor = &x##_destroy; \
	EntriesList[count++] = dummy;

#define DECLARE_END \
	return EntriesList;  \
}


#endif /*DECLAREFACTORYENTRIES_H */
