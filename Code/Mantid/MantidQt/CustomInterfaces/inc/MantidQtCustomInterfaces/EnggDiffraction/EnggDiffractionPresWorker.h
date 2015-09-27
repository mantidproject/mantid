#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFRACTIONPRESWORKER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFRACTIONPRESWORKER_H_

#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresenter.h"

#include <QThread>

namespace MantidQt {
namespace CustomInterfaces {

/**
Worker to run long tasks for the presenter of the EnggDiffraction
GUI. It has a finished() signal, and it is expected to emit it when
the hard/long-work methods finish.

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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
class EnggDiffWorker : public QObject {
  Q_OBJECT

public:
  /// for calibration
  EnggDiffWorker(EnggDiffractionPresenter *pres, const std::string &outFilename,
                 const std::string &vanNo, const std::string &ceriaNo)
      : m_pres(pres), m_outFilenames(), m_outCalibFilename(outFilename),
        m_vanNo(vanNo), m_ceriaNo(ceriaNo), m_banks() {}

  /// for focusing
  EnggDiffWorker(EnggDiffractionPresenter *pres, const std::string &outDir,
                 const std::vector<std::string> &outFilenames,
                 const std::string &runNo, const std::vector<bool> &banks,
                 const std::string &specIDs, const std::string &dgFile)
      : m_pres(pres), m_outFilenames(outFilenames), m_outCalibFilename(),
        m_runNo(runNo), m_outDir(outDir), m_banks(banks), m_specIDs(specIDs),
        m_dgFile(dgFile) {}

private slots:

  /**
   * Calculate a new calibration. Connect this from a thread started()
   * signal.
   */
  void calibrate() {
    m_pres->doNewCalibration(m_outCalibFilename, m_vanNo, m_ceriaNo);
    emit finished();
  }

  /**
   * Focus a run. You must connect this from a thread started()
   * signal.
   */
  void focus() {
    m_pres->doFocusRun(m_outDir, m_outFilenames, m_runNo, m_banks, m_specIDs,
                       m_dgFile);
    emit finished();
  }

signals:
  void finished();

private:
  EnggDiffractionPresenter *m_pres;

  /// parameters for calibration
  const std::vector<std::string> m_outFilenames;
  const std::string m_outCalibFilename, m_vanNo, m_ceriaNo;
  /// sample run to process
  const std::string m_runNo;
  /// Output directory
  const std::string m_outDir;
  /// instrument banks: do focus/don't
  const std::vector<bool> m_banks;
  // parameters for specific types of focusing: "cropped"
  const std::string m_specIDs;
  // for focusing "texture"
  const std::string m_dgFile;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFRACTIONPRESWORKER_H_
