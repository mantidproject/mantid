// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "TransmissionRunPair.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/optional.hpp>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class ReductionWorkspaces

    The ReductionWorkspaces model holds information about all input and output
    workspaces that are involved in a reduction
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL ReductionWorkspaces {
public:
  ReductionWorkspaces(std::vector<std::string> inputRunNumbers, TransmissionRunPair transmissionRuns);

  std::vector<std::string> const &inputRunNumbers() const;
  TransmissionRunPair const &transmissionRuns() const;
  std::string const &iVsLambda() const;
  std::string const &iVsQ() const;
  std::string const &iVsQBinned() const;

  void setOutputNames(std::string iVsLambda, std::string iVsQ, std::string iVsQBinned);
  void resetOutputNames();
  bool hasOutputName(std::string const &wsName) const;
  void renameOutput(std::string const &oldName, std::string const &newName);

private:
  std::vector<std::string> m_inputRunNumbers;
  TransmissionRunPair m_transmissionRuns;
  std::string m_iVsLambda;
  std::string m_iVsQ;
  std::string m_iVsQBinned;

  friend class Encoder;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(ReductionWorkspaces const &lhs, ReductionWorkspaces const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(ReductionWorkspaces const &lhs, ReductionWorkspaces const &rhs);

TransmissionRunPair transmissionWorkspaceNames(TransmissionRunPair const &transmissionRuns);

MANTIDQT_ISISREFLECTOMETRY_DLL ReductionWorkspaces workspaceNames(std::vector<std::string> const &inputRunNumbers,
                                                                  TransmissionRunPair const &transmissionRuns);

MANTIDQT_ISISREFLECTOMETRY_DLL std::string
postprocessedWorkspaceName(std::vector<std::vector<std::string> const *> const &summedRunNumbers);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
