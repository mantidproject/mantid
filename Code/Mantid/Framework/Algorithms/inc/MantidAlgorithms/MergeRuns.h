#ifndef MANTID_ALGORITHMS_MERGERUNS_H_
#define MANTID_ALGORITHMS_MERGERUNS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <list>
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** Combines the data contained in an arbitrary number of input workspaces.
    If the input workspaces do not have common binning, the bins in the output workspace
    will cover the entire range of all the input workspaces, with the largest bin widths
    used in areas of overlap.
    The input workspaces must contain histogram data with the same number of spectra,
    units and instrument name in order for the algorithm to succeed.
    Other than this it is currently left to the user to ensure that the combination of the
    workspaces is a valid operation.

    Required Properties:
    <UL>
    <LI> InputWorkspaces  - The names of the input workspace as a comma separated list. </LI>
    <LI> OutputWorkspace - The name of the output workspace which will contain the combined inputs. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 22/09/2008

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport MergeRuns : public API::Algorithm
{
public:
  MergeRuns();
  virtual ~MergeRuns();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "MergeRuns"; }
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
  void buildAdditionTables();


  /// An addition table is a list of pairs: First int = workspace index in the EW being added, Second int = workspace index to which it will be added in the OUTPUT EW. -1 if it should add a new entry at the end.
  typedef std::vector< std::pair<int, int> >  AdditionTable;

  // Methods called by exec()
  std::list<API::MatrixWorkspace_sptr> validateInputs(const std::vector<std::string>& inputWorkspaces);
  bool validateInputsForEventWorkspaces(const std::vector<std::string>& inputWorkspaces);
  void calculateRebinParams(const API::MatrixWorkspace_const_sptr& ws1, const API::MatrixWorkspace_const_sptr& ws2, std::vector<double>& params) const;
  void noOverlapParams(const MantidVec& X1, const MantidVec& X2, std::vector<double>& params) const;
  void intersectionParams(const MantidVec& X1, int& i, const MantidVec& X2, std::vector<double>& params) const;
  void inclusionParams(const MantidVec& X1, int& i, const MantidVec& X2, std::vector<double>& params) const;
  API::MatrixWorkspace_sptr rebinInput(const API::MatrixWorkspace_sptr& workspace, const std::vector<double>& params);
  /// Progress reporting
  API::Progress* m_progress;

  /// List of input workspaces
  std::list<Mantid::API::MatrixWorkspace_sptr> inWS;
  /// List of input EVENT workspaces
  std::vector<Mantid::DataObjects::EventWorkspace_sptr> inEventWS;
  /// Addition tables for event workspaces
  std::vector<AdditionTable *> tables;


};

} // namespace Algorithm
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MERGERUNS_H_ */
