#ifndef MANTID_ALGORITHM_HELLOWORLDALGORITHM_H_
#define MANTID_ALGORITHM_HELLOWORLDALGORITHM_H_

#include <iostream>

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/StatusCode.h"

namespace Mantid
{
namespace Algorithms
{
	/** @class HelloWorldAlgorithm HelloWorldAlgorithm.h
   	
 	    Algorithm basic test class.
   			    	
      @author Matt Clarke, ISIS, RAL
      @date 09/11/2007
   	    
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
   	
      File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class HelloWorldAlgorithm : public API::Algorithm
{
public:
  ///no arg constructor
  HelloWorldAlgorithm() : API::Algorithm() {}
  ///virtual destructor
  virtual ~HelloWorldAlgorithm() {}
  ///Initialisation code
  Mantid::Kernel::StatusCode init() { return Mantid::Kernel::StatusCode::SUCCESS; }
  ///Execution code
  Mantid::Kernel::StatusCode exec() { std::cout << "\nHello, World!\n"; return Mantid::Kernel::StatusCode::SUCCESS; }
  ///Finalisation code
  Mantid::Kernel::StatusCode final() { return Mantid::Kernel::StatusCode::SUCCESS; }
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_HELLOWORLDALGORITHM_H_*/

