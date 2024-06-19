// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitPlotModel.h"

#include <utility>

#include "ConvolutionModel.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Workspace_fwd.h"

namespace {
using namespace Mantid::API;

IFunction_sptr firstFunctionWithParameter(IFunction_sptr function, const std::string &category,
                                          const std::string &parameterName);

IFunction_sptr firstFunctionWithParameter(const CompositeFunction_sptr &composite, const std::string &category,
                                          const std::string &parameterName) {
  for (auto i = 0u; i < composite->nFunctions(); ++i) {
    const auto value = firstFunctionWithParameter(composite->getFunction(i), category, parameterName);
    if (value)
      return value;
  }
  return nullptr;
}

IFunction_sptr firstFunctionWithParameter(IFunction_sptr function, const std::string &category,
                                          const std::string &parameterName) {
  auto checkCategory = function->category().find(category);
  if (checkCategory != std::string::npos && function->hasParameter(parameterName))
    return function;

  const auto composite = std::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    return firstFunctionWithParameter(composite, category, parameterName);
  return nullptr;
}

std::optional<double> firstParameterValue(const IFunction_sptr &function, const std::string &category,
                                          const std::string &parameterName) {
  if (!function)
    return std::nullopt;

  const auto functionWithParameter = firstFunctionWithParameter(function, category, parameterName);
  if (functionWithParameter)
    return functionWithParameter->getParameter(parameterName);
  return std::nullopt;
}

std::optional<double> findFirstPeakCentre(const IFunction_sptr &function) {
  return firstParameterValue(function, "Peak", "PeakCentre");
}

std::optional<double> findFirstFWHM(const IFunction_sptr &function) {
  return firstParameterValue(function, "Peak", "FWHM");
}

std::optional<double> findFirstBackgroundLevel(const IFunction_sptr &function) {
  return firstParameterValue(function, "Background", "A0");
}

MatrixWorkspace_sptr castToMatrixWorkspace(const Workspace_sptr &workspace) {
  return std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}
// Need to adjust the guess range so the first data point isn't thrown away
constexpr double RANGEADJUSTMENT = 1e-5;
inline void adjustRange(std::pair<double, double> &range) {
  range.first = (1 - RANGEADJUSTMENT) * range.first;
  range.second = (1 + RANGEADJUSTMENT) * range.second;
}
} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

using namespace Mantid::API;

FitPlotModel::FitPlotModel(std::vector<FitData> *fittingData, IFitOutput *fitOutput)
    : m_fittingData(fittingData), m_fitOutput(fitOutput), m_activeWorkspaceID{0}, m_activeWorkspaceIndex{0},
      m_activeFunction() {}

void FitPlotModel::setActiveIndex(WorkspaceID workspaceID) { m_activeWorkspaceID = workspaceID; }

void FitPlotModel::setActiveSpectrum(WorkspaceIndex spectrum) { m_activeWorkspaceIndex = spectrum; }

void FitPlotModel::setFitFunction(Mantid::API::MultiDomainFunction_sptr function) {
  m_activeFunction = std::move(function);
}

MatrixWorkspace_sptr FitPlotModel::getWorkspace() const {
  if (m_activeWorkspaceID < m_fittingData->size())
    return m_fittingData->at(m_activeWorkspaceID.value).workspace();
  return nullptr;
}

FunctionModelSpectra FitPlotModel::getSpectra(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).spectra();
  return FunctionModelSpectra("");
}

std::pair<double, double> FitPlotModel::getRange() const {
  if (m_activeWorkspaceID.value < m_fittingData->size() &&
      !m_fittingData->at(m_activeWorkspaceID.value).zeroSpectra()) {
    return m_fittingData->at(m_activeWorkspaceID.value).getRange(m_activeWorkspaceIndex);
  }
  return std::make_pair(0., 0.);
}

std::pair<double, double> FitPlotModel::getWorkspaceRange() const {
  const auto &xValues = getWorkspace()->x(0);
  return {xValues.front(), xValues.back()};
}

std::pair<double, double> FitPlotModel::getResultRange() const {
  const auto &xValues = getResultWorkspace()->x(0);
  return {xValues.front(), xValues.back()};
}

WorkspaceID FitPlotModel::getActiveWorkspaceID() const { return m_activeWorkspaceID; }

WorkspaceIndex FitPlotModel::getActiveWorkspaceIndex() const { return m_activeWorkspaceIndex; }

WorkspaceID FitPlotModel::numberOfWorkspaces() const { return WorkspaceID{m_fittingData->size()}; }

size_t FitPlotModel::numberOfSpectra(WorkspaceID workspaceID) const {
  if (workspaceID < m_fittingData->size())
    return m_fittingData->at(workspaceID.value).numberOfSpectra().value;
  else
    throw std::runtime_error("Cannot find the number of spectra for a workspace: the workspace "
                             "index provided is too large.");
}

FitDomainIndex FitPlotModel::getActiveDomainIndex() const {
  FitDomainIndex index{0};
  for (WorkspaceID iws{0}; iws < numberOfWorkspaces(); ++iws) {
    if (iws < m_activeWorkspaceID) {
      index += FitDomainIndex{numberOfSpectra(iws)};
    } else {
      auto const spectra = getSpectra(iws);
      try {
        index += spectra.indexOf(m_activeWorkspaceIndex);
      } catch (const std::runtime_error &) {
        if (m_activeWorkspaceIndex.value != 0)
          throw;
      }
      break;
    }
  }
  return index;
}

std::optional<double> FitPlotModel::getFirstHWHM() const {
  auto fwhm = findFirstFWHM(m_activeFunction);
  if (fwhm) {
    return *fwhm / 2.0;
  }
  return std::nullopt;
}

std::optional<double> FitPlotModel::getFirstPeakCentre() const { return findFirstPeakCentre(m_activeFunction); }

std::optional<double> FitPlotModel::getFirstBackgroundLevel() const {
  auto const spectra = getSpectra(m_activeWorkspaceID);
  if (spectra.empty())
    return std::optional<double>();
  auto index = spectra.indexOf(m_activeWorkspaceIndex);
  if (!m_activeFunction || m_activeFunction->nFunctions() == 0)
    return std::optional<double>();
  return findFirstBackgroundLevel(m_activeFunction->getFunction(index.value));
}

double FitPlotModel::calculateHWHMMaximum(double minimum) const {
  const auto peakCentre = getFirstPeakCentre().value_or(0.);
  return peakCentre + (peakCentre - minimum);
}

double FitPlotModel::calculateHWHMMinimum(double maximum) const {
  const auto peakCentre = getFirstPeakCentre().value_or(0.);
  return peakCentre - (maximum - peakCentre);
}

bool FitPlotModel::canCalculateGuess() const {
  if (!m_activeFunction)
    return false;

  const auto composite = std::dynamic_pointer_cast<CompositeFunction>(m_activeFunction);
  const auto isEmptyModel = composite && composite->nFunctions() == 0;
  return getWorkspace() && !isEmptyModel;
}

MatrixWorkspace_sptr FitPlotModel::getResultWorkspace() const {
  const auto location = getResultLocation(m_activeWorkspaceID, m_activeWorkspaceIndex);

  if (location) {
    const auto group = location->result.lock();
    if (group)
      return castToMatrixWorkspace(group->getItem(location->index.value));
  }
  return nullptr;
}

MatrixWorkspace_sptr FitPlotModel::getGuessWorkspace() const {
  const auto range = getGuessRange();
  return createGuessWorkspace(getWorkspace(), getSingleFunction(m_activeWorkspaceID, m_activeWorkspaceIndex),
                              range.first, range.second);
}

Mantid::API::IFunction_sptr FitPlotModel::getSingleFunction(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  if (m_activeFunction->getNumberDomains() == 0) {
    throw std::runtime_error("Cannot set up a fit: is the function defined?");
  }
  return m_activeFunction->getFunction(getDomainIndex(workspaceID, spectrum).value);
}

FitDomainIndex FitPlotModel::getDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  FitDomainIndex index{0};
  for (size_t iws = 0; iws < m_fittingData->size(); ++iws) {
    if (iws < workspaceID.value) {
      index += numberOfSpectra(iws);
    } else {
      auto const spectra = getSpectra(iws);
      try {
        index += spectra.indexOf(spectrum);
      } catch (const std::runtime_error &) {
        if (spectrum.value != 0)
          throw;
      }
      break;
    }
  }
  return index;
}

std::optional<ResultLocationNew> FitPlotModel::getResultLocation(WorkspaceID workspaceID,
                                                                 WorkspaceIndex spectrum) const {
  auto fitDomainIndex = getDomainIndex(workspaceID, spectrum);
  if (!m_fitOutput->isEmpty() && numberOfWorkspaces() > workspaceID)
    return m_fitOutput->getResultLocation(fitDomainIndex);
  return std::nullopt;
}

std::pair<double, double> FitPlotModel::getGuessRange() const {
  std::pair<double, double> range;
  if (getResultWorkspace()) {
    range = getResultRange();
  } else {
    range = getRange();
  }
  adjustRange(range);
  return range;
}

MatrixWorkspace_sptr FitPlotModel::createGuessWorkspace(const MatrixWorkspace_sptr &inputWorkspace,
                                                        const IFunction_const_sptr &func, double startX,
                                                        double endX) const {
  IAlgorithm_sptr createWsAlg = AlgorithmManager::Instance().create("EvaluateFunction");
  createWsAlg->initialize();
  createWsAlg->setChild(true);
  createWsAlg->setLogging(false);
  createWsAlg->setProperty("Function", func->asString());
  createWsAlg->setProperty("IgnoreInvalidData", true);
  createWsAlg->setProperty("InputWorkspace", inputWorkspace);
  createWsAlg->setProperty("OutputWorkspace", "__QENSGuess");
  createWsAlg->setProperty("StartX", startX);
  createWsAlg->setProperty("EndX", endX);
  createWsAlg->execute();
  Workspace_sptr outputWorkspace = createWsAlg->getProperty("OutputWorkspace");
  return extractSpectra(std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(outputWorkspace), 1, 1, startX, endX);
}

std::vector<double> FitPlotModel::computeOutput(const IFunction_const_sptr &func,
                                                const std::vector<double> &dataX) const {
  if (dataX.empty())
    return std::vector<double>();

  FunctionDomain1DVector domain(dataX);
  FunctionValues outputData(domain);
  func->function(domain, outputData);

  std::vector<double> dataY(dataX.size());
  for (auto i = 0u; i < dataX.size(); ++i)
    dataY[i] = outputData.getCalculated(i);
  return dataY;
}

IAlgorithm_sptr FitPlotModel::createWorkspaceAlgorithm(std::size_t numberOfSpectra, const std::vector<double> &dataX,
                                                       const std::vector<double> &dataY) const {
  IAlgorithm_sptr createWsAlg = AlgorithmManager::Instance().create("CreateWorkspace");
  createWsAlg->initialize();
  createWsAlg->setChild(true);
  createWsAlg->setLogging(false);
  createWsAlg->setProperty("OutputWorkspace", "__QENSGuess");
  createWsAlg->setProperty("NSpec", boost::numeric_cast<int>(numberOfSpectra));
  createWsAlg->setProperty("DataX", dataX);
  createWsAlg->setProperty("DataY", dataY);
  return createWsAlg;
}

MatrixWorkspace_sptr FitPlotModel::extractSpectra(const MatrixWorkspace_sptr &inputWS, int startIndex, int endIndex,
                                                  double startX, double endX) const {
  auto extractSpectraAlg = AlgorithmManager::Instance().create("ExtractSpectra");
  extractSpectraAlg->initialize();
  extractSpectraAlg->setChild(true);
  extractSpectraAlg->setLogging(false);
  extractSpectraAlg->setProperty("InputWorkspace", inputWS);
  extractSpectraAlg->setProperty("StartWorkspaceIndex", startIndex);
  extractSpectraAlg->setProperty("XMin", startX);
  extractSpectraAlg->setProperty("XMax", endX);
  extractSpectraAlg->setProperty("EndWorkspaceIndex", endIndex);
  extractSpectraAlg->setProperty("OutputWorkspace", "__extracted");
  extractSpectraAlg->execute();
  return extractSpectraAlg->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr FitPlotModel::appendSpectra(const MatrixWorkspace_sptr &inputWS,
                                                 const MatrixWorkspace_sptr &spectraWS) const {
  auto appendSpectraAlg = AlgorithmManager::Instance().create("AppendSpectra");
  appendSpectraAlg->initialize();
  appendSpectraAlg->setChild(true);
  appendSpectraAlg->setLogging(false);
  appendSpectraAlg->setProperty("InputWorkspace1", inputWS);
  appendSpectraAlg->setProperty("InputWorkspace2", spectraWS);
  appendSpectraAlg->setProperty("OutputWorkspace", "__appended");
  appendSpectraAlg->execute();
  return appendSpectraAlg->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr FitPlotModel::cropWorkspace(const MatrixWorkspace_sptr &inputWS, double startX, double endX,
                                                 int startIndex, int endIndex) const {
  const auto cropAlg = AlgorithmManager::Instance().create("CropWorkspace");
  cropAlg->initialize();
  cropAlg->setChild(true);
  cropAlg->setLogging(false);
  cropAlg->setProperty("InputWorkspace", inputWS);
  cropAlg->setProperty("XMin", startX);
  cropAlg->setProperty("XMax", endX);
  cropAlg->setProperty("StartWorkspaceIndex", startIndex);
  cropAlg->setProperty("EndWorkspaceIndex", endIndex);
  cropAlg->setProperty("OutputWorkspace", "__cropped");
  cropAlg->execute();
  return cropAlg->getProperty("OutputWorkspace");
}

void FitPlotModel::deleteWorkspace(const std::string &name) const {
  auto deleteWorkspaceAlg = AlgorithmManager::Instance().create("DeleteWorkspace");
  deleteWorkspaceAlg->initialize();
  deleteWorkspaceAlg->setChild(true);
  deleteWorkspaceAlg->setLogging(false);
  deleteWorkspaceAlg->setProperty("Workspace", name);
  deleteWorkspaceAlg->execute();
}

} // namespace MantidQt::CustomInterfaces::Inelastic
