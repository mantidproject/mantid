// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "BaseInstrumentModel.h"
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
namespace MantidQt {
namespace CustomInterfaces {

BaseInstrumentModel::BaseInstrumentModel()
    : m_currentRun(0), m_tmpName("tmp"), m_instrumentName("MUSR"),
      m_wsName("testData") {}

void BaseInstrumentModel::loadEmptyInstrument() {
  auto alg =
      Mantid::API::AlgorithmManager::Instance().create("LoadEmptyInstrument");
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
std::pair<int, std::string>
BaseInstrumentModel::loadData(const std::string &name) {
  auto alg = AlgorithmManager::Instance().create("Load");
  alg->initialize();
  alg->setProperty("Filename", name);
  alg->setProperty("OutputWorkspace", m_wsName);
  alg->execute();
  auto ws =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_wsName);
  int runNumber = ws->getRunNumber();

  std::string message = "success";

  return std::make_pair(runNumber, message);
}

void BaseInstrumentModel::rename() {
  AnalysisDataService::Instance().rename(m_tmpName, m_wsName);
}
void BaseInstrumentModel::remove() {
  AnalysisDataService::Instance().remove(m_tmpName);
}

std::string BaseInstrumentModel::dataFileName() { return m_wsName; }

int BaseInstrumentModel::currentRun() {
  try {

    auto ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(m_wsName);
    return ws->getRunNumber();
  } catch (...) {
    return ERRORCODE;
  }
}

bool BaseInstrumentModel::isErrorCode(const int run) {
  return (run == ERRORCODE);
}

} // namespace CustomInterfaces
} // namespace MantidQt
