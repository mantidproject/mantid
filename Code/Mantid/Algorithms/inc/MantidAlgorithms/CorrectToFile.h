#ifndef ALGORITHMSCORRECTTOFILE_H_
#define ALGORITHMSCORRECTTOFILE_H_

//-----------------------------
// Includes
//----------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{

namespace Algorithms
{
  /**
     Required properties:
     <UL>
     <LI> Filename - The filename containing the data
     </UL>

     @author Martyn Gigg, Tessella Support Services plc
     @date 19/01/2009
     
     Copyright &copy; 2009 STFC Rutherford Appleton Laboratories
     
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
class DLLExport CorrectToFile : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  CorrectToFile() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~CorrectToFile() {}
  /// Algorithm's name
  virtual const std::string name() const { return "CorrectToFile"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  //Check if the workspace units match
  void checkWorkspaceUnits(Mantid::API::MatrixWorkspace_sptr toRebin, Mantid::API::MatrixWorkspace_sptr toMatch);

  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
};

}

}
#endif /*ALGORITHMSCORRECTTOFILE_H_*/
