#ifndef ALGORITHMSCREATESINGLEVALUEDWORKSPACE_H_
#define ALGORITHMSCREATESINGLEVALUEDWORKSPACE_H_

//------------------------------
//Includes
//------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{

namespace Algorithms
{
  /**
     Required properties:
     <UL>
     <LI>OutputWorkspace - The name of workspace to create</LI>
     <LI>DataValue - The value to place in this workspace</LI>
     </UL>

     Optional properties:
    <UL>
    <LI>ErrorValue - An error value</LI>
    </UL>
     @author Martyn Gigg, Tessella Support Services plc
     @date 28/01/2009
     
     Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
     
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
     Code Documentation is available at: <http://doxygen.mantidproject.org>    
  */
class DLLExport CreateSingleValuedWorkspace : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  CreateSingleValuedWorkspace() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~CreateSingleValuedWorkspace() {}
  /// Algorithm's name
  virtual const std::string name() const { return "CreateSingleValuedWorkspace"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

};

}

}

#endif /*ALGORITHMSCREATESINGLEVALUEDWORKSPACE_H_*/
