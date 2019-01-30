// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_RUN_H_
#define MANTID_CUSTOMINTERFACES_RUN_H_
#include "Common/DllConfig.h"
#include "ItemState.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "RangeInQ.h"
#include "ReductionOptionsMap.h"
#include "ReductionWorkspaces.h"
#include <boost/optional.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/variant.hpp>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

// Immutability here makes update notification easier.
class MANTIDQT_ISISREFLECTOMETRY_DLL Row
    : public API::BatchAlgorithmRunnerSubscriber {
public:
  Row(std::vector<std::string> number, double theta,
      std::pair<std::string, std::string> tranmissionRuns, RangeInQ qRange,
      boost::optional<double> scaleFactor, ReductionOptionsMap reductionOptions,
      ReductionWorkspaces reducedWorkspaceNames);

  std::vector<std::string> const &runNumbers() const;
  std::pair<std::string, std::string> const &transmissionWorkspaceNames() const;
  double theta() const;
  RangeInQ const &qRange() const;
  boost::optional<double> scaleFactor() const;
  ReductionOptionsMap const &reductionOptions() const;
  ReductionWorkspaces const &reducedWorkspaceNames() const;

  Row withExtraRunNumbers(std::vector<std::string> const &runNumbers) const;

  void
  notifyAlgorithmStarted(Mantid::API::IAlgorithm_sptr const algorithm) override;
  void notifyAlgorithmComplete(
      Mantid::API::IAlgorithm_sptr const algorithm) override;
  void notifyAlgorithmError(Mantid::API::IAlgorithm_sptr const algorithm,
                            std::string const &msg) override;

  State state() const;
  std::string message() const;
  bool requiresProcessing(bool reprocessFailed) const;

private:
  std::vector<std::string> m_runNumbers;
  double m_theta;
  RangeInQ m_qRange;
  boost::optional<double> m_scaleFactor;
  std::pair<std::string, std::string> m_transmissionRuns;
  ReductionWorkspaces m_reducedWorkspaceNames;
  ReductionOptionsMap m_reductionOptions;
  ItemState m_itemState;

  void setProgress(double p, std::string const &msg);
  void setStarting();
  void setRunning();
  void setSuccess();
  void setError(std::string const &msg);
};

MANTIDQT_ISISREFLECTOMETRY_DLL Row mergedRow(Row const &rowA, Row const &rowB);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACE_RUN_H_
