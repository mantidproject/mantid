#ifndef MANTID_DATAHANDLING_SAVEDASC_H_
#define MANTID_DATAHANDLING_SAVEDASC_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <string>

namespace Mantid
{
namespace DataHandling
{
/**
     Saves a workspace into an ASCII format that can be read by the
     DAVE analysis software.

     Required properties:
     <UL>
     <LI> InputWorkspace - The workspace name to save. </LI>
     <LI> Filename - The filename for output </LI>
     </UL>

     @author Steve Williams ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
     @date 27/07/2009

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
class DLLExport SaveDASC : public API::Algorithm
{
public:
  /// (Empty) Constructor
  SaveDASC() : API::Algorithm(){}
  /// Virtual destructor
  virtual ~SaveDASC() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SaveDASC"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }
  ///
private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  void writeHeader(API::MatrixWorkspace_const_sptr WS, std::ofstream &output);
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_SAVEDASC_H_

