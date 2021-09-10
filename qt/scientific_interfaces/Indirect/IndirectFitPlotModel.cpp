// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitPlotModel.h"

#include <utility>

#include "ConvFitModel.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Workspace_fwd.h"

namespace {
using namespace Mantid::API;

// The name of the conjoined input and guess workspaces -- required for
// creating an external guess plot.
const std::string INPUT_AND_GUESS_NAME = "__QENSInputAndGuess";

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
  if (function->category() == category && function->hasParameter(parameterName))
    return function;

  const auto composite = std::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    return firstFunctionWithParameter(composite, category, parameterName);
  return nullptr;
}

boost::optional<double> firstParameterValue(const IFunction_sptr &function, const std::string &category,
                                            const std::string &parameterName) {
  if (!function)
    return boost::none;

  const auto functionWithParameter = firstFunctionWithParameter(function, category, parameterName);
  if (functionWithParameter)
    return functionWithParameter->getParameter(parameterName);
  return boost::none;
}

boost::optional<double> findFirstPeakCentre(const IFunction_sptr &function) {
  return firstParameterValue(std::move(function), "Peak", "PeakCentre");
}

boost::optional<double> findFirstFWHM(const IFunction_sptr &function) {
  return firstParameterValue(std::move(function), "Peak", "FWHM");
}

boost::optional<double> findFirstBackgroundLevel(const IFunction_sptr &function) {
  return firstParameterValue(std::move(function), "Background", "A0");
}

void setFunctionParameters(const IFunction_sptr &function, const std::string &category,
                           const std::string &parameterName, double value);

void setFunctionParameters(const CompositeFunction_sptr &composite, const std::string &category,
                           const std::string &parameterName, double value) {
  for (auto i = 0u; i < composite->nFunctions(); ++i)
    setFunctionParameters(composite->getFunction(i), category, parameterName, value);
}

void setFunctionParameters(const IFunction_sptr &function, const std::string &category,
                           const std::string &parameterName, double value) {
  if (function->category() == category && function->hasParameter(parameterName))
    function->setParameter(parameterName, value);

  auto composite = std::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    setFunctionParameters(composite, category, parameterName, value);
}

void setFunctionParameters(const MultiDomainFunction_sptr &function, const std::string &category,
                           const std::string &parameterName, double value) {
  for (size_t i = 0u; i < function->nFunctions(); ++i)
    setFunctionParameters(function->getFunction(i), category, parameterName, value);
}

void setFirstBackground(IFunction_sptr function, double value) {
  firstFunctionWithParameter(std::move(function), "Background", "A0")->setParameter("A0", value);
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

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace Mantid::API;

IndirectFitPlotModel::IndirectFitPlotModel(IndirectFittingModel *fittingModel)
    : m_fittingModel(fittingModel), m_activeWorkspaceID{0}, m_activeWorkspaceIndex{0} {}

IndirectFitPlotModel::~IndirectFitPlotModel() { deleteExternalGuessWorkspace(); }

void IndirectFitPlotModel::setActiveIndex(WorkspaceID workspaceID) { m_activeWorkspaceID = workspaceID; }

void IndirectFitPlotModel::setActiveSpectrum(WorkspaceIndex spectrum) { m_activeWorkspaceIndex = spectrum; }

void IndirectFitPlotModel::setStartX(double startX) {
  if (getRange().second > startX)
    m_fittingModel->setStartX(startX, m_activeWorkspaceID);
}

void IndirectFitPlotModel::setEndX(double endX) {
  if (getRange().first < endX)
    m_fittingModel->setEndX(endX, m_activeWorkspaceID);
}

void IndirectFitPlotModel::setFWHM(double fwhm) {
  m_fittingModel->setDefaultParameterValue("FWHM", fwhm, m_activeWorkspaceID);
  setFunctionParameters(m_fittingModel->getFitFunction(), "Peak", "FWHM", fwhm);
}

void IndirectFitPlotModel::setBackground(double background) {
  m_fittingModel->setDefaultParameterValue("A0", background, m_activeWorkspaceID);
  setFirstBackground(m_fittingModel->getFitFunction(), background);
}

void IndirectFitPlotModel::deleteExternalGuessWorkspace() {
  if (AnalysisDataService::Instance().doesExist(INPUT_AND_GUESS_NAME))
    deleteWorkspace(INPUT_AND_GUESS_NAME);
}

MatrixWorkspace_sptr IndirectFitPlotModel::getWorkspace() const {
  return m_fittingModel->getWorkspace(m_activeWorkspaceID);
}

FunctionModelSpectra IndirectFitPlotModel::getSpectra() const {
  return m_fittingModel->getSpectra(m_activeWorkspaceID);
}

std::pair<double, double> IndirectFitPlotModel::getRange() const {
  return m_fittingModel->getFittingRange(m_activeWorkspaceID, m_activeWorkspaceIndex);
}

std::pair<double, double> IndirectFitPlotModel::getWorkspaceRange() const {
  const auto xValues = getWorkspace()->x(0);
  return {xValues.front(), xValues.back()};
}

std::pair<double, double> IndirectFitPlotModel::getResultRange() const {
  const auto xValues = getResultWorkspace()->x(0);
  return {xValues.front(), xValues.back()};
}

WorkspaceID IndirectFitPlotModel::getActiveWorkspaceIndex() const { return m_activeWorkspaceID; }

WorkspaceIndex IndirectFitPlotModel::getActiveSpectrum() const { return m_activeWorkspaceIndex; }

WorkspaceID IndirectFitPlotModel::numberOfWorkspaces() const { return m_fittingModel->getNumberOfWorkspaces(); }

FitDomainIndex IndirectFitPlotModel::getActiveDomainIndex() const {
  FitDomainIndex index{0};
  for (WorkspaceID iws{0}; iws < numberOfWorkspaces(); ++iws) {
    if (iws < m_activeWorkspaceID) {
      index += FitDomainIndex{m_fittingModel->getNumberOfSpectra(iws)};
    } else {
      auto const spectra = m_fittingModel->getSpectra(iws);
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

std::string IndirectFitPlotModel::getFitDataName(WorkspaceID workspaceID) const {
  if (m_fittingModel->getWorkspace(workspaceID))
    return m_fittingModel->createDisplayName(workspaceID);
  return "";
}

std::string IndirectFitPlotModel::getFitDataName() const { return getFitDataName(m_activeWorkspaceID); }

std::string IndirectFitPlotModel::getLastFitDataName() const {
  auto const workspaceCount = m_fittingModel->getNumberOfWorkspaces();
  if (workspaceCount.value > 0)
    return getFitDataName(workspaceCount - WorkspaceID{1});
  return "";
}

boost::optional<double> IndirectFitPlotModel::getFirstHWHM() const {
  auto fwhm = findFirstFWHM(m_fittingModel->getFitFunction());
  if (fwhm) {
    return *fwhm / 2.0;
  }
  return boost::none;
}

boost::optional<double> IndirectFitPlotModel::getFirstPeakCentre() const {
  return findFirstPeakCentre(m_fittingModel->getFitFunction());
}

boost::optional<double> IndirectFitPlotModel::getFirstBackgroundLevel() const {
  auto const spectra = m_fittingModel->getSpectra(m_activeWorkspaceID);
  if (spectra.empty())
    return boost::optional<double>();
  auto index = spectra.indexOf(m_activeWorkspaceIndex);
  IFunction_sptr fun = m_fittingModel->getFitFunction();
  if (!fun)
    return boost::optional<double>();
  return findFirstBackgroundLevel(fun->getFunction(index.value));
}

double IndirectFitPlotModel::calculateHWHMMaximum(double minimum) const {
  const auto peakCentre = getFirstPeakCentre().get_value_or(0.);
  return peakCentre + (peakCentre - minimum);
}

double IndirectFitPlotModel::calculateHWHMMinimum(double maximum) const {
  const auto peakCentre = getFirstPeakCentre().get_value_or(0.);
  return peakCentre - (maximum - peakCentre);
}

bool IndirectFitPlotModel::canCalculateGuess() const {
  const auto function = m_fittingModel->getFitFunction();
  if (!function)
    return false;

  const auto composite = std::dynamic_pointer_cast<CompositeFunction>(function);
  const auto resolutionLoaded = isResolutionLoaded();
  const auto isEmptyModel = composite && composite->nFunctions() == 0;
  return getWorkspace() && !isEmptyModel && resolutionLoaded;
}

bool IndirectFitPlotModel::isResolutionLoaded() const {
  const auto model = dynamic_cast<ConvFitModel *>(m_fittingModel);
  if (model) {
    return m_fittingModel->getResolutionsForFit().size() != 0;
  }
  // If its not a ConvFitModel it doesn't require a resolution, so return true
  return true;
}

MatrixWorkspace_sptr IndirectFitPlotModel::getResultWorkspace() const {
  const auto location = m_fittingModel->getResultLocation(m_activeWorkspaceID, m_activeWorkspaceIndex);

  if (location) {
    const auto group = location->result.lock();
    if (group)
      return castToMatrixWorkspace(group->getItem(location->index.value));
  }
  return nullptr;
}

MatrixWorkspace_sptr IndirectFitPlotModel::getGuessWorkspace() const {
  const auto range = getGuessRange();
  return createGuessWorkspace(getWorkspace(),
                              m_fittingModel->getSingleFunction(m_activeWorkspaceID, m_activeWorkspaceIndex),
                              range.first, range.second);
}

MatrixWorkspace_sptr IndirectFitPlotModel::appendGuessToInput(const MatrixWorkspace_sptr &guessWorkspace) const {
  const auto range = getGuessRange();
  return createInputAndGuessWorkspace(getWorkspace(), std::move(guessWorkspace),
                                      static_cast<int>(m_activeWorkspaceIndex.value), range.first, range.second);
}

std::pair<double, double> IndirectFitPlotModel::getGuessRange() const {
  std::pair<double, double> range;
  if (getResultWorkspace())
    range = getResultRange();
  range = getRange();
  adjustRange(range);
  return range;
}

MatrixWorkspace_sptr IndirectFitPlotModel::createInputAndGuessWorkspace(const MatrixWorkspace_sptr &inputWS,
                                                                        const MatrixWorkspace_sptr &guessWorkspace,
                                                                        int spectrum, double startX,
                                                                        double endX) const {
  guessWorkspace->setInstrument(inputWS->getInstrument());
  guessWorkspace->replaceAxis(0, std::unique_ptr<Axis>(inputWS->getAxis(0)->clone(guessWorkspace.get())));
  guessWorkspace->setDistribution(inputWS->isDistribution());

  auto extracted = extractSpectra(inputWS, spectrum, spectrum, startX, endX);
  auto inputAndGuess = appendSpectra(extracted, guessWorkspace);
  AnalysisDataService::Instance().addOrReplace(INPUT_AND_GUESS_NAME, inputAndGuess);

  auto axis = std::make_unique<TextAxis>(2);
  axis->setLabel(0, "Sample");
  axis->setLabel(1, "Guess");
  inputAndGuess->replaceAxis(1, std::move(axis));
  return inputAndGuess;
}

MatrixWorkspace_sptr IndirectFitPlotModel::createGuessWorkspace(const MatrixWorkspace_sptr &inputWorkspace,
                                                                const IFunction_const_sptr &func, double startX,
                                                                double endX) const {
  IAlgorithm_sptr createWsAlg = AlgorithmManager::Instance().create("EvaluateFunction");
  createWsAlg->initialize();
  createWsAlg->setChild(true);
  createWsAlg->setLogging(false);
  createWsAlg->setProperty("Function", func->asString());
  createWsAlg->setProperty("InputWorkspace", inputWorkspace);
  createWsAlg->setProperty("OutputWorkspace", "__QENSGuess");
  createWsAlg->setProperty("StartX", startX);
  createWsAlg->setProperty("EndX", endX);
  createWsAlg->execute();
  Workspace_sptr outputWorkspace = createWsAlg->getProperty("OutputWorkspace");
  return extractSpectra(std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(outputWorkspace), 1, 1, startX, endX);
}

std::vector<double> IndirectFitPlotModel::computeOutput(const IFunction_const_sptr &func,
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

IAlgorithm_sptr IndirectFitPlotModel::createWorkspaceAlgorithm(std::size_t numberOfSpectra,
                                                               const std::vector<double> &dataX,
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

MatrixWorkspace_sptr IndirectFitPlotModel::extractSpectra(const MatrixWorkspace_sptr &inputWS, int startIndex,
                                                          int endIndex, double startX, double endX) const {
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

MatrixWorkspace_sptr IndirectFitPlotModel::appendSpectra(const MatrixWorkspace_sptr &inputWS,
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

MatrixWorkspace_sptr IndirectFitPlotModel::cropWorkspace(const MatrixWorkspace_sptr &inputWS, double startX,
                                                         double endX, int startIndex, int endIndex) const {
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

void IndirectFitPlotModel::deleteWorkspace(const std::string &name) const {
  auto deleteWorkspaceAlg = AlgorithmManager::Instance().create("DeleteWorkspace");
  deleteWorkspaceAlg->initialize();
  deleteWorkspaceAlg->setChild(true);
  deleteWorkspaceAlg->setLogging(false);
  deleteWorkspaceAlg->setProperty("Workspace", name);
  deleteWorkspaceAlg->execute();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
