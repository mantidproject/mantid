#ifndef ALGORITHM_PROVIDER_H_
#define ALGORITHM_PROVIDER_H_

#include <iostream>
#include <string>
#include <map>
#include "Base.h"
#include "DeclareFactoryEntries.h"

// the types of the class factories
typedef Base* create_obj();
typedef void destroy_obj(Base*);
typedef Entry* entries_obj();

class AlgorithmProvider
{
public:
	virtual ~AlgorithmProvider();
	
	static AlgorithmProvider* Initialise();
	void GetAlgorithmList();
	Base* CreateAlgorithm(const std::string&);
	void DestroyAlgorithm(const std::string&, Base*);

private:
	static AlgorithmProvider* instance;
	static void* module;
	std::map<std::string, Entry*> algList;

	AlgorithmProvider();

};


#endif //ALGORITHM_PROVIDER_H_
