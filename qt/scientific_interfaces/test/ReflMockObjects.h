// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H

#include "../ISISReflectometry/IReflAsciiSaver.h"
#include "../ISISReflectometry/IReflAutoreduction.h"
#include "../ISISReflectometry/IReflBatchPresenter.h"
#include "../ISISReflectometry/IReflMainWindowPresenter.h"
#include "../ISISReflectometry/IReflMainWindowView.h"
#include "../ISISReflectometry/IReflMessageHandler.h"
#include "../ISISReflectometry/IReflSearcher.h"
#include "../ISISReflectometry/InstrumentOptionDefaults.h"
#include "../ISISReflectometry/ReflSearchModel.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/DataProcessorUI/Command.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "MantidQtWidgets/Common/Hint.h"
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::DataProcessor;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/**** Models ****/

class MockReflSearchModel : public ReflSearchModel {
public:
  MockReflSearchModel(std::string const &run, std::string const &description,
                      std::string const &location)
      : ReflSearchModel(ITableWorkspace_sptr(), std::string()),
        m_result(run, description, location) {}
  ~MockReflSearchModel() override {}
  MOCK_CONST_METHOD2(data, QVariant(const QModelIndex &, int role));
  MOCK_METHOD2(setError, void(int, std::string const &));

  SearchResult const &getRowData(int) const override { return m_result; }

private:
  SearchResult m_result;
};

/**** Views ****/

class MockMainWindowView : public IReflMainWindowView {
public:
  MOCK_METHOD3(askUserString,
               std::string(const std::string &, const std::string &,
                           const std::string &));
  MOCK_METHOD2(askUserYesNo, bool(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserWarning, void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserCritical,
               void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserInfo, void(const std::string &, const std::string &));
  MOCK_METHOD1(runPythonAlgorithm, std::string(const std::string &));
  ~MockMainWindowView() override{};
};

/**** Presenters ****/

class MockMainWindowPresenter : public IReflMainWindowPresenter {
public:
  MOCK_METHOD1(runPythonAlgorithm, std::string(const std::string &));
  MOCK_METHOD1(settingsChanged, void(int));
  bool isProcessing() const override { return false; }

  ~MockMainWindowPresenter() override{};
};

class MockReflBatchPresenter : public IReflBatchPresenter {
public:
  MOCK_METHOD0(notifyReductionPaused, void());
  MOCK_METHOD0(notifyReductionResumed, void());
  MOCK_METHOD2(notifyReductionCompletedForGroup,
               void(GroupData const &, std::string const &));
  MOCK_METHOD2(notifyReductionCompletedForRow,
               void(GroupData const &, std::string const &));

  /// Transmission runs for a specific run angle
  MOCK_CONST_METHOD1(getOptionsForAngle, OptionsQMap(const double));
  /// Whether there are per-angle transmission runs specified
  MOCK_CONST_METHOD0(hasPerAngleOptions, bool());
  /// Set the instrument name
  MOCK_CONST_METHOD1(notifyInstrumentChanged, void(const std::string &));
  /// Data processing check for all groups
  MOCK_CONST_METHOD0(isProcessing, bool());
  MOCK_CONST_METHOD0(requestClose, bool());
  MOCK_METHOD0(settingsChanged, void());
};

/**** Progress ****/

class MockProgressBase : public Mantid::Kernel::ProgressBase {
public:
  MOCK_METHOD1(doReport, void(const std::string &));
  ~MockProgressBase() override {}
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
  ~MockICatalogInfo() override {}
};

class MockReflAsciiSaver : public IReflAsciiSaver {
public:
  MOCK_CONST_METHOD1(isValidSaveDirectory, bool(std::string const &));
  MOCK_CONST_METHOD4(save,
                     void(std::string const &, std::vector<std::string> const &,
                          std::vector<std::string> const &,
                          FileFormatOptions const &));
  virtual ~MockReflAsciiSaver() = default;
};

class MockReflSearcher : public IReflSearcher {
public:
  MOCK_METHOD1(search, Mantid::API::ITableWorkspace_sptr(const std::string &));
};

/**** Catalog ****/
class MockMessageHandler : public IReflMessageHandler {
public:
  MOCK_METHOD2(giveUserCritical,
               void(const std::string &, const std::string &));
  MOCK_METHOD2(giveUserInfo, void(const std::string &, const std::string &));
};

/**** Autoreduction ****/
class MockReflAutoreduction : public IReflAutoreduction {
public:
  MOCK_CONST_METHOD0(running, bool());
  MOCK_CONST_METHOD1(searchStringChanged, bool(const std::string &));
  MOCK_CONST_METHOD0(searchResultsExist, bool());
  MOCK_METHOD0(setSearchResultsExist, void());

  MOCK_METHOD1(setupNewAutoreduction, bool(const std::string &));
  MOCK_METHOD0(pause, bool());
  MOCK_METHOD0(stop, void());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

#endif /*MANTID_CUSTOMINTERFACES_REFLMOCKOBJECTS_H*/
