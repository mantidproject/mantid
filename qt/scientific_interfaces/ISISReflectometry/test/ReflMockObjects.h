// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Batch/IBatchJobAlgorithm.h"
#include "GUI/Batch/IBatchJobManager.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "GUI/Batch/IBatchPresenterFactory.h"
#include "GUI/Common/IDecoder.h"
#include "GUI/Common/IEncoder.h"
#include "GUI/Common/IFileHandler.h"
#include "GUI/Common/IJobManager.h"
#include "GUI/Common/IPlotter.h"
#include "GUI/Common/IPythonRunner.h"
#include "GUI/Common/IReflMessageHandler.h"
#include "GUI/Event/IEventPresenter.h"
#include "GUI/Experiment/IExperimentPresenter.h"
#include "GUI/Instrument/IInstrumentPresenter.h"
#include "GUI/Instrument/InstrumentOptionDefaults.h"
#include "GUI/MainWindow/IMainWindowPresenter.h"
#include "GUI/MainWindow/IMainWindowView.h"
#include "GUI/Runs/IRunNotifier.h"
#include "GUI/Runs/IRunsPresenter.h"
#include "GUI/Runs/ISearchModel.h"
#include "GUI/Runs/ISearcher.h"
#include "GUI/Runs/SearchCriteria.h"
#include "GUI/Save/IFileSaver.h"
#include "GUI/Save/ISavePresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidFrameworkTestHelpers/FallbackBoostOptionalIO.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/Hint.h"
#include "Reduction/PreviewRow.h"

#include <QMap>
#include <QString>
#include <QVariant>
#include <gmock/gmock.h>
#include <memory>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace Mantid::API;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/**** Factories ****/
class MockBatchPresenterFactory : public IBatchPresenterFactory {
public:
  MOCK_METHOD1(makeProxy, IBatchPresenter *(IBatchView *));
  std::unique_ptr<IBatchPresenter> make(IBatchView *view) { return std::unique_ptr<IBatchPresenter>(makeProxy(view)); }
};

/**** Presenters ****/

class MockBatchPresenter : public IBatchPresenter {
public:
  MOCK_METHOD1(acceptMainPresenter, void(IMainWindowPresenter *));
  MOCK_METHOD1(initInstrumentList, std::string(const std::string &));
  MOCK_METHOD0(notifyResumeReductionRequested, void());
  MOCK_METHOD0(notifyPauseReductionRequested, void());
  MOCK_METHOD0(notifyResumeAutoreductionRequested, void());
  MOCK_METHOD0(notifyPauseAutoreductionRequested, void());
  MOCK_METHOD0(notifyAutoreductionCompleted, void());
  MOCK_METHOD0(notifyAnyBatchReductionResumed, void());
  MOCK_METHOD0(notifyAnyBatchReductionPaused, void());
  MOCK_METHOD0(notifyAnyBatchAutoreductionResumed, void());
  MOCK_METHOD0(notifyAnyBatchAutoreductionPaused, void());
  MOCK_METHOD0(notifyReductionPaused, void());

  MOCK_METHOD1(notifyChangeInstrumentRequested, void(const std::string &));
  MOCK_METHOD1(notifyInstrumentChanged, void(const std::string &));
  MOCK_METHOD0(notifyUpdateInstrumentRequested, void());
  MOCK_METHOD0(notifyRestoreDefaultsRequested, void());
  MOCK_METHOD0(notifySettingsChanged, void());
  MOCK_METHOD1(notifySetRoundPrecision, void(int &));
  MOCK_METHOD0(notifyResetRoundPrecision, void());
  MOCK_METHOD0(notifyBatchLoaded, void());
  MOCK_METHOD1(notifyRowContentChanged, void(Row &));
  MOCK_METHOD1(notifyGroupNameChanged, void(Group &));
  MOCK_METHOD0(notifyRunsTransferred, void());

  MOCK_CONST_METHOD0(isProcessing, bool());
  MOCK_CONST_METHOD0(isAutoreducing, bool());
  MOCK_CONST_METHOD0(isAnyBatchProcessing, bool());
  MOCK_CONST_METHOD0(isAnyBatchAutoreducing, bool());
  MOCK_CONST_METHOD0(isOverwriteBatchPrevented, bool());
  MOCK_CONST_METHOD1(discardChanges, bool(std::string const &));
  MOCK_CONST_METHOD0(getUnsavedBatchFlag, bool());
  MOCK_METHOD1(setUnsavedBatchFlag, void(bool));
  MOCK_CONST_METHOD0(percentComplete, int());
  MOCK_CONST_METHOD0(rowProcessingProperties, std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps>());
  MOCK_CONST_METHOD0(requestClose, bool());
  MOCK_CONST_METHOD0(instrument, Mantid::Geometry::Instrument_const_sptr());
  MOCK_CONST_METHOD0(instrumentName, std::string());
  MOCK_CONST_METHOD0(isBatchUnsaved, bool());
  MOCK_METHOD0(setBatchUnsaved, void());
  MOCK_METHOD0(notifyChangesSaved, void());
  MOCK_METHOD0(notifyPreviewApplyRequested, void());
  MOCK_CONST_METHOD0(hasROIDetectorIDsForPreviewRow, bool());
};

class MockRunsPresenter : public IRunsPresenter {
public:
  MOCK_METHOD1(acceptMainPresenter, void(IBatchPresenter *));
  MOCK_METHOD1(initInstrumentList, std::string(const std::string &));
  MOCK_CONST_METHOD0(runsTable, RunsTable const &());
  MOCK_METHOD0(mutableRunsTable, RunsTable &());
  MOCK_METHOD1(notifyChangeInstrumentRequested, bool(std::string const &));
  MOCK_METHOD0(notifyResumeReductionRequested, void());
  MOCK_METHOD0(notifyPauseReductionRequested, void());
  MOCK_METHOD0(notifyRowStateChanged, void());
  MOCK_METHOD1(notifyRowStateChanged, void(boost::optional<Item const &>));
  MOCK_METHOD0(notifyRowModelChanged, void());
  MOCK_METHOD1(notifyRowModelChanged, void(boost::optional<Item const &>));
  MOCK_METHOD0(notifyReductionPaused, void());
  MOCK_METHOD0(notifyReductionResumed, void());
  MOCK_METHOD0(resumeAutoreduction, bool());
  MOCK_METHOD0(notifyAutoreductionPaused, void());
  MOCK_METHOD0(notifyAutoreductionResumed, void());
  MOCK_METHOD0(autoreductionCompleted, void());
  MOCK_METHOD0(notifyAnyBatchReductionPaused, void());
  MOCK_METHOD0(notifyAnyBatchReductionResumed, void());
  MOCK_METHOD0(notifyAnyBatchAutoreductionPaused, void());
  MOCK_METHOD0(notifyAnyBatchAutoreductionResumed, void());
  MOCK_METHOD1(notifyInstrumentChanged, void(std::string const &));
  MOCK_METHOD0(notifyTableChanged, void());
  MOCK_METHOD1(notifyRowContentChanged, void(Row &));
  MOCK_METHOD1(notifyGroupNameChanged, void(Group &));
  MOCK_METHOD0(settingsChanged, void());
  MOCK_METHOD0(notifyChangesSaved, void());
  MOCK_METHOD0(notifyBatchLoaded, void());
  MOCK_CONST_METHOD0(hasUnsavedChanges, bool());
  MOCK_CONST_METHOD0(isAnyBatchProcessing, bool());
  MOCK_CONST_METHOD0(isAnyBatchAutoreducing, bool());
  MOCK_CONST_METHOD0(isOperationPrevented, bool());
  MOCK_CONST_METHOD0(isProcessing, bool());
  MOCK_CONST_METHOD0(isAutoreducing, bool());
  MOCK_CONST_METHOD0(isOverwritingTablePrevented, bool());
  MOCK_CONST_METHOD0(isOverwriteBatchPrevented, bool());
  MOCK_CONST_METHOD0(percentComplete, int());
  MOCK_METHOD1(setRoundPrecision, void(int &));
  MOCK_METHOD0(resetRoundPrecision, void());
  MOCK_METHOD0(notifySearchComplete, void());
  MOCK_CONST_METHOD0(instrumentName, std::string());
};

class MockEventPresenter : public IEventPresenter {
public:
  MOCK_METHOD1(acceptMainPresenter, void(IBatchPresenter *));
  MOCK_METHOD0(notifyReductionPaused, void());
  MOCK_METHOD0(notifyReductionResumed, void());
  MOCK_METHOD0(notifyAutoreductionPaused, void());
  MOCK_METHOD0(notifyAutoreductionResumed, void());
  MOCK_CONST_METHOD0(slicing, Slicing &());
};

class MockExperimentPresenter : public IExperimentPresenter {
public:
  MOCK_METHOD1(acceptMainPresenter, void(IBatchPresenter *));
  MOCK_CONST_METHOD0(experiment, Experiment const &());
  MOCK_METHOD0(notifyReductionPaused, void());
  MOCK_METHOD0(notifyReductionResumed, void());
  MOCK_METHOD0(notifyAutoreductionPaused, void());
  MOCK_METHOD0(notifyAutoreductionResumed, void());
  MOCK_METHOD1(notifyInstrumentChanged, void(std::string const &));
  MOCK_METHOD1(notifyPreviewApplyRequested, void(PreviewRow const &));
  MOCK_METHOD0(restoreDefaults, void());
  MOCK_METHOD(bool, hasValidSettings, (), (const, noexcept, override));
};

class MockInstrumentPresenter : public IInstrumentPresenter {
public:
  MOCK_METHOD1(acceptMainPresenter, void(IBatchPresenter *));
  MOCK_CONST_METHOD0(instrument, Instrument const &());
  MOCK_METHOD0(notifyReductionPaused, void());
  MOCK_METHOD0(notifyReductionResumed, void());
  MOCK_METHOD0(notifyAutoreductionPaused, void());
  MOCK_METHOD0(notifyAutoreductionResumed, void());
  MOCK_METHOD1(notifyInstrumentChanged, void(std::string const &));
  MOCK_METHOD0(restoreDefaults, void());
};

class MockSavePresenter : public ISavePresenter {
public:
  MOCK_METHOD1(acceptMainPresenter, void(IBatchPresenter *));
  MOCK_METHOD2(saveWorkspaces, void(std::vector<std::string> const &, bool const));
  MOCK_CONST_METHOD0(shouldAutosave, bool());
  MOCK_CONST_METHOD0(shouldAutosaveGroupRows, bool());
  MOCK_METHOD0(notifyReductionPaused, void());
  MOCK_METHOD0(notifyReductionResumed, void());
  MOCK_METHOD0(notifyAutoreductionPaused, void());
  MOCK_METHOD0(notifyAutoreductionResumed, void());
};

/**** Progress ****/

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  MOCK_METHOD1(doReport, void(const std::string &));
  ~MockProgressBase() override = default;
};

/**** Catalog ****/

class MockICatalogInfo : public Mantid::Kernel::ICatalogInfo {
public:
  MOCK_CONST_METHOD0(catalogName, const std::string());
  MOCK_CONST_METHOD0(soapEndPoint, const std::string());
  MOCK_CONST_METHOD0(externalDownloadURL, const std::string());
  MOCK_CONST_METHOD0(catalogPrefix, const std::string());
  MOCK_CONST_METHOD0(windowsPrefix, const std::string());
  MOCK_CONST_METHOD0(macPrefix, const std::string());
  MOCK_CONST_METHOD0(linuxPrefix, const std::string());
  MOCK_CONST_METHOD0(clone, ICatalogInfo *());
  MOCK_CONST_METHOD1(transformArchivePath, std::string(const std::string &));
  ~MockICatalogInfo() override = default;
};

class MockSearcher : public ISearcher {
public:
  MOCK_METHOD1(subscribe, void(SearcherSubscriber *notifyee));
  MOCK_METHOD1(search, SearchResults(SearchCriteria));
  MOCK_METHOD1(startSearchAsync, bool(SearchCriteria));
  MOCK_CONST_METHOD0(searchInProgress, bool());
  MOCK_CONST_METHOD1(getSearchResult, SearchResult const &(int));
  MOCK_METHOD0(reset, void());
  MOCK_CONST_METHOD0(hasUnsavedChanges, bool());
  MOCK_METHOD0(setSaved, void());
  MOCK_CONST_METHOD0(searchCriteria, SearchCriteria());
  MOCK_CONST_METHOD0(getSearchResultsCSV, std::string());
};

class MockSearcherSubscriber : public SearcherSubscriber {
public:
  MOCK_METHOD0(notifySearchComplete, void());
  MOCK_METHOD0(notifySearchFailed, void());
};

class MockRunNotifier : public IRunNotifier {
public:
  MOCK_METHOD1(subscribe, void(RunNotifierSubscriber *));
  MOCK_METHOD0(startPolling, void());
  MOCK_METHOD0(stopPolling, void());
};

class MockRunNotifierSubscriber : public RunNotifierSubscriber {
public:
  MOCK_METHOD0(notifyCheckForNewRuns, void());
};

class MockSearchModel : public ISearchModel {
public:
  MOCK_METHOD1(mergeNewResults, void(SearchResults const &));
  MOCK_METHOD1(replaceResults, void(SearchResults const &));
  MOCK_CONST_METHOD1(getRowData, SearchResult const &(int));
  MOCK_CONST_METHOD0(getRows, SearchResults const &());
  MOCK_METHOD0(clear, void());
  MOCK_CONST_METHOD0(hasUnsavedChanges, bool());
  MOCK_METHOD0(setUnsaved, void());
  MOCK_METHOD0(setSaved, void());
  MOCK_CONST_METHOD0(getSearchResultsCSV, std::string());
};

class MockMessageHandler : public IReflMessageHandler {
public:
  MOCK_METHOD2(giveUserCritical, void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserWarning, void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserInfo, void(const std::string &, const std::string &));
  MOCK_METHOD2(askUserOkCancel, bool(const std::string &, const std::string &));
  MOCK_METHOD1(askUserForLoadFileName, std::string(const std::string &));
  MOCK_METHOD1(askUserForSaveFileName, std::string(const std::string &));
};

class MockFileHandler : public IFileHandler {
public:
  MOCK_METHOD2(saveJSONToFile, void(std::string const &, QMap<QString, QVariant> const &));
  MOCK_METHOD1(loadJSONFromFile, QMap<QString, QVariant>(const std::string &));
  MOCK_CONST_METHOD2(saveCSVToFile, void(std::string const &, std::string const &));
  MOCK_CONST_METHOD1(fileExists, bool(std::string const &));
  MOCK_CONST_METHOD1(getFullFilePath, std::string(std::string const &));
};

class MockJobManager : public IJobManager {
public:
  MOCK_METHOD1(subscribe, void(JobManagerSubscriber *notifyee));
  MOCK_METHOD1(startPreprocessing, void(PreviewRow &row));
  MOCK_METHOD1(startSumBanks, void(PreviewRow &row));
  MOCK_METHOD1(startReduction, void(PreviewRow &row));
};

class MockJobManagerSubscriber : public JobManagerSubscriber {
public:
  MOCK_METHOD0(notifyLoadWorkspaceCompleted, void());
  MOCK_METHOD0(notifySumBanksCompleted, void());
  MOCK_METHOD0(notifyReductionCompleted, void());
  MOCK_METHOD0(notifyLoadWorkspaceAlgorithmError, void());
  MOCK_METHOD0(notifySumBanksAlgorithmError, void());
  MOCK_METHOD0(notifyReductionAlgorithmError, void());
};

class MockEncoder : public IEncoder {
public:
  MOCK_METHOD3(encodeBatch, QMap<QString, QVariant>(const IMainWindowView *, int, bool));
};

class MockDecoder : public IDecoder {
public:
  MOCK_METHOD3(decodeBatch, void(const IMainWindowView *, int, const QMap<QString, QVariant> &));
  MOCK_CONST_METHOD1(decodeVersion, size_t(const QMap<QString, QVariant> &));
};

class MockPythonRunner : public IPythonRunner {
public:
  MOCK_METHOD1(runPythonAlgorithm, std::string(const std::string &));
};

class MockPlotter : public IPlotter {
public:
  MOCK_CONST_METHOD1(reflectometryPlot, void(const std::vector<std::string> &));
};

/**** Saver ****/
class MockFileSaver : public IFileSaver {
public:
  MOCK_CONST_METHOD1(isValidSaveDirectory, bool(std::string const &));
  MOCK_CONST_METHOD4(save, void(std::string const &, std::vector<std::string> const &, std::vector<std::string> const &,
                                FileFormatOptions const &));
  virtual ~MockFileSaver() = default;
};

/**** Job runner ****/

class MockBatchJobManager : public IBatchJobManager {
public:
  MockBatchJobManager() {};
  MOCK_CONST_METHOD0(isProcessing, bool());
  MOCK_CONST_METHOD0(isAutoreducing, bool());
  MOCK_CONST_METHOD0(percentComplete, int());
  MOCK_METHOD0(notifyReductionResumed, void());
  MOCK_METHOD0(notifyReductionPaused, void());
  MOCK_METHOD0(notifyAutoreductionResumed, void());
  MOCK_METHOD0(notifyAutoreductionPaused, void());
  MOCK_METHOD1(setReprocessFailedItems, void(bool));
  MOCK_METHOD1(getRunsTableItem, boost::optional<Item &>(MantidQt::API::IConfiguredAlgorithm_sptr const &algorithm));
  MOCK_METHOD1(algorithmStarted, void(MantidQt::API::IConfiguredAlgorithm_sptr));
  MOCK_METHOD1(algorithmComplete, void(MantidQt::API::IConfiguredAlgorithm_sptr));
  MOCK_METHOD2(algorithmError, void(MantidQt::API::IConfiguredAlgorithm_sptr, std::string const &));
  MOCK_CONST_METHOD2(algorithmOutputWorkspacesToSave,
                     std::vector<std::string>(MantidQt::API::IConfiguredAlgorithm_sptr, bool));
  MOCK_METHOD1(notifyWorkspaceDeleted, boost::optional<Item const &>(std::string const &));
  MOCK_METHOD2(notifyWorkspaceRenamed, boost::optional<Item const &>(std::string const &, std::string const &));
  MOCK_METHOD0(notifyAllWorkspacesDeleted, void());
  MOCK_METHOD0(getAlgorithms, std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>());
  MOCK_CONST_METHOD0(rowProcessingProperties, std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps>());
  MOCK_CONST_METHOD0(getProcessPartial, bool());
  MOCK_CONST_METHOD0(getProcessAll, bool());
};

class MockBatchJobAlgorithm : public IBatchJobAlgorithm, public MantidQt::API::IConfiguredAlgorithm {
public:
  MockBatchJobAlgorithm() {}
  MOCK_CONST_METHOD0(algorithm, Mantid::API::IAlgorithm_sptr());
  MOCK_METHOD((const Mantid::API::IAlgorithmRuntimeProps &), getAlgorithmRuntimeProps, (), (const, override, noexcept));
  MOCK_METHOD((bool), validatePropsPreExec, (), (const, override, noexcept));
  MOCK_METHOD0(item, Item *());
  MOCK_METHOD0(updateItem, void());
  MOCK_CONST_METHOD0(outputWorkspaceNames, std::vector<std::string>());
  MOCK_CONST_METHOD0(outputWorkspaceNameToWorkspace, std::map<std::string, Workspace_sptr>());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
