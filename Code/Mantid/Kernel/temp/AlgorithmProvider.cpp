#include <iostream>

#include "../inc/DllOpen.h"
#include "../inc/AlgorithmProvider.h"
#include "../inc/DeclareFactoryEntries.h"

AlgorithmProvider* AlgorithmProvider::instance = 0;
void* AlgorithmProvider::module = 0;

AlgorithmProvider::AlgorithmProvider()
{

}

AlgorithmProvider::~AlgorithmProvider()
{
	//Close lib
	DllOpen::CloseDll(module);
	module = 0;
}

AlgorithmProvider* AlgorithmProvider::Initialise()
{
	if (!instance)
	{
		instance = new AlgorithmProvider;
				
		//Load dynamically loaded library
		module = DllOpen::OpenDll("UserAlgs");
		if (!module) 
		{
			std::cout << "Could not open library!\n";
			return 0;
		}
	}
	
	return instance;
}

void AlgorithmProvider::GetAlgorithmList()
{
	entries_obj* createMyAlg = (entries_obj*) DllOpen::GetFunction(module, "GetEntries");
	Entry* entry = createMyAlg();
	std::cout << "Algorithms available:\n";
	while (entry->name !=0)
	{
		std::cout << entry->name << '\n';
		algList[entry->name] = entry;
		
		++entry;
	}
}

Base* AlgorithmProvider::CreateAlgorithm(const std::string& algName)
{
	Entry* temp = algList[algName];
	
	if (!temp)
	{
		return 0;
	}
	
	create_obj* createMyAlg = (create_obj*) temp->constructor;
	
	return createMyAlg();
}

void AlgorithmProvider::DestroyAlgorithm(const std::string& algName, Base* obj)
{
	Entry* temp = algList[algName];
	
	if (!temp)
	{
		return;
	}
	
	destroy_obj* destroyMyAlg = (destroy_obj*) temp->constructor;

	destroyMyAlg(obj);
}

