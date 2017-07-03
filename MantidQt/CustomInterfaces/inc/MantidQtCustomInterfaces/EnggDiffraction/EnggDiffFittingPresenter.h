#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGPRESENTER_H_

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffFittingPresenter.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffFittingView.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffractionCalibration.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffractionParam.h"

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
class MANTIDQT_CUSTOMINTERFACES_DLL EnggDiffFittingPresenter
    : public QObject,
      public IEnggDiffFittingPresenter,
      public IEnggDiffractionCalibration,
      public IEnggDiffractionParam {
  // Q_OBJECT for 'connect' with thread/worker
  Q_OBJECT

public:
  EnggDiffFittingPresenter(
      IEnggDiffFittingView *view,
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
  void doFitting(const std::string &focusedRunNo,
                 const std::string &expectedPeaks);

  void runLoadAlg(const std::string focusedFile,
                  Mantid::API::MatrixWorkspace_sptr &focusedWS);

  void runFittingAlgs(std::string FocusedFitPeaksTableName,
                      std::string FocusedWSName);

  std::string
  functionStrFactory(Mantid::API::ITableWorkspace_sptr &paramTableWS,
                     std::string tableName, size_t row, std::string &startX,
                     std::string &endX);

  void plotFocusedFile(bool plotSinglePeaks);

  void plotFitPeaksCurves();

  void runSaveDiffFittingAsciiAlg(const std::string &tableWorkspace,
                                  std::string &filePath);

  void runEvaluateFunctionAlg(const std::string &bk2BkExpFunction,
                              const std::string &InputName,
                              const std::string &OutputName,
                              const std::string &startX,
                              const std::string &endX);

  void runCropWorkspaceAlg(std::string workspaceName);

  void runAppendSpectraAlg(std::string workspace1Name,
                           std::string workspace2Name);

  void runRebinToWorkspaceAlg(std::string workspaceName);

  void convertUnits(std::string workspaceName);
  void runConvertUnitsAlg(std::string workspaceName);
  void runAlignDetectorsAlg(std::string workspaceName);

  void setDifcTzero(Mantid::API::MatrixWorkspace_sptr wks) const;
  void getDifcTzero(Mantid::API::MatrixWorkspace_const_sptr wks, double &difc,
                    double &difa, double &tzero) const;

  void runCloneWorkspaceAlg(std::string inputWorkspace,
                            const std::string &outputWorkspace);

  void setDataToClonedWS(std::string &current_WS, const std::string &cloned_WS);

  void setBankItems(const std::vector<std::string> &bankFiles);

  void setRunNoItems(const std::vector<std::string> &runNumVector,
                     bool multiRun);

  void setDefaultBank(const std::vector<std::string> &splittedBaseName,
                      const std::string &selectedFile);

protected:
  void processStart();
  void processLoad();
  void processFitPeaks();
  void processFitAllPeaks();
  void processShutDown();
  void processLogMsg();

  /// clean shut down of model, view, etc.
  void cleanup();

protected slots:

  void fittingFinished();
  void fittingRunNoChanged();

private:
  bool isDigit(const std::string text) const;

  // Methods related single peak fits
  virtual void
  startAsyncFittingWorker(const std::vector<std::string> &focusedRunNo,
                          const std::string &expectedPeaks);

  std::string getBaseNameFromStr(const std::string &filePath) const;

  std::string validateFittingexpectedPeaks(std::string &expectedPeaks) const;

  void inputChecksBeforeFitting(const std::string &focusedRunNo,
                                const std::string &expectedPeaks);

  // TODO make this const when the global is removed
  bool findFilePathFromBaseName(const std::string &directoryToSearch,
                                const std::string &baseFileNamesToFind,
                                std::vector<std::string> &foundFullFilePath);

  std::vector<std::string>
  splitFittingDirectory(const std::string &selectedfPath);

  std::vector<std::string> enableMultiRun(const std::string &firstRun,
                                          const std::string &lastRun);

  void browsePeaksToFit();

  void addPeakToList();

  void savePeakList();

  std::string readPeaksFile(std::string fileDir);

  void fittingWriteFile(const std::string &fileDir);

  std::vector<std::string>
  getAllBrowsedFilePaths(const std::string &inputFullPath,
                         std::vector<std::string> &foundFullFilePaths);

  std::vector<std::string> processMultiRun(const std::string userInput);

  std::vector<std::string>
  processSingleRun(const std::string &userInputBasename,
                   const std::vector<std::string> &splitBaseName);

  std::vector<std::string>
  processFullPathInput(const Poco::Path &pocoFilePath,
                       const std::vector<std::string> &splitBaseName);

  // whether to use AlignDetectors to convert units
  static const bool g_useAlignDetectors;

  static int g_fitting_runno_counter;

  // name of the workspace with the focused ws being used for fitting
  static const std::string g_focusedFittingWSName;

  // input run number - used for output file name
  std::vector<std::string> g_multi_run;

  // Holds the previous user input so we can short circuit further checks
  std::string m_previousInput;

  /// true if the last fitting completed successfully
  bool m_fittingFinishedOK;

  // directories of all the run numbers when multi-run option
  std::vector<std::string> g_multi_run_directories;

  QThread *m_workerThread;

  /// interface for the 'current' calibration
  boost::shared_ptr<IEnggDiffractionCalibration> m_mainCalib;

  /// interface for the 'current' calibration
  boost::shared_ptr<IEnggDiffractionParam> m_mainParam;

  /// Associated view for this presenter (MVP pattern)
  IEnggDiffFittingView *const m_view;

  /// Holds if the view is in the process of being closed
  bool m_viewHasClosed;
};

} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGPRESENTER_H_
