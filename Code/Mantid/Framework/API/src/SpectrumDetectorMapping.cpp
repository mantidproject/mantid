#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace API
{
  /** Constructor that fills the map from the spectrum-detector relationships in the given workspace.
   *  @throws Exception::In
   */
  SpectrumDetectorMapping::SpectrumDetectorMapping(const MatrixWorkspace * const workspace)
  {
    if ( ! workspace )
    {
      throw std::invalid_argument("SpectrumDetectorMapping: Null workspace pointer passed");
    }

    for ( size_t i = 0; i < workspace->getNumberHistograms(); ++i )
    {
      auto spectrum = workspace->getSpectrum(i);
      m_mapping[spectrum->getSpectrumNo()] = spectrum->getDetectorIDs();
    }
  }

  /// Destructor
  SpectrumDetectorMapping::~SpectrumDetectorMapping()
  {}

  const std::set<detid_t>&
  SpectrumDetectorMapping::getDetectorIDsForSpectrumNo(const specid_t spectrumNo) const
  {
    return m_mapping.at(spectrumNo);
  }

  const SpectrumDetectorMapping::sdmap& SpectrumDetectorMapping::getMapping() const
  {
    return m_mapping;
  }

} // namespace API
} // namespace Mantid
