#include "JumpFitModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/TextAxis.h"

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

struct ContainsOneOrMore {
  ContainsOneOrMore(std::vector<std::string> &&substrings)
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

MatrixWorkspace_sptr scaleWorkspace(MatrixWorkspace_sptr workspace,
                                    const std::string &output, double factor) {
  auto scaleAlg = AlgorithmManager::Instance().create("Scale");
  scaleAlg->initialize();
  scaleAlg->setProperty("InputWorkspace", workspace);
  scaleAlg->setProperty("OutputWorkspace", output);
  scaleAlg->setProperty("Factor", factor);
  scaleAlg->execute();
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(output);
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
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

void JumpFitModel::addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                                const Spectra &) {
  const auto &jumpParameters = addJumpFitParameters(workspace.get());
  const auto noWidths = jumpParameters.widths.empty();
  const auto name = getHWHMName(workspace->getName());
  const auto spectra =
      noWidths ? jumpParameters.eisfSpectra[0] : jumpParameters.widthSpectra[0];
  IndirectFittingModel::addWorkspace(scaleWorkspace(workspace, name, 0.5),
                                     createSpectra(spectra));
}

JumpFitParameters &
JumpFitModel::addJumpFitParameters(MatrixWorkspace *workspace) {
  const auto &foundParameters = m_jumpParameters.find(workspace);
  if (foundParameters != m_jumpParameters.end())
    return foundParameters->second;

  const auto parameters = createJumpFitParameters(workspace);
  if (parameters.widths.empty() && parameters.eisf.empty())
    throw std::invalid_argument("Workspace contains no Width or EISF spectra.");
  return m_jumpParameters[workspace] = std::move(parameters);
}

std::unordered_map<Mantid::API::MatrixWorkspace *,
                   JumpFitParameters>::const_iterator
JumpFitModel::findJumpFitParameters(std::size_t dataIndex) const {
  const auto workspace = getWorkspace(dataIndex).get();
  if (!workspace)
    return m_jumpParameters.end();
  return m_jumpParameters.find(workspace);
}

void JumpFitModel::setActiveWidth(std::size_t widthIndex,
                                  std::size_t dataIndex) {
  const auto parametersIt = findJumpFitParameters(dataIndex);

  if (parametersIt != m_jumpParameters.end()) {
    const auto &widthSpectra = parametersIt->second.widthSpectra;
    const auto spectra = createSpectra(widthSpectra[widthIndex]);
    setSpectra(spectra, dataIndex);
  }
}

void JumpFitModel::setActiveEISF(std::size_t eisfIndex, std::size_t dataIndex) {
  const auto parametersIt = findJumpFitParameters(dataIndex);

  if (parametersIt != m_jumpParameters.end()) {
    const auto &eisfSpectra = parametersIt->second.eisfSpectra;
    const auto spectra = createSpectra(eisfSpectra[eisfIndex]);
    setSpectra(spectra, dataIndex);
  }
}

void JumpFitModel::setFitType(const std::string &fitType) {
  m_fitType = fitType;
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
  auto name = createOutputName("%1%_JumpFit", "", 0);
  auto position = name.find("_Result");
  if (position != std::string::npos)
    return name.substr(0, position) + name.substr(position + 7, name.size());
  return name;
}

std::string JumpFitModel::simultaneousFitOutputName() const {
  return sequentialFitOutputName();
}

std::string JumpFitModel::singleFitOutputName(std::size_t, std::size_t) const {
  return sequentialFitOutputName();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
