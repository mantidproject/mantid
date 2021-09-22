// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentModel.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Unit.h"

#include <utility>

namespace {
const int ERRORCODE = -999;
}

using namespace Mantid::API;
namespace MantidQt::MantidWidgets {

BaseCustomInstrumentModel::BaseCustomInstrumentModel()
    : m_currentRun(0), m_tmpName("tmp"), m_instrumentName("MUSR"), m_wsName("testData") {}

BaseCustomInstrumentModel::BaseCustomInstrumentModel(std::string tmpName, std::string instrumentName,
                                                     std::string wsName)
    : m_currentRun(0), m_tmpName(std::move(tmpName)), m_instrumentName(std::move(instrumentName)),
      m_wsName(std::move(wsName)) {}

void BaseCustomInstrumentModel::loadEmptyInstrument() {
  auto alg = Mantid::API::AlgorithmManager::Instance().create("LoadEmptyInstrument");
  alg->initialize();
  alg->setProperty("OutputWorkspace", m_wsName);
  alg->setProperty("InstrumentName", m_instrumentName);
  alg->execute();
}
/*
 * Loads data
 * Loads data, normalise to current and then converts to d spacing
 * @param name:: string name for  data
 * @return std::pair<int,std::string>:: the run number and status
 */
std::pair<int, std::string> BaseCustomInstrumentModel::loadData(const std::string &name) {
  auto alg = AlgorithmManager::Instance().create("Load");
  alg->initialize();
  alg->setProperty("Filename", name);
  alg->setProperty("OutputWorkspace", m_wsName);
  alg->execute();
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_wsName);
  int runNumber = ws->getRunNumber();

  std::string message = "success";

  return std::make_pair(runNumber, message);
}

void BaseCustomInstrumentModel::rename() { AnalysisDataService::Instance().rename(m_tmpName, m_wsName); }
void BaseCustomInstrumentModel::remove() { AnalysisDataService::Instance().remove(m_tmpName); }

std::string BaseCustomInstrumentModel::dataFileName() { return m_wsName; }

int BaseCustomInstrumentModel::currentRun() {
  try {

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_wsName);
    return ws->getRunNumber();
  } catch (...) {
    return ERRORCODE;
  }
}

bool BaseCustomInstrumentModel::isErrorCode(const int run) { return (run == ERRORCODE); }

} // namespace MantidQt::MantidWidgets
