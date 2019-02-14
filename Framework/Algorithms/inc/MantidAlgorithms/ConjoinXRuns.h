// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CONJOINXRUNS_H_
#define MANTID_ALGORITHMS_CONJOINXRUNS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

#include <list>

namespace Mantid {
namespace Algorithms {

/** ConjoinXRuns : This algorithms joins the input workspaces horizontally,
 * i.e. by appending (concatenating) their columns.
 */
class MANTID_ALGORITHMS_DLL ConjoinXRuns : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

  /// ConjoinXRuns parameter names of the paramter file for sample log merging
  static const std::string SUM_MERGE;
  static const std::string TIME_SERIES_MERGE;
  static const std::string LIST_MERGE;
  static const std::string WARN_MERGE;
  static const std::string WARN_MERGE_TOLERANCES;
  static const std::string FAIL_MERGE;
  static const std::string FAIL_MERGE_TOLERANCES;

protected:
  void fillHistory() override;

private:
  void init() override;
  void exec() override;

  std::string checkLogEntry(API::MatrixWorkspace_sptr) const;
  std::vector<double> getXAxis(API::MatrixWorkspace_sptr) const;
  void joinSpectrum(int64_t);

  /// Sample log entry name
  std::string m_logEntry;
  /// Progress reporting
  std::unique_ptr<API::Progress> m_progress;
  /// List of input matrix workspaces
  std::list<API::MatrixWorkspace_sptr> m_inputWS;
  /// Output workspace
  API::MatrixWorkspace_sptr m_outWS;
  /// X-axis cache if sample log is given
  std::map<std::string, std::vector<double>> m_axisCache;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CONJOINXRUNS_H */
