#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFRACTIONPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFRACTIONPRESENTER_H_

#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "DllConfig.h"
#include "IEnggDiffractionCalibration.h"
#include "IEnggDiffractionParam.h"
#include "IEnggDiffractionPresenter.h"
#include "IEnggDiffractionView.h"

#include <boost/scoped_ptr.hpp>

#include <QObject>

namespace Poco {
class Path;
}

class QThread;

namespace MantidQt {
namespace CustomInterfaces {

/**
Presenter for the Enggineering Diffraction GUI (presenter as in the
MVP Model-View-Presenter pattern). In principle, in a strict MVP
setup, signals from the model should always be handled through this
presenter and never go directly to the view, and viceversa.

Copyright &copy; 2015-2016 ISIS Rutherford Appleton Laboratory, NScD
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
class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffractionPresenter
    : public QObject,
      public IEnggDiffractionPresenter,
      public IEnggDiffractionCalibration,
      public IEnggDiffractionParam {
  // Q_OBJECT for 'connect' with thread/worker
  Q_OBJECT

public:
  EnggDiffractionPresenter(IEnggDiffractionView *view);
  ~EnggDiffractionPresenter() override;

  void notify(IEnggDiffractionPresenter::Notification notif) override;

  /// the calibration hard work that a worker / thread will run
  void doNewCalibration(const std::string &outFilename,
                        const std::string &vanNo, const std::string &ceriaNo,
                        const std::string &specNos);

  /// the focusing hard work that a worker / thread will run
  void doFocusRun(const std::string &dir, const std::string &runNo,
                  const std::vector<bool> &banks, const std::string &specNos,
                  const std::string &dgFile);

  /// checks if its a valid run number returns string
  std::string isValidRunNumber(const std::vector<std::string> &dir);

  /// checks if its a valid run number inside vector and returns a vector;
  /// used for mutli-run focusing, and other multi-run file selections
  std::vector<std::string>
  isValidMultiRunNumber(const std::vector<std::string> &dir);

  /// pre-processing re-binning with Rebin, for a worker/thread
  void doRebinningTime(const std::string &runNo, double bin,
                       const std::string &outWSName);

  /// pre-processing re-binning with RebinByPulseTimes, for a worker/thread
  void doRebinningPulses(const std::string &runNo, size_t nperiods, double bin,
                         const std::string &outWSName);

protected:
  void initialize();

  /// clean shut down of model, view, etc.
  void cleanup();

  void processStart();
  void processLoadExistingCalib();
  void processCalcCalib();
  void ProcessCropCalib();
  void processFocusBasic();
  void processFocusCropped();
  void processFocusTexture();
  void processResetFocus();
  void processRebinTime();
  void processRebinMultiperiod();
  void processLogMsg();
  void processInstChange();
  void processRBNumberChange();
  void processShutDown();
  void processStopFocus();

protected slots:
  void calibrationFinished();
  void focusingFinished();
  void rebinningFinished();

private:
  bool validateRBNumber(const std::string &rbn) const;

  /// @name Calibration related private methods
  //@{
  void inputChecksBeforeCalibrate(const std::string &newVanNo,
                                  const std::string &newCeriaNo);

  std::string outputCalibFilename(const std::string &vanNo,
                                  const std::string &ceriaNo,
                                  const std::string &bankName = "");

  void updateNewCalib(const std::string &fname);

  void parseCalibrateFilename(const std::string &path, std::string &instName,
                              std::string &vanNo, std::string &ceriaNo);

  void grabCalibParms(const std::string &fname);

  void updateCalibParmsTable();

  // this may need to be mocked up in tests
  virtual void startAsyncCalibWorker(const std::string &outFilename,
                                     const std::string &vanNo,
                                     const std::string &ceriaNo,
                                     const std::string &specNos);

  void doCalib(const EnggDiffCalibSettings &cs, const std::string &vanNo,
               const std::string &ceriaNo, const std::string &outFilename,
               const std::string &specNos);

  void appendCalibInstPrefix(const std::string &vanNo,
                             std::string &outVanName) const;

  void appendCalibInstPrefix(const std::string &vanNo, const std::string &cerNo,
                             std::string &outVanName,
                             std::string &outCerName) const;

  std::string
  buildCalibrateSuggestedFilename(const std::string &vanNo,
                                  const std::string &ceriaNo,
                                  const std::string &bankName = "") const;

  std::vector<GSASCalibrationParms> currentCalibration() const override;
  //@}

  /// @name Focusing related private methods
  //@{
  /// this may also need to be mocked up in tests
  void startFocusing(const std::vector<std::string> &multi_runNo,
                     const std::vector<bool> &banks,
                     const std::string &specNos = "",
                     const std::string &dgFile = "");

  virtual void
  startAsyncFocusWorker(const std::string &dir,
                        const std::vector<std::string> &multi_RunNo,
                        const std::vector<bool> &banks,
                        const std::string &specNos, const std::string &dgFile);

  void inputChecksBeforeFocusBasic(const std::vector<std::string> &multi_RunNo,
                                   const std::vector<bool> &banks);
  void
  inputChecksBeforeFocusCropped(const std::vector<std::string> &multi_RunNo,
                                const std::vector<bool> &banks,
                                const std::string &specNos);
  void
  inputChecksBeforeFocusTexture(const std::vector<std::string> &multi_RunNo,
                                const std::string &dgfile);
  void inputChecksBeforeFocus();
  void inputChecksBanks(const std::vector<bool> &banks);

  std::vector<std::string> outputFocusFilenames(const std::string &runNo,
                                                const std::vector<bool> &banks);

  std::string outputFocusCroppedFilename(const std::string &runNo);

  std::vector<std::string> sumOfFilesLoadVec();

  std::vector<std::string>
  outputFocusTextureFilenames(const std::string &runNo,
                              const std::vector<size_t> &bankIDs);

  void loadDetectorGroupingCSV(const std::string &dgFile,
                               std::vector<size_t> &bankIDs,
                               std::vector<std::string> &specs);

  void doFocusing(const EnggDiffCalibSettings &cs,
                  const std::string &fullFilename, const std::string &runNo,
                  size_t bank, const std::string &specNos,
                  const std::string &dgFile);

  void
  loadOrCalcVanadiumWorkspaces(const std::string &vanNo,
                               const std::string &inputDirCalib,
                               Mantid::API::ITableWorkspace_sptr &vanIntegWS,
                               Mantid::API::MatrixWorkspace_sptr &vanCurvesWS,
                               bool forceRecalc, const std::string &specNos);

  void findPrecalcVanadiumCorrFilenames(const std::string &vanNo,
                                        const std::string &inputDirCalib,
                                        std::string &preIntegFilename,
                                        std::string &preCurvesFilename,
                                        bool &found);

  void loadVanadiumPrecalcWorkspaces(
      const std::string &preIntegFilename, const std::string &preCurvesFilename,
      Mantid::API::ITableWorkspace_sptr &vanIntegWS,
      Mantid::API::MatrixWorkspace_sptr &vanCurvesWS, const std::string &vanNo,
      const std::string &specNos);

  void calcVanadiumWorkspaces(const std::string &vanNo,
                              Mantid::API::ITableWorkspace_sptr &vanIntegWS,
                              Mantid::API::MatrixWorkspace_sptr &vanCurvesWS);

  /// @name Methods related to pre-processing / re-binning
  //@{
  void inputChecksBeforeRebin(const std::string &runNo);

  void inputChecksBeforeRebinTime(const std::string &runNo, double bin);

  void inputChecksBeforeRebinPulses(const std::string &runNo, size_t nperiods,
                                    double timeStep);

  Mantid::API::Workspace_sptr loadToPreproc(const std::string &runNo);

  virtual void startAsyncRebinningTimeWorker(const std::string &runNo,
                                             double bin,
                                             const std::string &outWSName);

  virtual void startAsyncRebinningPulsesWorker(const std::string &runNo,
                                               size_t nperiods, double timeStep,
                                               const std::string &outWSName);
  //@}

  // plots workspace according to the user selection
  void plotFocusedWorkspace(std::string outWSName);

  void plotCalibWorkspace(std::vector<double> difc, std::vector<double> tzero,
                          std::string specNos);

  // algorithms to save the generated workspace
  void saveGSS(std::string inputWorkspace, std::string bank, std::string runNo);
  void saveFocusedXYE(std::string inputWorkspace, std::string bank,
                      std::string runNo);
  void saveOpenGenie(std::string inputWorkspace, std::string bank,
                     std::string runNo);

  // generates the required file name of the output files
  std::string outFileNameFactory(std::string inputWorkspace, std::string runNo,
                                 std::string bank, std::string format);

  // returns a directory as a path, creating it if not found, and checking
  // errors
  Poco::Path outFilesUserDir(const std::string &addToDir) override;
  Poco::Path outFilesGeneralDir(const std::string &addComponent);
  Poco::Path outFilesRootDir();

  std::string appendToPath(const std::string &path,
                           const std::string &toAppend) const;

  /// convenience methods to copy files to different destinations
  void copyToGeneral(const Poco::Path &source, const std::string &pathComp);
  void copyToUser(const Poco::Path &source, const std::string &pathComp);
  void copyFocusedToUserAndAll(const std::string &fullFilename);

  // generates appropriate names for table workspaces
  std::string outFitParamsTblNameGenerator(const std::string &specNos,
                                           size_t bank_i) const;

  // generates the pycode string which can be passed to view
  std::string vanadiumCurvesPlotFactory();

  std::string DifcZeroWorkspaceFactory(
      const std::vector<double> &difc, const std::vector<double> &tzero,
      const std::string &specNo, const std::string &customisedBankName) const;

  std::string
  plotDifcZeroWorkspace(const std::string &customisedBankName) const;

  void writeOutCalibFile(const std::string &outFilename,
                         const std::vector<double> &difc,
                         const std::vector<double> &tzero,
                         const std::vector<std::string> &bankNames,
                         const std::string &ceriaNo, const std::string &vanNo,
                         const std::string &templateFile = "");

  /// keep track of the paths the user "browses to", to add them in
  /// the file search path
  void recordPathBrowsedTo(const std::string &filename);

  /// paths the user has "browsed to", to add them to the search path
  std::vector<std::string> m_browsedToPaths;

  /// The message to tell the user that an RB number is needed
  const static std::string g_shortMsgRBNumberRequired;
  const static std::string g_msgRBNumberRequired;

  /// string to use for invalid run number error message
  const static std::string g_runNumberErrorStr;

  // name of the workspace with the vanadium integration (of spectra)
  static const std::string g_vanIntegrationWSName;

  // name of the workspace with the vanadium (smoothed) curves
  static const std::string g_vanCurvesWSName;

  // for the GSAS parameters (difc, difa, tzero) of the banks
  static const std::string g_calibBanksParms;

  /// whether to allow users to give the output calibration filename
  const static bool g_askUserCalibFilename;

  /// whether to break the thread
  static bool g_abortThread;

  /// whether to run Sum Of Files & which focus run number to use
  static std::string g_sumOfFilesFocus;

  /// saves the last valid run number
  static std::string g_lastValidRun;

  /// bank name used or SpecNos for cropped calibration
  static std::string g_calibCropIdentifier;

  QThread *m_workerThread;

  /// true if the last calibration completed successfully
  bool m_calibFinishedOK;
  /// path where the calibration has been produced (par/prm file)
  std::string m_calibFullPath;

  /// The current calibration parameters (used for units conversion). It should
  /// be updated when a new calibration is done or re-loading an existing one
  std::vector<GSASCalibrationParms> m_currentCalibParms;

  /// true if the last focusing completed successfully
  bool m_focusFinishedOK;
  /// true if the last pre-processing/re-binning completed successfully
  bool m_rebinningFinishedOK;

  /// Counter for the cropped output files
  static int g_croppedCounter;

  /// counter for the plotting workspace
  static int g_plottingCounter;

  /// Associated view for this presenter (MVP pattern)
  IEnggDiffractionView *const m_view;

  /// Tracks if the view has started to shut down following a close signal
  bool m_viewHasClosed;

  /// Associated model for this presenter (MVP pattern)
  // const boost::scoped_ptr<EnggDiffractionModel> m_model;

  /// the current selected instrument
  std::string m_currentInst = "";
};

} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFRACTIONPRESENTER_H_
