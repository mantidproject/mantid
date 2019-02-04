// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGPRESWORKER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGPRESWORKER_H_

#include "EnggDiffFittingPresenter.h"
#include "MantidKernel/Logger.h"

#include <QThread>
#include <stdexcept>

namespace MantidQt {
namespace CustomInterfaces {

/**
Worker to run long tasks for the presenter of the fitting tab of the
EnggDiffraction GUI. It has a finished() signal, and it is expected to
emit it when the hard/long-work methods finish.
*/
class EnggDiffFittingWorker : public QObject {
  Q_OBJECT

public:
  // for fitting (single peak fits)
  EnggDiffFittingWorker(EnggDiffFittingPresenter *pres,
                        const std::vector<RunLabel> &runLabels,
                        const std::string &expectedPeaks)
      : m_pres(pres), m_runLabels(runLabels), m_expectedPeaks(expectedPeaks) {}

private slots:

  void fitting() {
    try {
      m_pres->doFitting(m_runLabels, m_expectedPeaks);
    } catch (std::exception &ex) {
      Mantid::Kernel::Logger log("EngineeringDiffractionFitting");
      log.error(ex.what());
    }
    emit finished();
  }

signals:
  void finished();

private:
  EnggDiffFittingPresenter *m_pres;

  /// sample run to process
  const std::vector<RunLabel> m_runLabels;
  // parameters for fitting, list of peaks
  const std::string m_expectedPeaks;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGPRESWORKER_H_
