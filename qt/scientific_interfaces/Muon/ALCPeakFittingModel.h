// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#include "DllConfig.h"
#include "IALCPeakFittingModel.h"
#include "IALCPeakFittingModelSubscriber.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/IAlgorithmRunnerSubscriber.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"

namespace MantidQt {
namespace CustomInterfaces {

/** ALCPeakFittingModel : Concrete model for ALC peak fitting
 */
class MANTIDQT_MUONINTERFACE_DLL ALCPeakFittingModel : public IALCPeakFittingModel,
                                                       public API::IAlgorithmRunnerSubscriber {
public:
  // -- IALCPeakFittingModel interface
  // -----------------------------------------------------------
  Mantid::API::IFunction_const_sptr fittedPeaks() const override { return m_fittedPeaks; }
  Mantid::API::MatrixWorkspace_sptr data() const override { return m_data; }
  Mantid::API::ITableWorkspace_sptr parameterTable() const { return m_parameterTable; }

  void fitPeaks(Mantid::API::IFunction_const_sptr peaks) override;

  Mantid::API::MatrixWorkspace_sptr guessData(Mantid::API::IFunction_const_sptr function,
                                              const std::vector<double> &xValues) override;
  // -- End of IALCPeakFittingModel interface
  // ----------------------------------------------------

  // -- IAlgorithmRunnerSubscriber interface
  // -----------------------------------------------------------
  void notifyBatchComplete(API::IConfiguredAlgorithm_sptr &algorithm, bool error) override;
  void notifyAlgorithmError(MantidQt::API::IConfiguredAlgorithm_sptr &algorithm, const std::string &message) override;

  // -- End of IAlgorithmRunnerSubscriber interface
  // ----------------------------------------------------

  ALCPeakFittingModel(std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner);

  ALCPeakFittingModel();

  /// Update the data
  void setData(Mantid::API::MatrixWorkspace_sptr newData);

  /// Export data and fitted peaks as a single workspace
  Mantid::API::MatrixWorkspace_sptr exportWorkspace();

  /// Export fitted peaks as a table workspace
  Mantid::API::ITableWorkspace_sptr exportFittedPeaks();

  void subscribe(IALCPeakFittingModelSubscriber *subscriber) override;

private:
  /// The subscriber to the model.
  IALCPeakFittingModelSubscriber *m_subscriber;

  /// The Algorithm runner for async processing.
  std::unique_ptr<MantidQt::API::IAlgorithmRunner> m_algorithmRunner;

  /// The data we are fitting peaks to
  Mantid::API::MatrixWorkspace_sptr m_data;

  /// Parameter table containing fit results
  Mantid::API::ITableWorkspace_sptr m_parameterTable;

  /// Setter for convenience
  void setFittedPeaks(Mantid::API::IFunction_const_sptr fittedPeaks);

  /// Last fitted peaks
  Mantid::API::IFunction_const_sptr m_fittedPeaks;
};

} // namespace CustomInterfaces
} // namespace MantidQt
