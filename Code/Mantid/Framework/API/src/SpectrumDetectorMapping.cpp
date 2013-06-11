#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace API
{
  /** Constructor that fills the map from the spectrum-detector relationships in the given workspace.
   *  @throws std::invalid_argument if a null workspace pointer is passed in
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

  /** Constructor that fills the map from a pair of vectors.
   *  @throws std::invalid_argument if the vectors are not of equal length
   */
  SpectrumDetectorMapping::SpectrumDetectorMapping(const std::vector<specid_t>& spectrumNumbers , const std::vector<detid_t>& detectorIDs)
  {
    if ( spectrumNumbers.size() != detectorIDs.size() )
    {
      throw std::invalid_argument("SpectrumDetectorMapping: Different length spectrum number & detector ID array passed");
    }

    fillMapFromArray( spectrumNumbers.data(), detectorIDs.data(), spectrumNumbers.size() );
  }

  /** Constructor that fills the map from a pair c-style arrays.
   *  Not safe! Prefer the vector constructor where possible!
   *  @throws std::invalid_argument if a null array pointer is passed in
   */
  SpectrumDetectorMapping::SpectrumDetectorMapping(const specid_t* const spectrumNumbers, const detid_t* const detectorIDs, size_t arrayLengths)
  {
    if ( spectrumNumbers == NULL || detectorIDs == NULL )
    {
      throw std::invalid_argument("SpectrumDetectorMapping: Null array pointer passed");
    }

    fillMapFromArray(spectrumNumbers, detectorIDs, arrayLengths);
  }

  /// Called by the vector & c-array constructors to do the actual filling
  void SpectrumDetectorMapping::fillMapFromArray(const specid_t * const spectrumNumbers,
      const detid_t * const detectorIDs, const size_t arrayLengths)
  {
    for ( size_t i = 0; i < arrayLengths; ++i )
    {
      m_mapping[spectrumNumbers[i]].insert(detectorIDs[i]);
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
