// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/MaskBinsData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <vector>

namespace MantidQt::MantidWidgets {

/// Add a range of x values for bin masking.
void MaskBinsData::addXRange(double start, double end, const std::vector<size_t> &indices) {
  BinMask range(start, end);
  range.spectra = indices;
  m_masks.append(range);
}

/// Mask a given workspace according to the stored ranges.
/// @param wsName :: A workspace to mask.
void MaskBinsData::mask(std::shared_ptr<Mantid::API::MatrixWorkspace> &workspace) const {
  for (const auto &binMask : m_masks) {
    auto &spectra = binMask.spectra;
    std::vector<int64_t> spectraList(spectra.size());
    std::transform(spectra.cbegin(), spectra.cend(), spectraList.begin(),
                   [](const size_t spec) -> int { return static_cast<int>(spec); });
    auto alg = Mantid::API::AlgorithmManager::Instance().create("MaskBins", -1);
    alg->setProperty("InputWorkspace", workspace);
    alg->getPointerToProperty("OutputWorkspace")->createTemporaryValue();
    alg->setProperty("OutputWorkspace", workspace);
    alg->setProperty("InputWorkspaceIndexSet", spectraList);
    alg->setProperty("XMin", binMask.start);
    alg->setProperty("XMax", binMask.end);
    alg->execute();
  }
}

/// Check if there is no data
bool MaskBinsData::isEmpty() const { return m_masks.isEmpty(); }

/// Subtract integrated counts in the masked bins from given vector of
/// integrated spectra.
/// @param workspace :: A workspace to integrate.
/// @param spectraIntgrs :: An in/out vector with integrated spectra. On input
/// it must contain
///   integrals from workspace for all its spectra.
void MaskBinsData::subtractIntegratedSpectra(const Mantid::API::MatrixWorkspace &workspace,
                                             std::vector<double> &spectraIntgrs) const {
  for (const auto &binMask : m_masks) {
    std::vector<double> subtract;
    workspace.getIntegratedSpectra(subtract, binMask.start, binMask.end, false);
    auto &spectra = binMask.spectra;
    for (const auto &ispec : spectra) {
      auto counts = spectraIntgrs[ispec] - subtract[ispec];
      spectraIntgrs[ispec] = counts >= 0.0 ? counts : 0.0;
    }
  }
}

/// Clear the masking data
void MaskBinsData::clear() { m_masks.clear(); }

/** Load mask bins state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 */
void MaskBinsData::loadFromProject(const std::string &lines) {
  Q_UNUSED(lines);
  throw std::runtime_error("MaskBinsData::loadFromProject() not implemented for Qt >= 5");
}

/** Save the state of the mask bins to a Mantid project file
 * @return a string representing the state of the mask bins
 */
std::string MaskBinsData::saveToProject() const {
  throw std::runtime_error("MaskBinsData::saveToProject() not implemented for Qt >= 5");
}

} // namespace MantidQt::MantidWidgets
