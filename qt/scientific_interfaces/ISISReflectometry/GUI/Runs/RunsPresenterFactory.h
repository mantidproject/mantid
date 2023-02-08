// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "GUI/RunsTable/RunsTablePresenterFactory.h"
#include "IRunsPresenter.h"
#include "IRunsView.h"
#include "MantidAPI/AsyncAlgorithmRunner.h"
#include "MantidQtWidgets/Common/ProgressableView.h"
#include "RunsPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class RunsPresenterFactory {
public:
  RunsPresenterFactory(RunsTablePresenterFactory runsTablePresenterFactory, double thetaTolerance,
                       std::vector<std::string> instruments, IReflMessageHandler *messageHandler,
                       IFileHandler *fileHandler)
      : m_runsTablePresenterFactory(std::move(runsTablePresenterFactory)), m_thetaTolerance(std::move(thetaTolerance)),
        m_instruments(std::move(instruments)), m_messageHandler(messageHandler), m_fileHandler(fileHandler) {}

  std::unique_ptr<IRunsPresenter> make(IRunsView *view) {
    auto algorithmRunner = std::make_unique<Mantid::API::AsyncAlgorithmRunner>();
    return std::make_unique<RunsPresenter>(view, view, m_runsTablePresenterFactory, m_thetaTolerance, m_instruments,
                                           m_messageHandler, m_fileHandler, std::move(algorithmRunner));
  }

private:
  RunsTablePresenterFactory m_runsTablePresenterFactory;
  double m_thetaTolerance;
  std::vector<std::string> m_instruments;
  IReflMessageHandler *m_messageHandler;
  IFileHandler *m_fileHandler;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
