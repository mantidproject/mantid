// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFRACTIONPRESWORKER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFRACTIONPRESWORKER_H_

#include "EnggDiffractionPresenter.h"

#include <QThread>

namespace MantidQt {
namespace CustomInterfaces {

/**
Worker to run long tasks for the presenter of the EnggDiffraction
GUI. It has a finished() signal, and it is expected to emit it when
the hard/long-work methods finish.
*/
class EnggDiffWorker : public QObject {
  Q_OBJECT

public:
  /// for calibration
  EnggDiffWorker(EnggDiffractionPresenter *pres, const std::string &outFilename,
                 const std::string &vanNo, const std::string &ceriaNo,
                 const std::string &specNos)
      : m_pres(pres), m_outFilenames(), m_outCalibFilename(outFilename),
        m_vanNo(vanNo), m_ceriaNo(ceriaNo), m_CalibSpectrumNos(specNos),
        m_banks(), m_bin(.0), m_nperiods(0) {}

  /// for focusing
  EnggDiffWorker(EnggDiffractionPresenter *pres,
                 const std::vector<std::string> &runNo,
                 const std::vector<bool> &banks, const std::string &SpectrumNos,
                 const std::string &dgFile)
      : m_pres(pres), m_outCalibFilename(), m_multiRunNo(runNo), m_banks(banks),
        m_SpectrumNos(SpectrumNos), m_dgFile(dgFile), m_bin(.0), m_nperiods(0) {
  }

  // for rebinning (ToF)
  EnggDiffWorker(EnggDiffractionPresenter *pres, const std::string &runNo,
                 double bin, const std::string &outWSName)
      : m_pres(pres), m_runNo(runNo), m_bin(bin), m_nperiods(0),
        m_outWSName(outWSName) {}

  // for rebinning (by pulse times)
  EnggDiffWorker(EnggDiffractionPresenter *pres, const std::string &runNo,
                 size_t nperiods, double timeStep, const std::string &outWSName)
      : m_pres(pres), m_runNo(runNo), m_bin(timeStep), m_nperiods(nperiods),
        m_outWSName(outWSName) {}

private slots:

  /**
   * Calculate a new calibration. Connect this from a thread started()
   * signal.
   */
  void calibrate() {
    m_pres->doNewCalibration(m_outCalibFilename, m_vanNo, m_ceriaNo,
                             m_CalibSpectrumNos);
    emit finished();
  }

  /**
   * Focus a run. You must connect this from a thread started()
   * signal.
   */
  void focus() {

    for (auto runNo : m_multiRunNo) {

      m_pres->doFocusRun(runNo, m_banks, m_SpectrumNos, m_dgFile);
    }
    emit finished();
  }

  void rebinTime() {
    m_pres->doRebinningTime(m_runNo, m_bin, m_outWSName);
    emit finished();
  }

  void rebinPulses() {
    m_pres->doRebinningPulses(m_runNo, m_nperiods, m_bin, m_outWSName);
    emit finished();
  }

signals:
  void finished();

private:
  EnggDiffractionPresenter *m_pres;

  /// parameters for calibration
  const std::vector<std::string> m_outFilenames;
  const std::string m_outCalibFilename, m_vanNo, m_ceriaNo;
  // parameters for specific types of calibration: "cropped"
  const std::string m_CalibSpectrumNos;
  /// sample run to process
  const std::string m_runNo;
  // sample multi-run to process
  const std::vector<std::string> m_multiRunNo;

  /// instrument banks: do focus/don't
  const std::vector<bool> m_banks;
  // parameters for specific types of focusing: "cropped"
  const std::string m_SpectrumNos;
  // for focusing "texture"
  const std::string m_dgFile;
  // parameters for pre-processing/rebinning
  const double m_bin;
  const size_t m_nperiods;
  const std::string m_outWSName;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFRACTIONPRESWORKER_H_
