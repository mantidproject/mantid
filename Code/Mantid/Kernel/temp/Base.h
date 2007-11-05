#ifndef BASE_H_
#define BASE_H_

#include <iostream>

#include "DeclareUserAlg.h"

class Base
{
public:
	Base() {};
	virtual ~Base() {};
	
	virtual void Print() { std::cout << "Hello From Base\n"; }
};

#endif /*BASE_H_*/
