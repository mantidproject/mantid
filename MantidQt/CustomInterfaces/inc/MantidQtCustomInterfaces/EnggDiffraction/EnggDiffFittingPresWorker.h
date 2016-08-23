#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGPRESWORKER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGPRESWORKER_H_

#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffFittingPresenter.h"

#include <QThread>

namespace MantidQt {
namespace CustomInterfaces {

/**
Worker to run long tasks for the presenter of the fitting tab of the
EnggDiffraction GUI. It has a finished() signal, and it is expected to
emit it when the hard/long-work methods finish.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
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
class EnggDiffFittingWorker : public QObject {
  Q_OBJECT

public:
  // for fitting (single peak fits)
  EnggDiffFittingWorker(EnggDiffFittingPresenter *pres,
                        const std::vector<std::string> &focusedRunNo,
                        const std::string &ExpectedPeaks)
      : m_pres(pres), m_multiRunNo(focusedRunNo),
        m_expectedPeaks(ExpectedPeaks) {}

private slots:

  void fitting() {

    for (size_t i = 0; i < m_multiRunNo.size(); ++i) {

      auto runNo = m_multiRunNo[i];
      m_pres->doFitting(runNo, m_expectedPeaks);
    }
    emit finished();
  }

signals:
  void finished();

private:
  EnggDiffFittingPresenter *m_pres;

  /// sample run to process
  const std::vector<std::string> m_multiRunNo;
  // parameters for fitting, list of peaks
  const std::string m_expectedPeaks;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGPRESWORKER_H_
