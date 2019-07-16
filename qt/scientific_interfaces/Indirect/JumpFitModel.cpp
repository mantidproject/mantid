// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "JumpFitModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/TextAxis.h"

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;
using IDAWorkspaceIndex = MantidQt::CustomInterfaces::IDA::WorkspaceIndex;

struct ContainsOneOrMore {
  explicit ContainsOneOrMore(std::vector<std::string> &&substrings)
      : m_substrings(std::move(substrings)) {}

  bool operator()(const std::string &str) const {
    for (const auto &substring : m_substrings) {
      if (str.rfind(substring) != std::string::npos)
        return true;
    }
    return false;
  }

private:
  std::vector<std::string> m_substrings;
};

template <typename Predicate>
std::pair<std::vector<std::string>, std::vector<IDAWorkspaceIndex>>
findAxisLabels(TextAxis *axis, Predicate const &predicate) {
  std::vector<std::string> labels;
  std::vector<IDAWorkspaceIndex> spectra;

  for (auto i = 0u; i < axis->length(); ++i) {
    auto label = axis->label(i);
    if (predicate(label)) {
      labels.emplace_back(label);
      spectra.emplace_back(IDAWorkspaceIndex{static_cast<int>(i)});
    }
  }
  return std::make_pair(labels, spectra);
}

template <typename Predicate>
std::pair<std::vector<std::string>, std::vector<IDAWorkspaceIndex>>
findAxisLabels(MatrixWorkspace const *workspace, Predicate const &predicate) {
  auto axis = dynamic_cast<TextAxis *>(workspace->getAxis(1));
  if (axis)
    return findAxisLabels(axis, predicate);
  return std::make_pair(std::vector<std::string>(), std::vector<IDAWorkspaceIndex>());
}

Spectra createSpectra(IDAWorkspaceIndex spectrum) {
  return Spectra(spectrum, spectrum);
}

std::string getHWHMName(const std::string &resultName) {
  auto position = resultName.rfind("_FWHM");
  if (position != std::string::npos)
    return resultName.substr(0, position) + "_HWHM" +
           resultName.substr(position + 5, resultName.size());
  return resultName + "_HWHM";
}

JumpFitParameters createJumpFitParameters(MatrixWorkspace *workspace) {
  auto foundWidths =
      findAxisLabels(workspace, ContainsOneOrMore({".Width", ".FWHM"}));
  auto foundEISF = findAxisLabels(workspace, ContainsOneOrMore({".EISF"}));

  JumpFitParameters parameters;
  parameters.widths = foundWidths.first;
  parameters.widthSpectra = foundWidths.second;
  parameters.eisf = foundEISF.first;
  parameters.eisfSpectra = foundEISF.second;
  return parameters;
}

void deleteTemporaryWorkspaces(std::vector<std::string> const &workspaceNames) {
  auto deleter = AlgorithmManager::Instance().create("DeleteWorkspace");
  deleter->setLogging(false);
  for (auto const &name : workspaceNames) {
    deleter->setProperty("Workspace", name);
    deleter->execute();
  }
}

std::string scaleWorkspace(std::string const &inputName,
                           std::string const &outputName, double factor) {
  auto scaleAlg = AlgorithmManager::Instance().create("Scale");
  scaleAlg->initialize();
  scaleAlg->setLogging(false);
  scaleAlg->setProperty("InputWorkspace", inputName);
  scaleAlg->setProperty("OutputWorkspace", outputName);
  scaleAlg->setProperty("Factor", factor);
  scaleAlg->execute();
  return outputName;
}

std::string extractSpectra(std::string const &inputName, IDAWorkspaceIndex startIndex,
                           IDAWorkspaceIndex endIndex, std::string const &outputName) {
  auto extractAlg = AlgorithmManager::Instance().create("ExtractSpectra");
  extractAlg->initialize();
  extractAlg->setLogging(false);
  extractAlg->setProperty("InputWorkspace", inputName);
  extractAlg->setProperty("StartWorkspaceIndex", startIndex.value);
  extractAlg->setProperty("EndWorkspaceIndex", endIndex.value);
  extractAlg->setProperty("OutputWorkspace", outputName);
  extractAlg->execute();
  return outputName;
}

std::string extractSpectrum(MatrixWorkspace_sptr workspace, IDAWorkspaceIndex index,
                            std::string const &outputName) {
  return extractSpectra(workspace->getName(), index, index, outputName);
}

std::string extractHWHMSpectrum(MatrixWorkspace_sptr workspace, IDAWorkspaceIndex index) {
  auto const scaledName = "__scaled_" + std::to_string(index.value);
  auto const extractedName = "__extracted_" + std::to_string(index.value);
  auto const outputName = scaleWorkspace(
      extractSpectrum(workspace, index, extractedName), scaledName, 0.5);
  deleteTemporaryWorkspaces({extractedName});
  return outputName;
}

std::string appendWorkspace(std::string const &lhsName,
                            std::string const &rhsName,
                            std::string const &outputName) {
  auto appendAlg = AlgorithmManager::Instance().create("AppendSpectra");
  appendAlg->initialize();
  appendAlg->setLogging(false);
  appendAlg->setProperty("InputWorkspace1", lhsName);
  appendAlg->setProperty("InputWorkspace2", rhsName);
  appendAlg->setProperty("OutputWorkspace", outputName);
  appendAlg->execute();
  return outputName;
}

MatrixWorkspace_sptr appendAll(std::vector<std::string> const &workspaces,
                               std::string const &outputName) {
  auto appended = workspaces[0];
  for (auto i = 1u; i < workspaces.size(); ++i)
    appended = appendWorkspace(appended, workspaces[i], outputName);
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(appended);
}

std::vector<std::string>
subdivideWidthWorkspace(MatrixWorkspace_sptr workspace,
                        const std::vector<IDAWorkspaceIndex> &widthSpectra) {
  std::vector<std::string> subworkspaces;
  subworkspaces.reserve(1 + 2 * widthSpectra.size());

  IDAWorkspaceIndex start{0};
  for (auto spectrum : widthSpectra) {
    if (spectrum > start) {
      auto const outputName = "__extracted_" + std::to_string(start.value) + "_to_" +
                              std::to_string(spectrum.value);
      subworkspaces.emplace_back(extractSpectra(workspace->getName(), start,
                                                spectrum - IDAWorkspaceIndex{1}, outputName));
    }
    subworkspaces.emplace_back(extractHWHMSpectrum(workspace, spectrum));
    start = spectrum + IDAWorkspaceIndex{1};
  }

  IDAWorkspaceIndex end{static_cast<int>(workspace->getNumberHistograms())};
  if (start < end) {
    auto const outputName =
        "__extracted_" + std::to_string(start.value) + "_to_" + std::to_string(end.value);
    subworkspaces.emplace_back(
        extractSpectra(workspace->getName(), start, end - IDAWorkspaceIndex{1}, outputName));
  }
  return subworkspaces;
}

MatrixWorkspace_sptr
createHWHMWorkspace(MatrixWorkspace_sptr workspace, const std::string &hwhmName,
                    const std::vector<IDAWorkspaceIndex> &widthSpectra) {
  if (widthSpectra.empty())
    return workspace;
  if (AnalysisDataService::Instance().doesExist(hwhmName))
    return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        hwhmName);

  const auto subworkspaces = subdivideWidthWorkspace(workspace, widthSpectra);
  const auto hwhmWorkspace = appendAll(subworkspaces, hwhmName);
  auto axis = dynamic_cast<TextAxis *>(
      workspace->getAxis(1)->clone(hwhmWorkspace.get()));
  hwhmWorkspace->replaceAxis(1, std::unique_ptr<TextAxis>(axis));

  deleteTemporaryWorkspaces(subworkspaces);

  return hwhmWorkspace;
}

boost::optional<IDAWorkspaceIndex>
getFirstSpectrum(const JumpFitParameters &parameters) {
  if (!parameters.widthSpectra.empty())
    return parameters.widthSpectra.front();
  else if (!parameters.eisfSpectra.empty())
    return parameters.eisfSpectra.front();
  return boost::none;
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

void JumpFitModel::addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                                const Spectra & /*spectra*/) {
  const auto name = getHWHMName(workspace->getName());
  const auto parameters = addJumpFitParameters(workspace.get(), name);

  const auto spectrum = getFirstSpectrum(parameters);
  if (!spectrum)
    throw std::invalid_argument("Workspace contains no Width or EISF spectra.");

  if (workspace->y(0).size() == 1)
    throw std::invalid_argument("Workspace contains only one data point.");

  const auto hwhmWorkspace =
      createHWHMWorkspace(workspace, name, parameters.widthSpectra);
  IndirectFittingModel::addNewWorkspace(hwhmWorkspace,
                                        createSpectra(*spectrum));
}

void JumpFitModel::removeWorkspace(DatasetIndex index) {
  m_jumpParameters.erase(getWorkspace(index)->getName());
  IndirectFittingModel::removeFittingData(index);
}

JumpFitParameters &
JumpFitModel::addJumpFitParameters(MatrixWorkspace *workspace,
                                   const std::string &hwhmName) {
  const auto &foundParameters = m_jumpParameters.find(hwhmName);
  if (foundParameters != m_jumpParameters.end())
    return foundParameters->second;

  const auto parameters = createJumpFitParameters(workspace);
  if (parameters.widths.empty() && parameters.eisf.empty())
    throw std::invalid_argument("Workspace contains no Width or EISF spectra.");
  return m_jumpParameters[hwhmName] = std::move(parameters);
}

std::unordered_map<std::string, JumpFitParameters>::const_iterator
JumpFitModel::findJumpFitParameters(DatasetIndex dataIndex) const {
  const auto ws = getWorkspace(dataIndex);
  if (!ws)
    return m_jumpParameters.end();
  return m_jumpParameters.find(ws->getName());
}

std::string JumpFitModel::getFitParameterName(DatasetIndex dataIndex,
              WorkspaceIndex spectrum) const {
  const auto ws = getWorkspace(dataIndex);
  const auto axis = dynamic_cast<TextAxis *>(ws->getAxis(1));
  return axis->label(spectrum.value);
}

void JumpFitModel::setActiveWidth(std::size_t widthIndex,
                                  DatasetIndex dataIndex) {
  const auto parametersIt = findJumpFitParameters(dataIndex);
  if (parametersIt != m_jumpParameters.end() &&
      parametersIt->second.widthSpectra.size() > widthIndex) {
    const auto &widthSpectra = parametersIt->second.widthSpectra;
    setSpectra(createSpectra(widthSpectra[widthIndex]), dataIndex);
  } else
    throw std::runtime_error("Invalid width index specified.");
}

void JumpFitModel::setActiveEISF(std::size_t eisfIndex, DatasetIndex dataIndex) {
  const auto parametersIt = findJumpFitParameters(dataIndex);
  if (parametersIt != m_jumpParameters.end() &&
      parametersIt->second.eisfSpectra.size() > eisfIndex) {
    const auto &eisfSpectra = parametersIt->second.eisfSpectra;
    setSpectra(createSpectra(eisfSpectra[eisfIndex]), dataIndex);
  } else
    throw std::runtime_error("Invalid EISF index specified.");
}

void JumpFitModel::setFitType(const std::string &fitType) {
  m_fitType = fitType;
}

bool JumpFitModel::zeroWidths(DatasetIndex dataIndex) const {
  const auto parameters = findJumpFitParameters(dataIndex);
  if (parameters != m_jumpParameters.end())
    return parameters->second.widths.empty();
  return true;
}

bool JumpFitModel::zeroEISF(DatasetIndex dataIndex) const {
  const auto parameters = findJumpFitParameters(dataIndex);
  if (parameters != m_jumpParameters.end())
    return parameters->second.eisf.empty();
  return true;
}

bool JumpFitModel::isMultiFit() const {
  if (numberOfWorkspaces().value == 0)
    return false;
  return !allWorkspacesEqual(getWorkspace(DatasetIndex{0}));
}

std::vector<std::string> JumpFitModel::getSpectrumDependentAttributes() const {
  return {};
}

std::vector<std::string> JumpFitModel::getWidths(DatasetIndex dataIndex) const {
  const auto parameters = findJumpFitParameters(dataIndex);
  if (parameters != m_jumpParameters.end())
    return parameters->second.widths;
  return std::vector<std::string>();
}

std::vector<std::string> JumpFitModel::getEISF(DatasetIndex dataIndex) const {
  const auto parameters = findJumpFitParameters(dataIndex);
  if (parameters != m_jumpParameters.end())
    return parameters->second.eisf;
  return std::vector<std::string>();
}

boost::optional<WorkspaceIndex>
JumpFitModel::getWidthSpectrum(std::size_t widthIndex,
                               DatasetIndex dataIndex) const {
  const auto parameters = findJumpFitParameters(dataIndex);
  if (parameters != m_jumpParameters.end() &&
      parameters->second.widthSpectra.size() > widthIndex)
    return parameters->second.widthSpectra[widthIndex];
  return boost::none;
}

boost::optional<WorkspaceIndex>
JumpFitModel::getEISFSpectrum(std::size_t eisfIndex,
                              DatasetIndex dataIndex) const {
  const auto parameters = findJumpFitParameters(dataIndex);
  if (parameters != m_jumpParameters.end() &&
      parameters->second.eisfSpectra.size() > eisfIndex)
    return parameters->second.eisfSpectra[eisfIndex];
  return boost::none;
}

std::string JumpFitModel::sequentialFitOutputName() const {
  if (isMultiFit())
    return "MultiFofQFit_" + m_fitType + "_Results";
  return constructOutputName();
}

std::string JumpFitModel::simultaneousFitOutputName() const {
  return sequentialFitOutputName();
}

std::string JumpFitModel::singleFitOutputName(DatasetIndex index,
        WorkspaceIndex spectrum) const {
  return createSingleFitOutputName("%1%_FofQFit_" + m_fitType + "_s%2%_Results",
                                   index, spectrum);
}

std::string JumpFitModel::getResultXAxisUnit() const { return ""; }

std::string JumpFitModel::getResultLogName() const { return "SourceName"; }

std::string JumpFitModel::constructOutputName() const {
  auto const name = createOutputName("%1%_FofQFit_" + m_fitType, "", DatasetIndex{0});
  auto const position = name.find("_Results");
  if (position != std::string::npos)
    return name.substr(0, position) + name.substr(position + 7, name.size());
  return name;
}

bool JumpFitModel::allWorkspacesEqual(
    Mantid::API::MatrixWorkspace_sptr workspace) const {
  for (auto i = DatasetIndex{1}; i < numberOfWorkspaces(); ++i) {
    if (getWorkspace(i) != workspace)
      return false;
  }
  return true;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
