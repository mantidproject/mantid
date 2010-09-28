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
     <LI>WorkspaceToCorrect - The input workspace to correct</LI>
     <LI>Filename - The filename containing the data to use</LI>
     <LI>FirstColumnValue - What does the first column of the file denote</LI>
     <LI>WorkspaceOperation - Whether to divide or multiply by the file data</LI>
     <LI>OutputWorkspace - The output workspace to use for the results </LI> 
     </UL>

     @author Martyn Gigg, Tessella Support Services plc
     @date 19/01/2009
     
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
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  static const double LOAD_TIME;
  /// Initialisation code
  void init();
  ///Execution code
  void exec();
  /// Load in the RKH file for that has the correction information
  API::MatrixWorkspace_sptr fileToWksp(const std::string & corrFile);
  /// Multiply or divide the input workspace as specified by the user
  void doWkspAlgebra(API::MatrixWorkspace_sptr lhs, API::MatrixWorkspace_sptr rhs, const std::string & operation, API::MatrixWorkspace_sptr & result);
};

}
}
#endif /*ALGORITHMSCORRECTTOFILE_H_*/
