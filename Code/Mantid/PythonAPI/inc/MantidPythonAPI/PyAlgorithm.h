#ifndef MANTID_PYTHONAPI_PYALGORITHM_H_
#define MANTID_PYTHONAPI_PYALGORITHM_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{

namespace PythonAPI
{

/** @class PyAlgorithm PyAlgorithm.h PythonAPI/PyAlgorithm.h

    PyAlgorithm is a version of the Algorithm class that is adjusted to allow algorithms
    to be written in Python.

    @author ISIS, STFC
    @date 18/09/2008

    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/
class DLLExport PyAlgorithm : public API::Algorithm
{
public:
	///Constructor - takes the name of the algorithm
	PyAlgorithm(std::string name) : API::Algorithm(), algName(name) {};
	virtual ~PyAlgorithm() {};
		
	///Returns the algorithm name	
	virtual const std::string name() const {return algName;}

	///init redirects to PyInit
	virtual bool initialise();
	///exec redirects to PyExec
	virtual bool execute();
	
	///PyInit is effectively a renaming of init
	virtual void PyInit() {};
	///PyExec is effectively a renaming of exec as that is a Python keyword
	virtual void PyExec() {};
	
private:
	///Default constructor is private
	PyAlgorithm() {};
	///Copy constructor is private	
	//PyAlgorithm(const PyAlgorithm&) {};
	///Assignment operator private
	PyAlgorithm& operator = (const PyAlgorithm&) {};
	
	///init redirects to PyInit
	virtual void init() { PyInit();}
	///exec redirects to PyExec
	virtual void exec() { PyExec();}
	
	///The name of the algorithm
	std::string algName;

};

}
}

#endif //MANTID_PYTHONAPI_PYALGORITHM_H_