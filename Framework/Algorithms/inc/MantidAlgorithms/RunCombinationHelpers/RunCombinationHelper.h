// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_RUNCOMBINATIONHELPER_H_
#define MANTID_ALGORITHMS_RUNCOMBINATIONHELPER_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/Logger.h"

#include <list>
#include <vector>

namespace Mantid {
namespace Algorithms {

/** RunCombinationHelper : This holds some useful utilities for operations
 * involving transformations of lists of workspaces into single one.
 * E.g. this is used commonly between MergeRuns and ConjoinXRuns
 */

namespace RunCombinationOptions {
static const std::string SKIP_BEHAVIOUR = "Skip File";
static const std::string STOP_BEHAVIOUR = "Stop";
static const std::string REBIN_BEHAVIOUR = "Rebin";
static const std::string FAIL_BEHAVIOUR = "Fail";
} // namespace RunCombinationOptions

class MANTID_ALGORITHMS_DLL RunCombinationHelper {
public:
  std::string checkCompatibility(API::MatrixWorkspace_sptr,
                                 bool checkNumberHistograms = false);
  void setReferenceProperties(API::MatrixWorkspace_sptr);
  static std::vector<std::string>
  unWrapGroups(const std::vector<std::string> &);
  std::list<API::MatrixWorkspace_sptr>
  validateInputWorkspaces(const std::vector<std::string> &inputWorkspaces,
                          Kernel::Logger &g_log);

private:
  size_t m_numberSpectra;
  size_t m_numberDetectors;
  std::string m_xUnit;
  std::string m_yUnit;
  std::string m_spectrumAxisUnit;
  std::string m_instrumentName;
  bool m_isHistogramData;
  bool m_isScanning;
  std::vector<bool> m_hasDx;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_RUNCOMBINATIONHELPER_H_ */
