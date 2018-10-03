// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_FLATPLATEABSORPTION_H_
#define MANTID_ALGORITHMS_FLATPLATEABSORPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AbsorptionCorrection.h"

namespace Mantid {
namespace Algorithms {
/** Calculates attenuation due to absorption and scattering in a flat plate/slab
   sample.

    Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as
   the input one. </LI>
    <LI> AttenuationXSection - The attenuation cross-section for the sample
   material in barns. </LI>
    <LI> ScatteringXSection - The scattering cross-section for the sample
   material in barns. </LI>
    <LI> SampleNumberDensity - The number density of the sample in
   Angstrom^-3.</LI>
    <LI> NumberOfWavelengthPoints - The number of wavelength points for which
   numerical integral is calculated (default: all points). </LI>
    <LI> ExpMethod - The method to calculate exponential function (Normal or
   Fast approximation). </LI>
    <LI> SampleHeight - The height of the sample in centimetres. </LI>
    <LI> SampleWidth - The width of the sample in centimetres. </LI>
    <LI> SampleThickness - The thickness of the sample in centimetres. </LI>
    <LI> Emode  - The energy mode (0=elastic, 1=direct geometry, 2=indirect
   geometry) </LI>
    <LI> Efixed - Value of fixed energy: EI (emode=1) or EF (emode=2) (meV)
   </LI>
    <LI> ElementSize - The side dimension of an integration element cube in mm
   (default: 1). </LI>
    </UL>

    This algorithm uses numerical integration method to calculate attenuation
   factors
    resulting from absorption and single scattering in a flat plate sample with
   the dimensions
    and material properties given. Factors are calculated for each spectrum
   (i.e. detector position)
    and wavelength point, as defined by the input workspace.
    The sample is divided up into a small cubes and thenpath lengths through the
   sample are
    calculated for the centre-point of each element and a numerical
    integration is carried out using these path lengths over the volume
   elements.

    @author Russell Taylor, Tessella plc
    @date 15/01/2010
*/
class DLLExport FlatPlateAbsorption : public AbsorptionCorrection {
public:
  /// (Empty) Constructor
  FlatPlateAbsorption();
  /// Algorithm's name
  const std::string name() const override { return "FlatPlateAbsorption"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates bin-by-bin correction factors for attenuation due to "
           "absorption and scattering in a sample of 'flat plate' geometry.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"AbsorptionCorrection"};
  }

protected:
  void initialiseCachedDistances() override;

private:
  void defineProperties() override;
  void retrieveProperties() override;
  std::string sampleXML() override;

  double m_slabHeight;      ///< The height of the sample in m
  double m_slabWidth;       ///< The width of the sample in m
  double m_slabThickness;   ///< The thickness of the sample in m
  int m_numXSlices;         ///< The number of slices in X
  int m_numYSlices;         ///< The number of slices in Y
  int m_numZSlices;         ///< The number of slices in Z
  double m_XSliceThickness; ///< The thickness of an X slice in m
  double m_YSliceThickness; ///< The thickness of a Y slice in m
  double m_ZSliceThickness; ///< The thickness of a Z slice in m
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FLATPLATEABSORPTION_H_*/
