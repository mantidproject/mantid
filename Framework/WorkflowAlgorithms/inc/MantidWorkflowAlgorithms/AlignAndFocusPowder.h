#ifndef MANTID_ALGORITHM_AlignAndFocusPowder_H_
#define MANTID_ALGORITHM_AlignAndFocusPowder_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"

namespace Mantid {
namespace Kernel {
class PropertyManager;
}

namespace WorkflowAlgorithms {
/**
This is a parent algorithm that uses several different child algorithms to
perform it's task.
Takes a workspace as input and the filename of a grouping file of a suitable
format.

The input workspace is
1) Converted to d-spacing units
2) Rebinned to a common set of bins
3) The spectra are grouped according to the grouping file.

    Required Properties:
<UL>
<LI> InputWorkspace - The name of the 2D Workspace to take as input </LI>
<LI> OutputWorkspace - The name of the 2D workspace in which to store the result
</LI>
</UL>


@author Vickie Lynch, SNS
@date 07/16/2012

Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class DLLExport AlignAndFocusPowder : public API::DataProcessorAlgorithm {
public:
  /// Constructor
  AlignAndFocusPowder() : API::DataProcessorAlgorithm() {}
  /// Destructor
  ~AlignAndFocusPowder() override {}

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "AlignAndFocusPowder"; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Workflow\\Diffraction";
  }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Algorithm to focus powder diffraction data into a number of "
           "histograms "
           "according to a grouping scheme defined in a CalFile.";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  void loadCalFile(const std::string &calFilename,
                   const std::string &groupFilename);
  API::MatrixWorkspace_sptr rebin(API::MatrixWorkspace_sptr matrixws);

  API::MatrixWorkspace_sptr conjoinWorkspaces(API::MatrixWorkspace_sptr ws1,
                                              API::MatrixWorkspace_sptr ws2,
                                              size_t offset);

  /// Call diffraction focus to a matrix workspace.
  API::MatrixWorkspace_sptr diffractionFocus(API::MatrixWorkspace_sptr ws);

  /// Convert units
  API::MatrixWorkspace_sptr convertUnits(API::MatrixWorkspace_sptr matrixws,
                                         std::string target);

  /// Call edit instrument geometry
  API::MatrixWorkspace_sptr editInstrument(API::MatrixWorkspace_sptr ws,
                                           std::vector<double> polars,
                                           std::vector<specnum_t> specids,
                                           std::vector<double> l2s,
                                           std::vector<double> phis);
  void convertOffsetsToCal(DataObjects::OffsetsWorkspace_sptr &offsetsWS);
  double getVecPropertyFromPmOrSelf(const std::string &name,
                                    std::vector<double> &avec);

  API::MatrixWorkspace_sptr m_inputW;
  API::MatrixWorkspace_sptr m_outputW;
  DataObjects::EventWorkspace_sptr m_inputEW;
  DataObjects::EventWorkspace_sptr m_outputEW;
  API::ITableWorkspace_sptr m_calibrationWS;
  DataObjects::MaskWorkspace_sptr m_maskWS;
  DataObjects::GroupingWorkspace_sptr m_groupWS;
  double m_l1{0.0};
  std::vector<int32_t> specids;
  std::vector<double> l2s;
  std::vector<double> tths;
  std::vector<double> phis;
  std::string m_instName;
  std::vector<double> m_params;
  int m_resampleX{0};
  std::vector<double> m_dmins;
  std::vector<double> m_dmaxs;
  bool dspace{false};
  double xmin{0.0};
  double xmax{0.0};
  double LRef{0.0};
  double DIFCref{0.0};
  double minwl{0.0};
  double maxwl{0.0};
  double tmin{0.0};
  double tmax{0.0};
  bool m_preserveEvents{false};
  void doSortEvents(Mantid::API::Workspace_sptr ws);

  /// Low resolution TOF matrix workspace
  API::MatrixWorkspace_sptr m_lowResW;
  /// Low resolution TOF event workspace
  DataObjects::EventWorkspace_sptr m_lowResEW;
  /// Flag to process low resolution workspace
  bool m_processLowResTOF{false};
  /// Offset to low resolution TOF spectra
  size_t m_lowResSpecOffset{0};

  std::unique_ptr<API::Progress> m_progress = nullptr; ///< Progress reporting
};

} // namespace WorkflowAlgorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_AlignAndFocusPowder_H_*/
