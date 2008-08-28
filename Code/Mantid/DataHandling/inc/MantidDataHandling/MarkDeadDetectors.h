#ifndef MANTID_DATAHANDLING_MARKDEADDETECTORS_H_
#define MANTID_DATAHANDLING_MARKDEADDETECTORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace DataHandling
{
/** An algorithm to mark a detector, or set of detectors, as dead.
    The workspace spectra associated with those detectors are zeroed.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the (input & output) Workspace2D on which to perform the algorithm </LI>
    </UL>

    Optional Properties (One or the other should be set. SpectraList is used if both are set.):
    <UL>
    <LI> SpectraList - An ArrayProperty containing a list of spectra to mark dead </LI>
    <LI> DetectorList - An ArrayProperty containing a list of detector IDs to mark dead </LI>
    <LI> WorkspaceIndexList - An ArrayProperty containing the workspace indices to mark dead </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 15/04/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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
class DLLExport MarkDeadDetectors : public API::Algorithm
{
public:
  MarkDeadDetectors();
  virtual ~MarkDeadDetectors();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "MarkDeadDetectors";};
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Detectors";}

private:
  // Implement abstract Algorithm methods
  void init();
  void exec();
  void fillIndexListFromSpectra(std::vector<int>& indexList, std::vector<int>& spectraList,
                                const DataObjects::Workspace2D_sptr WS);

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_MARKDEADDETECTORS_H_*/
