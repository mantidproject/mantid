#include "MaskBinsData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <vector>

bool MaskBinsData::XRange::operator<(const XRange& other) const
{
  if (start == other.start)
  {
    return end < other.end;
  }
  return start < other.start;
}

/// Add a range of x values for bin masking.
void MaskBinsData::addXRange(double start, double end, const QList<int>& indices)
{
  XRange range(start, end);
  m_xRanges[range] = indices;
}

/// Mask a given workspace according to the stored ranges.
/// @param wsName :: A workspace to mask.
void MaskBinsData::mask(const std::string& wsName) const
{
  for(auto range = m_xRanges.begin(); range != m_xRanges.end(); ++range)
  {
    auto &spectra = range.value();
    std::vector<int> spectraList(spectra.begin(), spectra.end());
    auto alg = Mantid::API::AlgorithmManager::Instance().create("MaskDetectors",-1);
    alg->setPropertyValue("InputWorkspace", wsName);
    alg->setPropertyValue("OutputWorkspace", wsName);
    alg->setProperty("SpectraList", spectraList);
    alg->setProperty("XMin", range.key().start);
    alg->setProperty("XMax", range.key().end);
    alg->execute();
  }
}

/// Check if there is no data
bool MaskBinsData::isEmpty() const
{
  return m_xRanges.isEmpty();
}

/// Subtract integrated counts in the masked bins from given vector of integrated spectra.
/// @param workspace :: A workspace to integrate.
/// @param spectraIntgrs :: An in/out vector with integrated spectra. On input it must contain
///   integrals from workspace for all its spectra.
void MaskBinsData::subtractIntegratedSpectra(const Mantid::API::MatrixWorkspace& workspace, std::vector<double>& spectraIntgrs) const
{
  for(auto range = m_xRanges.begin(); range != m_xRanges.end(); ++range)
  {
    std::vector<double> subtract;
    workspace.getIntegratedSpectra(subtract, range.key().start, range.key().end, false);
    auto &spectra = range.value();
    for(auto ispec = spectra.begin(); ispec != spectra.end(); ++ispec)
    {
      auto counts = spectraIntgrs[*ispec] - subtract[*ispec];
      spectraIntgrs[*ispec] = counts >= 0.0 ? counts : 0.0;
    }
  }
}
