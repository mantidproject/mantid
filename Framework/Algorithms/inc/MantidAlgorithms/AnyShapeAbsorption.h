// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AbsorptionCorrection.h"
#include "MantidGeometry/Objects/CSGObject.h"

namespace Mantid {
namespace Algorithms {
/** Calculates an approximation of the attenuation due to absorption and
   scattering in a
    generic sample shape. The sample shape can be defined by, e.g., the
   CreateSampleShape
    algorithm. Note that the name of this algorithm is 'AbsorptionCorrection'.

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
    <LI> Emode  - The energy mode (0=elastic, 1=direct geometry, 2=indirect
   geometry) </LI>
    <LI> Efixed - Value of fixed energy: EI (emode=1) or EF (emode=2) (meV)
   </LI>
    <LI> ElementSize - The side dimension of an integration element cube in mm
   (default: 1). </LI>
    </UL>

    This algorithm uses numerical integration method to calculate attenuation
   factors
    resulting from absorption and single scattering in a sample with the
   material
    properties given. Factors are calculated for each spectrum (i.e. detector
   position)
    and wavelength point, as defined by the input workspace.
    The sample is first bounded by a cuboid, which is divided up into small
   cubes.
    The cubes whose centres lie within the sample make up the set of integration
   elements
    and path lengths through the sample are
    calculated for the centre-point of each element, and a numerical
    integration is carried out using these path lengths over the volume
   elements.

    If the "GaugeVolume" property on the run object is defined, the calculation
   will only use
    points within the defined gauge volume (and also within the sample).

    @author Russell Taylor, Tessella plc
    @date 11/03/2010
*/
class MANTID_ALGORITHMS_DLL AnyShapeAbsorption : public AbsorptionCorrection {
public:
  /// (Empty) Constructor
  AnyShapeAbsorption();
  /// Algorithm's name
  const std::string name() const override { return "AbsorptionCorrection"; }

  const std::vector<std::string> seeAlso() const override {
    return {"SetSampleMaterial",   "CreateSampleShape",     "DefineGaugeVolume",          "CylinderAbsorption",
            "FlatPlateAbsorption", "AnnularRingAbsorption", "CuboidGaugeVolumeAbsorption"};
  }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates an approximation of the attenuation due to absorption "
           "and single scattering in a generic sample shape. The sample shape "
           "can be defined by, e.g., the CreateSampleShape algorithm.\nNote "
           "that if your sample is of cuboid or cylinder geometry, you will "
           "get a more accurate result from the FlatPlateAbsorption or "
           "CylinderAbsorption algorithms respectively.";
  }

  /// Algorithm's version
  int version() const override { return (1); }

private:
  void defineProperties() override;
  void retrieveProperties() override;
  std::string sampleXML() override;
  void initialiseCachedDistances() override;
  /// Create the gauge volume for the correction
  std::shared_ptr<const Geometry::IObject> constructGaugeVolume();

  double m_cubeSide; ///< The length of the side of an element cube in m
};

} // namespace Algorithms
} // namespace Mantid
