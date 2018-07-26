#include "MantidQtWidgets/InstrumentView/MaskBinsData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"

#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/// Add a range of x values for bin masking.
void MaskBinsData::addXRange(double start, double end,
                             const std::vector<size_t> &indices) {
  BinMask range(start, end);
  range.spectra = indices;
  m_masks.append(range);
}

/// Mask a given workspace according to the stored ranges.
/// @param wsName :: A workspace to mask.
void MaskBinsData::mask(const std::string &wsName) const {
  for (auto mask = m_masks.begin(); mask != m_masks.end(); ++mask) {
    auto &spectra = mask->spectra;
    std::vector<int64_t> spectraList(spectra.size());
    std::transform(spectra.cbegin(), spectra.cend(), spectraList.begin(),
                   [](const size_t spec)
                       -> int { return static_cast<int>(spec); });
    auto alg = Mantid::API::AlgorithmManager::Instance().create("MaskBins", -1);
    alg->setPropertyValue("InputWorkspace", wsName);
    alg->setPropertyValue("OutputWorkspace", wsName);
    alg->setProperty("InputWorkspaceIndexSet", spectraList);
    alg->setProperty("XMin", mask->start);
    alg->setProperty("XMax", mask->end);
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
void MaskBinsData::subtractIntegratedSpectra(
    const Mantid::API::MatrixWorkspace &workspace,
    std::vector<double> &spectraIntgrs) const {
  for (auto mask = m_masks.begin(); mask != m_masks.end(); ++mask) {
    std::vector<double> subtract;
    workspace.getIntegratedSpectra(subtract, mask->start, mask->end, false);
    auto &spectra = mask->spectra;
    for (auto ispec = spectra.begin(); ispec != spectra.end(); ++ispec) {
      auto counts = spectraIntgrs[*ispec] - subtract[*ispec];
      spectraIntgrs[*ispec] = counts >= 0.0 ? counts : 0.0;
    }
  }
}

/// Clear the masking data
void MaskBinsData::clear() { m_masks.clear(); }

/** Load mask bins state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 */
void MaskBinsData::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);
  for (auto &maskLines : tsv.sections("Mask")) {
    API::TSVSerialiser mask(maskLines);
    mask.selectLine("Range");
    double start, end;
    mask >> start >> end;

    std::vector<size_t> spectra;
    const size_t numSpectra = mask.values("Spectra").size();
    for (size_t i = 0; i < numSpectra; ++i) {
      size_t spectrum;
      mask >> spectrum;
      spectra.push_back(spectrum);
    }

    addXRange(start, end, spectra);
  }
}

/** Save the state of the mask bins to a Mantid project file
 * @return a string representing the state of the mask bins
 */
std::string MaskBinsData::saveToProject() const {
  API::TSVSerialiser tsv;
  for (const auto &binMask : m_masks) {
    API::TSVSerialiser mask;
    mask.writeLine("Range") << binMask.start << binMask.end;
    mask.writeLine("Spectra");
    for (auto spectrum : binMask.spectra) {
      mask << spectrum;
    }
    tsv.writeSection("Mask", mask.outputLines());
  }
  return tsv.outputLines();
}

} // MantidWidgets
} // MantidQt
