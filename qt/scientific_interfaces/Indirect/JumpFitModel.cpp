#include "JumpFitModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/TextAxis.h"

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

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
std::pair<std::vector<std::string>, std::vector<std::size_t>>
findAxisLabels(TextAxis *axis, Predicate const &predicate) {
  std::vector<std::string> labels;
  std::vector<std::size_t> spectra;

  for (auto i = 0u; i < axis->length(); ++i) {
    auto label = axis->label(i);
    if (predicate(label)) {
      labels.emplace_back(label);
      spectra.emplace_back(i);
    }
  }
  return std::make_pair(labels, spectra);
}

template <typename Predicate>
std::pair<std::vector<std::string>, std::vector<std::size_t>>
findAxisLabels(MatrixWorkspace const *workspace, Predicate const &predicate) {
  auto axis = dynamic_cast<TextAxis *>(workspace->getAxis(1));
  if (axis)
    return findAxisLabels(axis, predicate);
  return std::make_pair(std::vector<std::string>(), std::vector<std::size_t>());
}

Spectra createSpectra(std::size_t spectrum) {
  return std::make_pair(spectrum, spectrum);
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

MatrixWorkspace_sptr scaleWorkspace(MatrixWorkspace_sptr workspace,
                                    double factor) {
  auto scaleAlg = AlgorithmManager::Instance().create("Scale");
  scaleAlg->initialize();
  scaleAlg->setLogging(false);
  scaleAlg->setChild(true);
  scaleAlg->setProperty("InputWorkspace", workspace);
  scaleAlg->setProperty("OutputWorkspace", "__scaled");
  scaleAlg->setProperty("Factor", factor);
  scaleAlg->execute();
  return scaleAlg->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr extractSpectra(MatrixWorkspace_sptr workspace,
                                    int startIndex, int endIndex) {
  auto extractAlg = AlgorithmManager::Instance().create("ExtractSpectra");
  extractAlg->initialize();
  extractAlg->setChild(true);
  extractAlg->setLogging(false);
  extractAlg->setProperty("InputWorkspace", workspace);
  extractAlg->setProperty("StartWorkspaceIndex", startIndex);
  extractAlg->setProperty("EndWorkspaceIndex", endIndex);
  extractAlg->setProperty("OutputWorkspace", "__extracted");
  extractAlg->execute();
  return extractAlg->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr extractSpectrum(MatrixWorkspace_sptr workspace,
                                     int index) {
  return extractSpectra(workspace, index, index);
}

MatrixWorkspace_sptr extractHWHMSpectrum(MatrixWorkspace_sptr workspace,
                                         int index) {
  return scaleWorkspace(extractSpectrum(workspace, index), 0.5);
}

MatrixWorkspace_sptr appendWorkspace(MatrixWorkspace_sptr lhs,
                                     MatrixWorkspace_sptr rhs) {
  auto appendAlg = AlgorithmManager::Instance().create("AppendSpectra");
  appendAlg->initialize();
  appendAlg->setChild(true);
  appendAlg->setLogging(false);
  appendAlg->setProperty("InputWorkspace1", lhs);
  appendAlg->setProperty("InputWorkspace2", rhs);
  appendAlg->setProperty("OutputWorkspace", "__appended");
  appendAlg->execute();
  return appendAlg->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr
appendAll(const std::vector<MatrixWorkspace_sptr> &workspaces) {
  auto appended = workspaces.front();
  for (auto i = 1u; i < workspaces.size(); ++i)
    appended = appendWorkspace(appended, workspaces[i]);
  return appended;
}

MatrixWorkspace_sptr addToADS(MatrixWorkspace_sptr workspace,
                              const std::string &name) {
  AnalysisDataService::Instance().addOrReplace(name, workspace);
  return workspace;
}

std::vector<MatrixWorkspace_sptr>
subdivideWidthWorkspace(MatrixWorkspace_sptr workspace,
                        const std::vector<std::size_t> &widthSpectra) {
  std::vector<MatrixWorkspace_sptr> subworkspaces;
  subworkspaces.reserve(1 + 2 * widthSpectra.size());

  int start = 0;
  for (auto i = 0u; i < widthSpectra.size(); ++i) {
    const auto spectrum = static_cast<int>(widthSpectra[i]);
    if (spectrum > start)
      subworkspaces.emplace_back(
          extractSpectra(workspace, start, spectrum - 1));
    subworkspaces.emplace_back(extractHWHMSpectrum(workspace, spectrum));
    start = spectrum + 1;
  }

  const int end = static_cast<int>(workspace->getNumberHistograms());
  if (start < end)
    subworkspaces.emplace_back(extractSpectra(workspace, start, end - 1));
  return subworkspaces;
}

MatrixWorkspace_sptr
createHWHMWorkspace(MatrixWorkspace_sptr workspace, const std::string &hwhmName,
                    const std::vector<std::size_t> &widthSpectra) {
  if (widthSpectra.empty())
    return workspace;
  if (AnalysisDataService::Instance().doesExist(hwhmName))
    return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        hwhmName);

  const auto subworkspaces = subdivideWidthWorkspace(workspace, widthSpectra);
  const auto hwhmWorkspace = appendAll(subworkspaces);
  const auto axis = workspace->getAxis(1)->clone(hwhmWorkspace.get());
  hwhmWorkspace->replaceAxis(1, dynamic_cast<TextAxis *>(axis));
  return addToADS(hwhmWorkspace, hwhmName);
}

boost::optional<std::size_t>
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
                                const Spectra &) {
  const auto name = getHWHMName(workspace->getName());
  const auto parameters = addJumpFitParameters(workspace.get(), name);

  const auto spectrum = getFirstSpectrum(parameters);
  if (!spectrum)
    throw std::invalid_argument("Workspace contains no Width or EISF spectra.");

  const auto hwhmWorkspace =
      createHWHMWorkspace(workspace, name, parameters.widthSpectra);
  IndirectFittingModel::addNewWorkspace(hwhmWorkspace,
                                        createSpectra(*spectrum));
}

void JumpFitModel::removeWorkspace(std::size_t index) {
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
JumpFitModel::findJumpFitParameters(std::size_t dataIndex) const {
  const auto workspace = getWorkspace(dataIndex);
  if (!workspace)
    return m_jumpParameters.end();
  return m_jumpParameters.find(workspace->getName());
}

std::string JumpFitModel::getFitParameterName(std::size_t dataIndex,
                                              std::size_t spectrum) const {
  const auto workspace = getWorkspace(dataIndex);
  const auto axis = dynamic_cast<TextAxis *>(workspace->getAxis(1));
  return axis->label(spectrum);
}

void JumpFitModel::setActiveWidth(std::size_t widthIndex,
                                  std::size_t dataIndex) {
  const auto parametersIt = findJumpFitParameters(dataIndex);
  if (parametersIt != m_jumpParameters.end()) {
    const auto &widthSpectra = parametersIt->second.widthSpectra;
    setSpectra(createSpectra(widthSpectra[widthIndex]), dataIndex);
  } else
    throw std::runtime_error("Invalid width index specified.");
}

void JumpFitModel::setActiveEISF(std::size_t eisfIndex, std::size_t dataIndex) {
  const auto parametersIt = findJumpFitParameters(dataIndex);
  if (parametersIt != m_jumpParameters.end()) {
    const auto &eisfSpectra = parametersIt->second.eisfSpectra;
    setSpectra(createSpectra(eisfSpectra[eisfIndex]), dataIndex);
  } else
    throw std::runtime_error("Invalid EISF index specified.");
}

void JumpFitModel::setFitType(const std::string &fitType) {
  m_fitType = fitType;
}

bool JumpFitModel::zeroWidths(std::size_t dataIndex) const {
  const auto parameters = findJumpFitParameters(dataIndex);
  if (parameters != m_jumpParameters.end())
    return parameters->second.widths.empty();
  return true;
}

bool JumpFitModel::zeroEISF(std::size_t dataIndex) const {
  const auto parameters = findJumpFitParameters(dataIndex);
  if (parameters != m_jumpParameters.end())
    return parameters->second.eisf.empty();
  return true;
}

bool JumpFitModel::isMultiFit() const {
  if (numberOfWorkspaces() == 0)
    return false;
  return !allWorkspacesEqual(getWorkspace(0));
}

std::vector<std::string> JumpFitModel::getWidths(std::size_t dataIndex) const {
  const auto parameters = findJumpFitParameters(dataIndex);
  if (parameters != m_jumpParameters.end())
    return parameters->second.widths;
  return std::vector<std::string>();
}

std::vector<std::string> JumpFitModel::getEISF(std::size_t dataIndex) const {
  const auto parameters = findJumpFitParameters(dataIndex);
  if (parameters != m_jumpParameters.end())
    return parameters->second.eisf;
  return std::vector<std::string>();
}

boost::optional<std::size_t>
JumpFitModel::getWidthSpectrum(std::size_t widthIndex,
                               std::size_t dataIndex) const {
  const auto parameters = findJumpFitParameters(dataIndex);
  if (parameters != m_jumpParameters.end())
    return parameters->second.widthSpectra[widthIndex];
  return boost::none;
}

boost::optional<std::size_t>
JumpFitModel::getEISFSpectrum(std::size_t eisfIndex,
                              std::size_t dataIndex) const {
  const auto parameters = findJumpFitParameters(dataIndex);
  if (parameters != m_jumpParameters.end())
    return parameters->second.eisfSpectra[eisfIndex];
  return boost::none;
}

std::string JumpFitModel::sequentialFitOutputName() const {
  if (isMultiFit())
    return "MultiFofQFit_" + m_fitType + "_Result";
  return constructOutputName();
}

std::string JumpFitModel::simultaneousFitOutputName() const {
  return sequentialFitOutputName();
}

std::string JumpFitModel::singleFitOutputName(std::size_t, std::size_t) const {
  return sequentialFitOutputName();
}

std::string JumpFitModel::constructOutputName() const {
  auto name = createOutputName("%1%_FofQFit", "", 0);
  auto position = name.find("_Result");
  if (position != std::string::npos)
    return name.substr(0, position) + name.substr(position + 7, name.size());
  return name;
}

bool JumpFitModel::allWorkspacesEqual(
    Mantid::API::MatrixWorkspace_sptr workspace) const {
  for (auto i = 1u; i < numberOfWorkspaces(); ++i) {
    if (getWorkspace(i) != workspace)
      return false;
  }
  return true;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
