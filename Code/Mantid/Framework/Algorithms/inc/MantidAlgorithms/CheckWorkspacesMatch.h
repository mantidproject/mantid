#ifndef MANTID_ALGORITHMS_CHECKWORKSPACESMATCH_H_
#define MANTID_ALGORITHMS_CHECKWORKSPACESMATCH_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
/** Compares two workspaces for equality. This algorithm is mainly intended for
   use by Mantid
    developers as part of the testing process.

    The data values (X,Y and error) are always checked. The algorithm can also
   optionally check
    the axes (this includes the units), the spectra-detector map, the instrument
   (the name and
    parameter map) and any bin masking.

    Required Properties:
    <UL>
    <LI> Workspace1 - The name of the first input workspace </LI>
    <LI> Workspace2 - The name of the second input workspace </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> Tolerance       - The maximum amount by which values may differ between
   the workspaces (default: 0) </LI>
    <LI> CheckAxes       - Whether to check that the axes match (default: true)
   </LI>
    <LI> CheckSpectraMap - Whether to check that the spectra-detector maps match
   (default: true) </LI>
    <LI> CheckInstrument - Whether to check that the instruments match (default:
   true) </LI>
    <LI> CheckMasking    - Whether to check that the bin masking matches
   (default: true) </LI>
    <LI> CheckSample     - Whether to check that the sample object mathces
   (default: false) </LI>
    </UL>

    Output Properties:
    <UL>
    <LI> Result - Contains 'success' if the workspaces match, the reason for the
   failure otherwise </LI>
    </UL>

    Copyright &copy; 2009-2013 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CheckWorkspacesMatch : public API::Algorithm {
public:
  CheckWorkspacesMatch();
  virtual ~CheckWorkspacesMatch();
  /// Algorithm's name
  virtual const std::string name() const { return "CheckWorkspacesMatch"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Compares two workspaces for equality. This algorithm is mainly "
           "intended for use by the Mantid development team as part of the "
           "testing process.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Utility\\Workspaces"; }

  /** Return the string output when comparison is successful.
   */
  static std::string successString() { return "Success!"; }

private:
  // Process two groups and ensure the Result string is set properly on the
  // final algorithm
  virtual bool processGroups();
  // Process the two groups
  void processGroups(boost::shared_ptr<const API::WorkspaceGroup> groupOne,
                     boost::shared_ptr<const API::WorkspaceGroup> groupTwo);

  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  void doComparison();
  void doPeaksComparison(API::IPeaksWorkspace_sptr tws1,
                         API::IPeaksWorkspace_sptr tws2);
  void doTableComparison(API::ITableWorkspace_const_sptr tws1,
                         API::ITableWorkspace_const_sptr tws2);
  void doMDComparison(API::Workspace_sptr w1, API::Workspace_sptr w2);
  bool compareEventWorkspaces(DataObjects::EventWorkspace_const_sptr ews1,
                              DataObjects::EventWorkspace_const_sptr ews2);
  bool checkData(API::MatrixWorkspace_const_sptr ws1,
                 API::MatrixWorkspace_const_sptr ws2);
  bool checkAxes(API::MatrixWorkspace_const_sptr ws1,
                 API::MatrixWorkspace_const_sptr ws2);
  bool checkSpectraMap(API::MatrixWorkspace_const_sptr ws1,
                       API::MatrixWorkspace_const_sptr ws2);
  bool checkInstrument(API::MatrixWorkspace_const_sptr ws1,
                       API::MatrixWorkspace_const_sptr ws2);
  bool checkMasking(API::MatrixWorkspace_const_sptr ws1,
                    API::MatrixWorkspace_const_sptr ws2);
  bool checkSample(const API::Sample &sample1, const API::Sample &sample2);
  bool checkRunProperties(const API::Run &run1, const API::Run &run2);

  /// Compare 2 EventsList
  int compareEventsListInDetails(const DataObjects::EventList &el1,
                                 const DataObjects::EventList &el2,
                                 double tolTof, double tolWeight,
                                 int64_t tolPulse, bool printdetails,
                                 size_t &numdiffpulse, size_t &numdifftof,
                                 size_t &numdiffboth) const;

  std::string result; ///< the result string

  API::Progress *prog;
  /// Variable states if one wants to compare workspaces in parallell. This
  /// usully true but if one wants to look at the comparison logs, parallell
  /// comparison make things complicated as
  /// logs from different threads are mixed together.  In this case, it is
  /// better not to do parallell comparison.
  bool m_ParallelComparison;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CHECKWORKSPACESMATCH_H_*/
