#ifndef MANTID_ALGORITHMS_CONJOINWORKSPACES_H_
#define MANTID_ALGORITHMS_CONJOINWORKSPACES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/PairedGroupAlgorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** Joins two partial, non-overlapping workspaces into one. Used in the situation where you
    want to load a raw file in two halves, process the data and then join them back into
    a single dataset.
    The input workspaces must come from the same instrument, have common units and bins
    and no detectors that contribute to spectra should overlap.

    Required Properties:
    <UL>
    <LI> InputWorkspace1  - The name of the first input workspace. </LI>
    <LI> InputWorkspace2  - The name of the second input workspace. </LI>
    </UL>

    The output will be stored in the first named input workspace, the second will be deleted.

    @author Russell Taylor, Tessella
    @date 25/08/2008

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ConjoinWorkspaces : public API::PairedGroupAlgorithm
{
public:
  /// Empty constructor
  ConjoinWorkspaces();
  /// Destructor
  virtual ~ConjoinWorkspaces();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "ConjoinWorkspaces"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  // Overridden Algorithm methods
  void init();
  void exec();
  void execEvent();

  void validateInputs(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2) const;
  void checkForOverlap(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2, bool checkSpectra) const;
  bool processGroups(API::WorkspaceGroup_sptr wsPt, const std::vector<Kernel::Property*>& prop);

  /// Progress reporting object
  API::Progress *m_progress;
  /// Event workspace inputs.
  DataObjects::EventWorkspace_const_sptr event_ws1, event_ws2;

};

} // namespace Algorithm
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CONJOINWORKSPACES_H_ */
