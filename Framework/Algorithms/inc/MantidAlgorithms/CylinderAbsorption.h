// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CYLINDERABSORPTION_H_
#define MANTID_ALGORITHMS_CYLINDERABSORPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AbsorptionCorrection.h"

namespace Mantid {
namespace Algorithms {
/** Calculates attenuation due to absorption and scattering in a cylindrical
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
    <LI> ExpMethod - The method to calculate exponential function (Normal of
   Fast approximation). </LI>
    <LI> Emode  - The energy mode (0=elastic, 1=direct geometry, 2=indirect
   geometry) </LI>
    <LI> Efixed - Value of fixed energy: EI (emode=1) or EF (emode=2) (meV)
   </LI>
    <LI> CylinderSampleHeight - The height of the cylindrical sample in
   centimetres. </LI>
    <LI> CylinderSampleRadius - The radius of the cylindrical sample in
   centimetres. </LI>
    <LI> NumberOfSlices - The number of slices into which the cylinder is
   divided for the calculation. </LI>
    <LI> NumberOfAnnuli - The number of annuli into which each slice is divided
   for the calculation. </LI>
    </UL>

    This algorithm uses numerical integration method to calculate attenuation
   factors
    resulting from absorption and single scattering in a cylindrical sample with
   the dimensions and material
    properties given. Factors are calculated for each spectrum (i.e. detector
   position) and wavelength point,
    as defined by the input workspace. The sample is divided up into a stack of
   slices, which are then divided
    into annuli (rings). These annuli are further divided to give the full set
   of elements for which a calculation
    will be carried out. Thus the calculation speed depends linearly on the
   total number of bins in the workspace
    and on the number of slices. The dependence on the number of annuli is
   stronger, going as 3n(n + 1).

    Path lengths through the sample are then calculated for the centre-point of
   each element and a numerical
    integration is carried out using these path lengths over the volume
   elements.

    This algorithm assumes that Y (the sample cylinder axis) is up.

    @author Russell Taylor, Tessella Support Services plc
    @date 02/12/2008
*/
class DLLExport CylinderAbsorption : public AbsorptionCorrection {
public:
  /// Default constructor
  CylinderAbsorption();
  /// Algorithm's name
  const std::string name() const override { return "CylinderAbsorption"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates bin-by-bin correction factors for attenuation due to "
           "absorption and single scattering in a 'cylindrical' sample.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"AbsorptionCorrection"};
  }

private:
  void defineProperties() override;
  void retrieveProperties() override;
  std::string sampleXML() override;
  void initialiseCachedDistances() override;
  void getShapeFromSample(const Geometry::IObject &sampleShape,
                          bool updateHeight, bool updateRadius);
  std::map<std::string, std::string> validateInputs() override;
  Kernel::V3D m_cylAxis; ///< The axis orientation of the cylinder
  double m_cylHeight;    ///< The height of the cylindrical sample in m
  double m_cylRadius;    ///< The radius of the cylindrical sample in m
  int m_numSlices;       ///< The number of slices
  int m_numAnnuli;       ///< The number of annuli
  bool m_useSampleShape;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CYLINDERABSORPTION_H_*/
