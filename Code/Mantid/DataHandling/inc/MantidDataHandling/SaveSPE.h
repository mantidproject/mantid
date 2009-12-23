#ifndef MANTID_DATAHANDLING_SAVESPE_H_
#define MANTID_DATAHANDLING_SAVESPE_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{
/**
     Saves a workspace into an ASCII SPE file.

     Required properties:
     <UL>
     <LI> InputWorkspace - The workspace name to save. </LI>
     <LI> Filename - The filename for output </LI>
     </UL>

     @author Stuart Campbell, NScD, Oak Ridge National Laboratory
     @date 08/09/2009

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
class DLLExport SaveSPE : public API::Algorithm
{
public:
  /// (Empty) Constructor
  SaveSPE() : API::Algorithm(){}
  /// Virtual destructor
  virtual ~SaveSPE() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SaveSPE"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }
  ///
private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();
  /// Number format string for a single number to be printed by fprintf
  static const char NUM_FORM[7];
  /// Number format string for a line of numbers printed by fprintf
  static const char NUMS_FORM[50];
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_SAVESPE_H_

