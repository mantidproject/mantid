// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FqFitModel.h"

#include <utility>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/Logger.h"

namespace {
Mantid::Kernel::Logger logger("FqFitModel");
}

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

struct ContainsOneOrMore {
  explicit ContainsOneOrMore(std::vector<std::string> &&substrings) : m_substrings(std::move(substrings)) {}

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
std::pair<std::vector<std::string>, std::vector<std::size_t>> findAxisLabels(TextAxis *axis,
                                                                             Predicate const &predicate) {
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
std::pair<std::vector<std::string>, std::vector<std::size_t>> findAxisLabels(MatrixWorkspace const *workspace,
                                                                             Predicate const &predicate) {
  auto axis = dynamic_cast<TextAxis *>(workspace->getAxis(1));
  if (axis)
    return findAxisLabels(axis, predicate);
  return std::make_pair(std::vector<std::string>(), std::vector<std::size_t>());
}

std::string createSpectra(const std::vector<std::size_t> &spectrum) {
  std::string spectra = "";
  for (auto spec : spectrum) {
    spectra.append(std::to_string(spec) + ",");
  }
  return spectra;
}

std::string getHWHMName(const std::string &resultName) {
  auto position = resultName.rfind("_FWHM");
  if (position != std::string::npos)
    return resultName.substr(0, position) + "_HWHM" + resultName.substr(position + 5, resultName.size());
  return resultName + "_HWHM";
}

FqFitParameters createFqFitParameters(MatrixWorkspace *workspace) {
  auto foundWidths = findAxisLabels(workspace, ContainsOneOrMore({".Width", ".FWHM"}));
  auto foundEISF = findAxisLabels(workspace, ContainsOneOrMore({".EISF"}));

  FqFitParameters parameters;
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

std::string scaleWorkspace(std::string const &inputName, std::string const &outputName, double factor) {
  auto scaleAlg = AlgorithmManager::Instance().create("Scale");
  scaleAlg->initialize();
  scaleAlg->setLogging(false);
  scaleAlg->setProperty("InputWorkspace", inputName);
  scaleAlg->setProperty("OutputWorkspace", outputName);
  scaleAlg->setProperty("Factor", factor);
  scaleAlg->execute();
  return outputName;
}

std::string extractSpectra(std::string const &inputName, int startIndex, int endIndex, std::string const &outputName) {
  auto extractAlg = AlgorithmManager::Instance().create("ExtractSpectra");
  extractAlg->initialize();
  extractAlg->setLogging(false);
  extractAlg->setProperty("InputWorkspace", inputName);
  extractAlg->setProperty("StartWorkspaceIndex", startIndex);
  extractAlg->setProperty("EndWorkspaceIndex", endIndex);
  extractAlg->setProperty("OutputWorkspace", outputName);
  extractAlg->execute();
  return outputName;
}

std::string extractSpectrum(const MatrixWorkspace_sptr &workspace, int index, std::string const &outputName) {
  return extractSpectra(workspace->getName(), index, index, outputName);
}

std::string extractHWHMSpectrum(const MatrixWorkspace_sptr &workspace, int index) {
  auto const scaledName = "__scaled_" + std::to_string(index);
  auto const extractedName = "__extracted_" + std::to_string(index);
  auto const outputName = scaleWorkspace(extractSpectrum(std::move(workspace), index, extractedName), scaledName, 0.5);
  deleteTemporaryWorkspaces({extractedName});
  return outputName;
}

std::string appendWorkspace(std::string const &lhsName, std::string const &rhsName, std::string const &outputName) {
  auto appendAlg = AlgorithmManager::Instance().create("AppendSpectra");
  appendAlg->initialize();
  appendAlg->setLogging(false);
  appendAlg->setProperty("InputWorkspace1", lhsName);
  appendAlg->setProperty("InputWorkspace2", rhsName);
  appendAlg->setProperty("OutputWorkspace", outputName);
  appendAlg->execute();
  return outputName;
}

MatrixWorkspace_sptr appendAll(std::vector<std::string> const &workspaces, std::string const &outputName) {
  auto appended = workspaces[0];
  for (auto i = 1u; i < workspaces.size(); ++i)
    appended = appendWorkspace(appended, workspaces[i], outputName);
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(appended);
}

std::vector<std::string> subdivideWidthWorkspace(const MatrixWorkspace_sptr &workspace,
                                                 const std::vector<std::size_t> &widthSpectra) {
  std::vector<std::string> subworkspaces;
  subworkspaces.reserve(1 + 2 * widthSpectra.size());

  int start = 0;
  for (auto spectrum_number : widthSpectra) {
    const auto spectrum = static_cast<int>(spectrum_number);
    if (spectrum > start) {
      auto const outputName = "__extracted_" + std::to_string(start) + "_to_" + std::to_string(spectrum);
      subworkspaces.emplace_back(extractSpectra(workspace->getName(), start, spectrum - 1, outputName));
    }
    subworkspaces.emplace_back(extractHWHMSpectrum(workspace, spectrum));
    start = spectrum + 1;
  }

  const int end = static_cast<int>(workspace->getNumberHistograms());
  if (start < end) {
    auto const outputName = "__extracted_" + std::to_string(start) + "_to_" + std::to_string(end);
    subworkspaces.emplace_back(extractSpectra(workspace->getName(), start, end - 1, outputName));
  }
  return subworkspaces;
}

MatrixWorkspace_sptr createHWHMWorkspace(MatrixWorkspace_sptr workspace, const std::string &hwhmName,
                                         const std::vector<std::size_t> &widthSpectra) {
  if (widthSpectra.empty())
    return workspace;
  if (AnalysisDataService::Instance().doesExist(hwhmName))
    return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(hwhmName);

  const auto subworkspaces = subdivideWidthWorkspace(workspace, widthSpectra);
  const auto hwhmWorkspace = appendAll(subworkspaces, hwhmName);
  auto axis = dynamic_cast<TextAxis *>(workspace->getAxis(1)->clone(hwhmWorkspace.get()));
  hwhmWorkspace->replaceAxis(1, std::unique_ptr<TextAxis>(axis));

  deleteTemporaryWorkspaces(subworkspaces);

  return hwhmWorkspace;
}

boost::optional<std::vector<std::size_t>> getSpectrum(const FqFitParameters &parameters) {
  if (!parameters.widthSpectra.empty())
    return parameters.widthSpectra;
  else if (!parameters.eisfSpectra.empty())
    return parameters.eisfSpectra;
  return boost::none;
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

FqFitModel::FqFitModel() { m_fitType = FQFIT_STRING; }

void FqFitModel::addWorkspace(const std::string &workspaceName) {
  auto workspace = Mantid::API::AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);
  const auto name = getHWHMName(workspace->getName());
  const auto parameters = addFqFitParameters(workspace.get(), name);

  const auto spectrum = getSpectrum(parameters);
  if (!spectrum)
    throw std::invalid_argument("Workspace contains no Width or EISF spectra.");

  if (workspace->y(0).size() == 1)
    throw std::invalid_argument("Workspace contains only one data point.");

  const auto hwhmWorkspace = createHWHMWorkspace(workspace, name, parameters.widthSpectra);
  IndirectFittingModel::addWorkspace(hwhmWorkspace->getName(), FunctionModelSpectra(""));
}

void FqFitModel::removeWorkspace(TableDatasetIndex index) {
  m_fqFitParameters.erase(getWorkspace(index)->getName());
  IndirectFittingModel::removeWorkspace(index);
}

FqFitParameters &FqFitModel::addFqFitParameters(MatrixWorkspace *workspace, const std::string &hwhmName) {
  const auto &foundParameters = m_fqFitParameters.find(hwhmName);
  if (foundParameters != m_fqFitParameters.end())
    return foundParameters->second;

  const auto parameters = createFqFitParameters(workspace);
  if (parameters.widths.empty() && parameters.eisf.empty())
    throw std::invalid_argument("Workspace contains no Width or EISF spectra.");
  return m_fqFitParameters[hwhmName] = std::move(parameters);
}

std::unordered_map<std::string, FqFitParameters>::const_iterator
FqFitModel::findFqFitParameters(TableDatasetIndex dataIndex) const {
  const auto ws = getWorkspace(dataIndex);
  if (!ws)
    return m_fqFitParameters.end();
  return m_fqFitParameters.find(ws->getName());
}

std::string FqFitModel::getFitParameterName(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const {
  const auto ws = getWorkspace(dataIndex);
  const auto axis = dynamic_cast<TextAxis *>(ws->getAxis(1));
  return axis->label(spectrum.value);
}

void FqFitModel::setActiveWidth(std::size_t widthIndex, TableDatasetIndex dataIndex, bool single) {
  const auto parametersIt = findFqFitParameters(dataIndex);
  if (parametersIt != m_fqFitParameters.end() && parametersIt->second.widthSpectra.size() > widthIndex) {
    const auto &widthSpectra = parametersIt->second.widthSpectra;
    if (single == true) {
      setSpectra(createSpectra(std::vector<std::size_t>({widthSpectra[widthIndex]})), dataIndex);
    } else { // In multiple mode the spectra needs to be appending on the
             // existing spectra list.
      auto spectra_vec = std::vector<std::size_t>({widthSpectra[widthIndex]});
      auto spectra = getSpectra(dataIndex);
      for (auto i : spectra) {
        spectra_vec.push_back(i.value);
      }
      setSpectra(createSpectra(spectra_vec), dataIndex);
    }
  } else
    logger.warning("Invalid width index specified.");
}

void FqFitModel::setActiveEISF(std::size_t eisfIndex, TableDatasetIndex dataIndex, bool single) {
  const auto parametersIt = findFqFitParameters(dataIndex);
  if (parametersIt != m_fqFitParameters.end() && parametersIt->second.eisfSpectra.size() > eisfIndex) {
    const auto &eisfSpectra = parametersIt->second.eisfSpectra;
    if (single == true) {
      setSpectra(createSpectra(std::vector<std::size_t>({eisfSpectra[eisfIndex]})), dataIndex);
    } else { // In multiple mode the spectra needs to be appending on the
             // existing spectra list.
      auto spectra_vec = std::vector<std::size_t>({eisfSpectra[eisfIndex]});
      auto spectra = getSpectra(dataIndex);
      for (size_t i = 0; i < spectra.size().value; i++) {
        spectra_vec.push_back(spectra[i].value);
      }
      setSpectra(createSpectra(spectra_vec), dataIndex);
    }
  } else
    logger.warning("Invalid EISF index specified.");
}

void FqFitModel::setFitType(const std::string &fitType) { m_fitString = fitType; }

bool FqFitModel::zeroWidths(TableDatasetIndex dataIndex) const {
  const auto parameters = findFqFitParameters(dataIndex);
  if (parameters != m_fqFitParameters.end())
    return parameters->second.widths.empty();
  return true;
}

bool FqFitModel::zeroEISF(TableDatasetIndex dataIndex) const {
  const auto parameters = findFqFitParameters(dataIndex);
  if (parameters != m_fqFitParameters.end())
    return parameters->second.eisf.empty();
  return true;
}

bool FqFitModel::isMultiFit() const {
  if (numberOfWorkspaces() == TableDatasetIndex{0})
    return false;
  return !allWorkspacesEqual(getWorkspace(TableDatasetIndex{0}));
}

std::vector<std::string> FqFitModel::getWidths(TableDatasetIndex dataIndex) const {
  const auto parameters = findFqFitParameters(dataIndex);
  if (parameters != m_fqFitParameters.end())
    return parameters->second.widths;
  return std::vector<std::string>();
}

std::vector<std::string> FqFitModel::getEISF(TableDatasetIndex dataIndex) const {
  const auto parameters = findFqFitParameters(dataIndex);
  if (parameters != m_fqFitParameters.end())
    return parameters->second.eisf;
  return std::vector<std::string>();
}

boost::optional<std::size_t> FqFitModel::getWidthSpectrum(std::size_t widthIndex, TableDatasetIndex dataIndex) const {
  const auto parameters = findFqFitParameters(dataIndex);
  if (parameters != m_fqFitParameters.end() && parameters->second.widthSpectra.size() > widthIndex)
    return parameters->second.widthSpectra[widthIndex];
  return boost::none;
}

boost::optional<std::size_t> FqFitModel::getEISFSpectrum(std::size_t eisfIndex, TableDatasetIndex dataIndex) const {
  const auto parameters = findFqFitParameters(dataIndex);
  if (parameters != m_fqFitParameters.end() && parameters->second.eisfSpectra.size() > eisfIndex)
    return parameters->second.eisfSpectra[eisfIndex];
  return boost::none;
}

std::string FqFitModel::getResultXAxisUnit() const { return ""; }

std::string FqFitModel::getResultLogName() const { return "SourceName"; }

bool FqFitModel::allWorkspacesEqual(const Mantid::API::MatrixWorkspace_sptr &workspace) const {
  for (auto i = TableDatasetIndex{1}; i < numberOfWorkspaces(); ++i) {
    if (getWorkspace(i) != workspace)
      return false;
  }
  return true;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
