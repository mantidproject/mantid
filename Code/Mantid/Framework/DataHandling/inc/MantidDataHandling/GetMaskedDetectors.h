#ifndef MANTID_DATAHANDLING_GETMASKEDDETECTORS_H_
#define MANTID_DATAHANDLING_GETMASKEDDETECTORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{
/** Retrieve the list of masked detectors from a workspace.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the input Workspace2D to get the list of masked detectors from</LI>
    <LI> DetectorList - Output ArrayProperty containing a list of masked detector IDs </LI>
    </UL>

    @author Mathieu Doucet, ORNL
    @date 07/15/2010

    Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport GetMaskedDetectors : public API::Algorithm
{
public:
  GetMaskedDetectors();
  virtual ~GetMaskedDetectors();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "GetMaskedDetectors";};
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Detectors";}

private:
  // Implement abstract Algorithm methods
  void init();
  void exec();
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_GETMASKEDDETECTORS_H_*/
