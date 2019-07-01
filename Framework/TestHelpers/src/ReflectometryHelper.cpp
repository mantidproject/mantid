// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidTestHelpers/ReflectometryHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

namespace Mantid {

using namespace API;
using namespace DataHandling;
using namespace DataObjects;
using namespace HistogramData;
using namespace Kernel;

namespace TestHelpers {

MatrixWorkspace_sptr createHistoWS(size_t nBins, double startX, double endX,
                                   std::vector<double> const &values,
                                   std::string const &unitX = "TOF") {
  double const dX = (endX - startX) / double(nBins);
  BinEdges xVals(nBins + 1, LinearGenerator(startX, dX));
  size_t nSpec = values.size();
  std::vector<double> yVals(nSpec * nBins);
  for (size_t i = 0; i < nSpec; ++i) {
    std::fill(yVals.begin() + i * nBins, yVals.begin() + (i + 1) * nBins,
              values[i]);
  }

  auto creator =
      AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
  creator->setChild(true);
  creator->initialize();
  creator->setProperty("DataX", xVals.rawData());
  creator->setProperty("DataY", yVals);
  creator->setProperty("NSpec", int(nSpec));
  creator->setProperty("UnitX", unitX);
  creator->setPropertyValue("OutputWorkspace", "dummy");
  creator->execute();
  MatrixWorkspace_sptr workspace = creator->getProperty("OutputWorkspace");
  return workspace;
}

MatrixWorkspace_sptr createREFL_WS(size_t nBins, double startX, double endX,
                                   std::vector<double> const &values,
                                   std::string const &paramsType) {
  MatrixWorkspace_sptr workspace = createHistoWS(nBins, startX, endX, values);

  auto instrumentLoader =
      AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
  instrumentLoader->initialize();
  instrumentLoader->setPropertyValue("Filename",
                                     "unit_testing/REFL_Definition.xml");
  instrumentLoader->setProperty("Workspace", workspace);
  instrumentLoader->setProperty("RewriteSpectraMap", OptionalBool(true));
  instrumentLoader->execute();

  if (!paramsType.empty()) {
    auto paramLoader =
        AlgorithmManager::Instance().createUnmanaged("LoadParameterFile");
    paramLoader->initialize();
    paramLoader->setPropertyValue("Filename", "unit_testing/REFL_Parameters_" +
                                                  paramsType + ".xml");
    paramLoader->setProperty("Workspace", workspace);
    paramLoader->execute();
  }

  return workspace;
}

void prepareInputGroup(std::string const &name, std::string const &paramsType,
                       size_t size, double const startX, double const endX,
                       size_t const nBins) {
  double monitorValue = 99.0;
  double detectorValue = 0.9;
  std::string names;

  auto &ADS = AnalysisDataService::Instance();

  for (size_t i = 0; i < size; ++i) {
    std::vector<double> values(257, detectorValue);
    values[0] = monitorValue;
    MatrixWorkspace_sptr ws =
        createREFL_WS(nBins, startX, endX, values, paramsType);
    std::string const name1 = name + "_" + std::to_string(i + 1);
    ADS.addOrReplace(name1, ws);
    monitorValue -= 1.0;
    detectorValue -= 0.1;
    if (i > 0)
      names.append(",");
    names.append(name1);
  }

  auto mkGroup =
      AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
  mkGroup->initialize();
  mkGroup->setProperty("InputWorkspaces", names);
  mkGroup->setProperty("OutputWorkspace", name);
  mkGroup->execute();
}

std::vector<MatrixWorkspace_sptr> groupToVector(WorkspaceGroup_sptr group) {
  std::vector<MatrixWorkspace_sptr> out;
  for (size_t i = 0; i < group->size(); ++i) {
    out.emplace_back(
        boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(i)));
  }
  return out;
}

std::vector<MatrixWorkspace_sptr> retrieveOutWS(std::string const &name) {
  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(name);
  return groupToVector(group);
}

void applyPolarizationEfficiencies(WorkspaceGroup_sptr ws) {

  auto wss = groupToVector(ws);
  auto Rpp = wss[0];
  auto Rpa = wss[1];
  auto Rap = wss[2];
  auto Raa = wss[3];

  auto nBins = Rpp->blocksize();
  auto startX = Rpp->x(0).front();
  auto endX = Rpp->x(0).back();

  auto Pp = createHistoWS(nBins, startX, endX, {0.9});
  auto Ap = createHistoWS(nBins, startX, endX, {0.8});
  auto Pa = createHistoWS(nBins, startX, endX, {0.7});
  auto Aa = createHistoWS(nBins, startX, endX, {0.6});

  auto Ipp = (Rpp * (Pp + 1.0) * (Ap + 1.0) +
              Raa * (Pp * (-1.0) + 1.0) * (Ap * (-1.0) + 1.0) +
              Rpa * (Pp + 1.0) * (Ap * (-1.0) + 1.0) +
              Rap * (Pp * (-1.0) + 1.0) * (Ap + 1.0)) /
             4.;
  auto Iaa = (Raa * (Pa + 1.0) * (Aa + 1.0) +
              Rpp * (Pa * (-1.0) + 1.0) * (Aa * (-1.0) + 1.0) +
              Rap * (Pa + 1.0) * (Aa * (-1.0) + 1.0) +
              Rpa * (Pa * (-1.0) + 1.0) * (Aa + 1.0)) /
             4.;
  auto Ipa = (Rpa * (Pp + 1.0) * (Aa + 1.0) +
              Rap * (Pp * (-1.0) + 1.0) * (Aa * (-1.0) + 1.0) +
              Rpp * (Pp + 1.0) * (Aa * (-1.0) + 1.0) +
              Raa * (Pp * (-1.0) + 1.0) * (Aa + 1.0)) /
             4.;
  auto Iap = (Rap * (Pa + 1.0) * (Ap + 1.0) +
              Rpa * (Pa * (-1.0) + 1.0) * (Ap * (-1.0) + 1.0) +
              Raa * (Pa + 1.0) * (Ap * (-1.0) + 1.0) +
              Rpp * (Pa * (-1.0) + 1.0) * (Ap + 1.0)) /
             4.;

  auto &ADS = AnalysisDataService::Instance();
  ADS.addOrReplace(Rpp->getName(), Ipp);
  ADS.addOrReplace(Rpa->getName(), Ipa);
  ADS.addOrReplace(Rap->getName(), Iap);
  ADS.addOrReplace(Raa->getName(), Iaa);

  auto instrument = Rpp->getInstrument();
  Ipp->setInstrument(instrument);
  Ipa->setInstrument(instrument);
  Iap->setInstrument(instrument);
  Iaa->setInstrument(instrument);
}

void applyPolarizationEfficiencies(std::string const &name) {
  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(name);
  applyPolarizationEfficiencies(group);
}

MatrixWorkspace_sptr createWorkspaceSingle(const double startX, const int nBins,
                                           const double deltaX,
                                           const std::vector<double> &yValues) {
  auto ws = WorkspaceCreationHelper::
      create2DWorkspaceWithReflectometryInstrumentMultiDetector(
          startX, 0.0, Mantid::Kernel::V3D(0, 0, 0),
          Mantid::Kernel::V3D(0, 0, 1), 0.5, 1.0, Mantid::Kernel::V3D(0, 0, 0),
          Mantid::Kernel::V3D(14, 0, 0), Mantid::Kernel::V3D(15, 0, 0),
          Mantid::Kernel::V3D(20, (20 - 15), 0), 2, nBins, deltaX);

  for (auto i = 0u; i < ws->y(0).size(); ++i) {
    ws->mutableY(0)[i] = yValues[i];
  }

  ws->mutableRun().addProperty<std::string>("run_number", "1234");

  return ws;
}

MatrixWorkspace_sptr createWorkspaceSingle(const double startX, const int nBins,
                                           const double deltaX) {
  auto ws = WorkspaceCreationHelper::
      create2DWorkspaceWithReflectometryInstrumentMultiDetector(
          startX, 0.0, Mantid::Kernel::V3D(0, 0, 0),
          Mantid::Kernel::V3D(0, 0, 1), 0.5, 1.0, Mantid::Kernel::V3D(0, 0, 0),
          Mantid::Kernel::V3D(14, 0, 0), Mantid::Kernel::V3D(15, 0, 0),
          Mantid::Kernel::V3D(20, (20 - 15), 0), 2, nBins, deltaX);

  ws->mutableRun().addProperty<std::string>("run_number", "1234");
  return ws;
}
} // namespace TestHelpers
} // namespace Mantid
