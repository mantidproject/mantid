#ifndef MANTID_ALGORITHM_HELLOWORLDALGORITHM_H_
#define MANTID_ALGORITHM_HELLOWORLDALGORITHM_H_

#include <iostream>

#include "MantidAPI/Algorithm.h"

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
  virtual const std::string name() const { return "HelloWorldAlgorithm";};///< Algorithm's name for identification
  virtual const std::string version() const { return "1";};///< Algorithm's name for identification

  ///Initialisation code
  void init() {  }
  ///Execution code
  void exec() { std::cout << "\nHello, World!\n"; }
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_HELLOWORLDALGORITHM_H_*/

