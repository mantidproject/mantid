#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGPRESENTER_H_

#include "DllConfig.h"
#include "IEnggDiffFittingModel.h"
#include "IEnggDiffFittingPresenter.h"
#include "IEnggDiffFittingView.h"
#include "IEnggDiffractionCalibration.h"
#include "IEnggDiffractionParam.h"

#include <string>
#include <vector>

#include <QObject>

class QThread;

namespace MantidQt {
namespace CustomInterfaces {

/**
Presenter for the fitting tab/widget of the enggineering diffraction
GUI (presenter as in the MVP Model-View-Presenter pattern).

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
// needs to be dll-exported for the tests
class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffFittingPresenter
    : public QObject,
      public IEnggDiffFittingPresenter,
      public IEnggDiffractionCalibration,
      public IEnggDiffractionParam {
  // Q_OBJECT for 'connect' with thread/worker
  Q_OBJECT

public:
  EnggDiffFittingPresenter(
      IEnggDiffFittingView *view, std::unique_ptr<IEnggDiffFittingModel> model,
      boost::shared_ptr<IEnggDiffractionCalibration> mainCalib,
      boost::shared_ptr<IEnggDiffractionParam> mainParam);
  ~EnggDiffFittingPresenter() override;

  void notify(IEnggDiffFittingPresenter::Notification notif) override;

  /// From the IEnggDiffractionCalibration interface
  //@{
  std::vector<GSASCalibrationParms> currentCalibration() const override;
  //@}

  /// From the IEnggDiffractionCalibration interface
  //@{
  Poco::Path outFilesUserDir(const std::string &addToDir) override;
  //@}

  /// the fitting hard work that a worker / thread will run
  void doFitting(const int runNumber, const size_t bank,
                 const std::string &expectedPeaks);

  void plotFocusedFile(bool plotSinglePeaks,
                       Mantid::API::MatrixWorkspace_sptr focusedPeaksWS);

  void plotFitPeaksCurves();

protected:
  void processStart();
  void processLoad();
  void processFitPeaks();
  void processFitAllPeaks();
  void processShutDown();
  void processLogMsg();
  void processRemoveRun();

  /// clean shut down of model, view, etc.
  void cleanup();

protected slots:

  void fittingFinished();

private:
  bool isDigit(const std::string &text) const;

  void warnFileNotFound(const std::exception &ex);

  // Methods related single peak fits
  virtual void startAsyncFittingWorker(
      const std::vector<std::pair<int, size_t>> &runNumberBankPairs,
      const std::string &expectedPeaks);

  std::string getBaseNameFromStr(const std::string &filePath) const;

  void validateFittingInputs(const std::string &focusedRunNo,
                             const std::string &expectedPeaks);

  void browsePeaksToFit();

  void addPeakToList();

  void savePeakList();

  std::string readPeaksFile(std::string fileDir);

  void fittingWriteFile(const std::string &fileDir);

  // Holds the previous user input so we can short circuit further checks
  std::string m_previousInput;

  /// true if the last fitting completed successfully
  bool m_fittingFinishedOK;

  QThread *m_workerThread;

  /// interface for the 'current' calibration
  boost::shared_ptr<IEnggDiffractionCalibration> m_mainCalib;

  /// interface for the 'current' calibration
  boost::shared_ptr<IEnggDiffractionParam> m_mainParam;

  /// Associated view for this presenter (MVP pattern)
  IEnggDiffFittingView *const m_view;

  /// Associated model for this presenter
  std::unique_ptr<IEnggDiffFittingModel> m_model;

  /// Holds if the view is in the process of being closed
  bool m_viewHasClosed;

  /// Handle the user selecting a different run to plot
  void processSelectRun();

  /// Whether the user is doing fitting on multiple runs
  bool m_multiRunMode;
};

} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGPRESENTER_H_
