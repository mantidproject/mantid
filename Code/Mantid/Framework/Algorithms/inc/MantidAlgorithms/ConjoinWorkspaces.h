#ifndef MANTID_ALGORITHMS_CONJOINWORKSPACES_H_
#define MANTID_ALGORITHMS_CONJOINWORKSPACES_H_
/*WIKI* 


This algorithm can be useful when working with large datasets. It enables the raw file to be loaded in two parts (not necessarily of equal size), the data processed in turn and the results joined back together into a single dataset. This can help avoid memory problems either because intermediate workspaces will be smaller and/or because the data will be much reduced after processing.

The output of the algorithm, in which the data from the second input workspace will be appended to the first, will be stored under the name of the first input workspace. Workspace data members other than the data (e.g. instrument etc.) will be copied from the first input workspace (but if they're not identical anyway, then you probably shouldn't be using this algorithm!). Both input workspaces will be deleted.

==== Restrictions on the input workspace ====

The input workspaces must come from the same instrument, have common units and bins and no detectors that contribute to spectra should overlap.


*WIKI*/

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
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();
  void execEvent();

  void validateInputs(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2) const;
  void checkForOverlap(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2, bool checkSpectra) const;
  void fixSpectrumNumbers(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2, API::MatrixWorkspace_sptr output);
  bool processGroups(API::WorkspaceGroup_sptr wsPt, const std::vector<Kernel::Property*>& prop);

  /// Progress reporting object
  API::Progress *m_progress;
  /// First event workspace input.
  DataObjects::EventWorkspace_const_sptr event_ws1;
  /// Second event workspace input.
  DataObjects::EventWorkspace_const_sptr event_ws2;

};

} // namespace Algorithm
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CONJOINWORKSPACES_H_ */
