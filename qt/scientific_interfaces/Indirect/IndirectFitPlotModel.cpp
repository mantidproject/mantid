// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitPlotModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidKernel/make_unique.h"

namespace {
using namespace Mantid::API;

// The name of the conjoined input and guess workspaces -- required for
// creating an external guess plot.
const std::string INPUT_AND_GUESS_NAME = "__QENSInputAndGuess";

IFunction_sptr firstFunctionWithParameter(IFunction_sptr function,
                                          const std::string &category,
                                          const std::string &parameterName);

IFunction_sptr firstFunctionWithParameter(CompositeFunction_sptr composite,
                                          const std::string &category,
                                          const std::string &parameterName) {
  for (auto i = 0u; i < composite->nFunctions(); ++i) {
    const auto value = firstFunctionWithParameter(composite->getFunction(i),
                                                  category, parameterName);
    if (value)
      return value;
  }
  return nullptr;
}

IFunction_sptr firstFunctionWithParameter(IFunction_sptr function,
                                          const std::string &category,
                                          const std::string &parameterName) {
  if (function->category() == category && function->hasParameter(parameterName))
    return function;

  const auto composite =
      boost::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    return firstFunctionWithParameter(composite, category, parameterName);
  return nullptr;
}

boost::optional<double> firstParameterValue(IFunction_sptr function,
                                            const std::string &category,
                                            const std::string &parameterName) {
  if (!function)
    return boost::none;

  const auto functionWithParameter =
      firstFunctionWithParameter(function, category, parameterName);
  if (functionWithParameter)
    return functionWithParameter->getParameter(parameterName);
  return boost::none;
}

boost::optional<double> findFirstPeakCentre(IFunction_sptr function) {
  return firstParameterValue(function, "Peak", "PeakCentre");
}

boost::optional<double> findFirstFWHM(IFunction_sptr function) {
  return firstParameterValue(function, "Peak", "FWHM");
}

boost::optional<double> findFirstBackgroundLevel(IFunction_sptr function) {
  return firstParameterValue(function, "Background", "A0");
}

void setFunctionParameters(IFunction_sptr function, const std::string &category,
                           const std::string &parameterName, double value);

void setFunctionParameters(CompositeFunction_sptr composite,
                           const std::string &category,
                           const std::string &parameterName, double value) {
  for (auto i = 0u; i < composite->nFunctions(); ++i)
    setFunctionParameters(composite->getFunction(i), category, parameterName,
                          value);
}

void setFunctionParameters(IFunction_sptr function, const std::string &category,
                           const std::string &parameterName, double value) {
  if (function->category() == category && function->hasParameter(parameterName))
    function->setParameter(parameterName, value);

  auto composite = boost::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    setFunctionParameters(composite, category, parameterName, value);
}

void setFirstBackground(IFunction_sptr function, double value) {
  firstFunctionWithParameter(function, "Background", "A0")
      ->setParameter("A0", value);
}

MatrixWorkspace_sptr castToMatrixWorkspace(Workspace_sptr workspace) {
  return boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace Mantid::API;

IndirectFitPlotModel::IndirectFitPlotModel(IndirectFittingModel *fittingModel)
    : m_fittingModel(fittingModel), m_activeIndex(0), m_activeSpectrum(0) {}

IndirectFitPlotModel::~IndirectFitPlotModel() {
  deleteExternalGuessWorkspace();
}

void IndirectFitPlotModel::setActiveIndex(std::size_t index) {
  m_activeIndex = index;
}

void IndirectFitPlotModel::setActiveSpectrum(std::size_t spectrum) {
  m_activeSpectrum = spectrum;
}

void IndirectFitPlotModel::setStartX(double startX) {
  if (getRange().second > startX)
    m_fittingModel->setStartX(startX, m_activeIndex, m_activeSpectrum);
}

void IndirectFitPlotModel::setEndX(double endX) {
  if (getRange().first < endX)
    m_fittingModel->setEndX(endX, m_activeIndex, m_activeSpectrum);
}

//void IndirectFitPlotModel::setFWHM(double fwhm) {
//  m_fittingModel->setDefaultParameterValue("FWHM", fwhm, m_activeIndex);
//  setFunctionParameters(m_fittingModel->getFittingFunction(), "Peak", "FWHM",
//                        fwhm);
//}
//
//void IndirectFitPlotModel::setBackground(double background) {
//  m_fittingModel->setDefaultParameterValue("A0", background, m_activeIndex);
//  setFirstBackground(m_fittingModel->getFittingFunction(), background);
//}

void IndirectFitPlotModel::deleteExternalGuessWorkspace() {
  if (AnalysisDataService::Instance().doesExist(INPUT_AND_GUESS_NAME))
    deleteWorkspace(INPUT_AND_GUESS_NAME);
}

MatrixWorkspace_sptr IndirectFitPlotModel::getWorkspace() const {
  return m_fittingModel->getWorkspace(m_activeIndex);
}

Spectra IndirectFitPlotModel::getSpectra() const {
  return m_fittingModel->getSpectra(m_activeIndex);
}

std::pair<double, double> IndirectFitPlotModel::getRange() const {
  return m_fittingModel->getFittingRange(m_activeIndex, m_activeSpectrum);
}

std::pair<double, double> IndirectFitPlotModel::getWorkspaceRange() const {
  const auto xValues = getWorkspace()->x(0);
  return {xValues.front(), xValues.back()};
}

std::pair<double, double> IndirectFitPlotModel::getResultRange() const {
  const auto xValues = getResultWorkspace()->x(0);
  return {xValues.front(), xValues.back()};
}

std::size_t IndirectFitPlotModel::getActiveDataIndex() const {
  return m_activeIndex;
}

std::size_t IndirectFitPlotModel::getActiveSpectrum() const {
  return m_activeSpectrum;
}

std::size_t IndirectFitPlotModel::numberOfWorkspaces() const {
  return m_fittingModel->numberOfWorkspaces();
}

std::string IndirectFitPlotModel::getFitDataName(std::size_t index) const {
  if (m_fittingModel->getWorkspace(index))
    return m_fittingModel->createDisplayName("%1% (%2%)", "-", index);
  return "";
}

std::string IndirectFitPlotModel::getFitDataName() const {
  return getFitDataName(m_activeIndex);
}

std::string IndirectFitPlotModel::getLastFitDataName() const {
  auto const workspaceCount = m_fittingModel->numberOfWorkspaces();
  if (workspaceCount > 0)
    return getFitDataName(workspaceCount - 1);
  return "";
}

boost::optional<double> IndirectFitPlotModel::getFirstHWHM() const {
  //auto fwhm = findFirstFWHM(m_fittingModel->getFittingFunction());
  //if (fwhm)
  //  return *fwhm / 2.0;
  return boost::none;
}

boost::optional<double> IndirectFitPlotModel::getFirstPeakCentre() const {
  return 0.0; // findFirstPeakCentre(m_fittingModel->getFittingFunction());
}

boost::optional<double> IndirectFitPlotModel::getFirstBackgroundLevel() const {
  return 0.0; // findFirstBackgroundLevel(m_fittingModel->getFittingFunction());
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
  const auto function = m_fittingModel->getFittingFunction();
  if (!function)
    return false;

  const auto composite =
      boost::dynamic_pointer_cast<CompositeFunction>(function);
  const auto isEmptyModel = composite && composite->nFunctions() == 0;
  return getWorkspace() && !isEmptyModel;
}

MatrixWorkspace_sptr IndirectFitPlotModel::getResultWorkspace() const {
  const auto location =
      m_fittingModel->getResultLocation(m_activeIndex, m_activeSpectrum);

  if (location) {
    const auto group = location->result.lock();
    if (group)
      return castToMatrixWorkspace(group->getItem(location->index));
  }
  return nullptr;
}

MatrixWorkspace_sptr IndirectFitPlotModel::getGuessWorkspace() const {
  const auto range = getRange();
  return createGuessWorkspace(
      getWorkspace(), m_fittingModel->getSingleFunction(m_activeIndex, m_activeSpectrum),
      boost::numeric_cast<int>(m_activeSpectrum), range.first, range.second);
}

MatrixWorkspace_sptr IndirectFitPlotModel::appendGuessToInput(
    MatrixWorkspace_sptr guessWorkspace) const {
  const auto range = getRange();
  return createInputAndGuessWorkspace(
      getWorkspace(), guessWorkspace,
      boost::numeric_cast<int>(m_activeSpectrum), range.first, range.second);
}

MatrixWorkspace_sptr IndirectFitPlotModel::createInputAndGuessWorkspace(
    MatrixWorkspace_sptr inputWS, MatrixWorkspace_sptr guessWorkspace,
    int spectrum, double startX, double endX) const {
  guessWorkspace->setInstrument(inputWS->getInstrument());
  guessWorkspace->replaceAxis(0,
                              inputWS->getAxis(0)->clone(guessWorkspace.get()));
  guessWorkspace->setDistribution(inputWS->isDistribution());

  auto extracted = extractSpectra(inputWS, spectrum, spectrum, startX, endX);
  auto inputAndGuess = appendSpectra(extracted, guessWorkspace);
  AnalysisDataService::Instance().addOrReplace(INPUT_AND_GUESS_NAME,
                                               inputAndGuess);

  auto axis = Mantid::Kernel::make_unique<TextAxis>(2);
  axis->setLabel(0, "Sample");
  axis->setLabel(1, "Guess");
  inputAndGuess->replaceAxis(1, axis.release());
  return inputAndGuess;
}

MatrixWorkspace_sptr IndirectFitPlotModel::createGuessWorkspace(
    MatrixWorkspace_sptr inputWorkspace, IFunction_const_sptr func,
    int workspaceIndex, double startX, double endX) const {
  auto croppedWS = cropWorkspace(inputWorkspace, startX, endX, workspaceIndex,
                                 workspaceIndex);
  const auto dataY = computeOutput(func, croppedWS->points(0).rawData());

  if (dataY.empty())
    return WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

  auto createWs = createWorkspaceAlgorithm(1, croppedWS->dataX(0), dataY);
  createWs->execute();
  return createWs->getProperty("OutputWorkspace");
}

std::vector<double>
IndirectFitPlotModel::computeOutput(IFunction_const_sptr func,
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

IAlgorithm_sptr IndirectFitPlotModel::createWorkspaceAlgorithm(
    std::size_t numberOfSpectra, const std::vector<double> &dataX,
    const std::vector<double> &dataY) const {
  IAlgorithm_sptr createWsAlg =
      AlgorithmManager::Instance().create("CreateWorkspace");
  createWsAlg->initialize();
  createWsAlg->setChild(true);
  createWsAlg->setLogging(false);
  createWsAlg->setProperty("OutputWorkspace", "__QENSGuess");
  createWsAlg->setProperty("NSpec", boost::numeric_cast<int>(numberOfSpectra));
  createWsAlg->setProperty("DataX", dataX);
  createWsAlg->setProperty("DataY", dataY);
  return createWsAlg;
}

MatrixWorkspace_sptr
IndirectFitPlotModel::extractSpectra(MatrixWorkspace_sptr inputWS,
                                     int startIndex, int endIndex,
                                     double startX, double endX) const {
  auto extractSpectraAlg =
      AlgorithmManager::Instance().create("ExtractSpectra");
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

MatrixWorkspace_sptr
IndirectFitPlotModel::appendSpectra(MatrixWorkspace_sptr inputWS,
                                    MatrixWorkspace_sptr spectraWS) const {
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

MatrixWorkspace_sptr
IndirectFitPlotModel::cropWorkspace(MatrixWorkspace_sptr inputWS, double startX,
                                    double endX, int startIndex,
                                    int endIndex) const {
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
  auto deleteWorkspaceAlg =
      AlgorithmManager::Instance().create("DeleteWorkspace");
  deleteWorkspaceAlg->initialize();
  deleteWorkspaceAlg->setChild(true);
  deleteWorkspaceAlg->setLogging(false);
  deleteWorkspaceAlg->setProperty("Workspace", name);
  deleteWorkspaceAlg->execute();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
