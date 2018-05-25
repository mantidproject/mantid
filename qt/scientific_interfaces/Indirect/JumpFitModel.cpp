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
findAxisLabels(MatrixWorkspace_const_sptr workspace,
               Predicate const &predicate) {
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
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

void JumpFitModel::addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                                const Spectra &) {
  IndirectFittingModel::clearWorkspaces();
  findWidths(workspace);

  if (!m_widths.empty()) {
    const auto name = getHWHMName(workspace->getName());
    IndirectFittingModel::addWorkspace(scaleWorkspace(workspace, name, 0.5),
                                       createSpectra(m_widthSpectra[0]));
  }
}

void JumpFitModel::findWidths(MatrixWorkspace_sptr workspace) {
  auto found =
      findAxisLabels(workspace, ContainsOneOrMore({".Width", ".FWHM"}));
  m_widths = found.first;
  m_widthSpectra = found.second;
}

void JumpFitModel::findEISF(MatrixWorkspace_sptr workspace) {
  auto found =
    findAxisLabels(workspace, ContainsOneOrMore({ ".EISF" }));
  m_eisf = found.first;
  m_eisfSpectra = found.second;
}

void JumpFitModel::setActiveWidth(std::size_t widthIndex) {
  setSpectra(createSpectra(m_widthSpectra[widthIndex]), 0);
}

void JumpFitModel::setActiveEISF(std::size_t eisfIndex) {
  setSpectra(createSpectra(m_eisfSpectra[eisfIndex]), 0);
}

void JumpFitModel::setFitType(const std::string &fitType) {
  m_fitType = fitType;
}

const std::vector<std::string> &JumpFitModel::getWidths() const {
  return m_widths;
}

const std::vector<std::string> &JumpFitModel::getEISF() const {
  return m_eisf;
}

std::size_t JumpFitModel::getWidthSpectrum(std::size_t widthIndex) const {
  return m_widthSpectra[widthIndex];
}

std::size_t JumpFitModel::getEISFSpectrum(std::size_t eisfIndex) const {
  return m_eisfSpectra[eisfIndex];
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
