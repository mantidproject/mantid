#include "MantidQtMantidWidgets/InstrumentView/MaskBinsData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtAPI/TSVSerialiser.h"

#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/// Add a range of x values for bin masking.
void MaskBinsData::addXRange(double start, double end,
                             const QList<int> &indices) {
  BinMask range(start, end);
  range.spectra = indices;
  m_masks.append(range);
}

/// Mask a given workspace according to the stored ranges.
/// @param wsName :: A workspace to mask.
void MaskBinsData::mask(const std::string &wsName) const {
  for (auto mask = m_masks.begin(); mask != m_masks.end(); ++mask) {
    auto &spectra = mask->spectra;
    std::vector<int> spectraList(spectra.begin(), spectra.end());
    auto alg = Mantid::API::AlgorithmManager::Instance().create("MaskBins", -1);
    alg->setPropertyValue("InputWorkspace", wsName);
    alg->setPropertyValue("OutputWorkspace", wsName);
    alg->setProperty("SpectraList", spectraList);
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

void MaskBinsData::loadFromProject(const std::string &lines) {
  TSVSerialiser tsv(lines);
  for (auto &maskLines : tsv.sections("Mask")) {
    TSVSerialiser mask(maskLines);
    mask.selectLine("Range");
    double start, end;
    mask >> start >> end;

    QList<int> spectra;
    const size_t numSpectra = mask.values("Spectra").size();
    for (size_t i = 0; i < numSpectra; ++i) {
      int spectrum;
      mask >> spectrum;
      spectra.append(spectrum);
    }

    addXRange(start, end, spectra);
  }
}

std::string MaskBinsData::saveToProject() const {
  TSVSerialiser tsv;
  for (const auto &binMask : m_masks) {
    TSVSerialiser mask;
    mask.writeLine("Range") << binMask.start << binMask.end;
    mask.writeLine("Spectra");
    for (const int spectrum : binMask.spectra) {
      mask << spectrum;
    }
    tsv.writeSection("Mask", mask.outputLines());
  }
  return tsv.outputLines();
}

} // MantidWidgets
} // MantidQt
