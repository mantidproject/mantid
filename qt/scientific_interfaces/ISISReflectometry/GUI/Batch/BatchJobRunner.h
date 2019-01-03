// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BATCHJOBRUNNER_H_
#define MANTID_CUSTOMINTERFACES_BATCHJOBRUNNER_H_

#include "Common/DllConfig.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "Reduction/Batch.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL BatchJobRunner {
public:
  BatchJobRunner(Batch batch, API::BatchAlgorithmRunner &batchAlgoRunner);

  bool isProcessing() const;
  bool isAutoreducing() const;

  void resumeReduction();
  void pauseReduction();
  void resumeAutoreduction();
  void pauseAutoreduction();

  void progressHandle(const Mantid::API::IAlgorithm *alg, double p,
                      const std::string &msg);

private slots:
  void rowReductionComplete(bool error);

private:
  Batch m_batch;
  bool m_isProcessing;
  bool m_isAutoreducing;
  API::BatchAlgorithmRunner &m_batchAlgoRunner;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_BATCHJOBRUNNER_H_
