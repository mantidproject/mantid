#include "MantidPythonAPI/PyAlgorithm.h"

namespace Mantid
{
namespace PythonAPI
{

///init redirects to PyInit
bool PyAlgorithm::initialise() 
{
	try
	{
		PyInit();
		return true;
	}
	catch(...)
	{
		return false;
	}
}

///exec redirects to PyExec
bool PyAlgorithm::execute() 
{ 
	try
	{
		PyExec();
		return true;
	}
	catch(...)
	{
		return false;
	}
}

}
}

