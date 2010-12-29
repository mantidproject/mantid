#ifndef MANTID_DATAHANDLING_GROUPDETECTORS_H_
#define MANTID_DATAHANDLING_GROUPDETECTORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{
/** An algorithm for grouping detectors and the spectra associated with them
    into a single DetectorGroup and spectrum.
    This algorithm can only be used on a workspace that has common X bins.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the (input & output) Workspace2D on which to perform the algorithm </LI>
    </UL>

    Optional Properties (Only one of these should be set. Priority to highest listed below if more than one is set.):
    <UL>
    <LI> SpectraList - An ArrayProperty containing a list of spectra to combine </LI>
    <LI> DetectorList - An ArrayProperty containing a list of detector IDs to combine </LI>
    <LI> WorkspaceIndexList - An ArrayProperty containing the workspace indices to combine </LI>
    </UL>

    Output Properties:
    <UL>
    <LI> ResultIndex - The workspace index containing the grouped spectra </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 17/04/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport GroupDetectors : public API::Algorithm
{
public:
  GroupDetectors();
  virtual ~GroupDetectors();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "GroupDetectors";};
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

#endif /*MANTID_DATAHANDLING_GROUPDETECTORS_H_*/
