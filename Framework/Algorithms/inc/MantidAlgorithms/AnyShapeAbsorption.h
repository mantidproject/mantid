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

    Unlike the other algorithms in this family, this one integrates over the sample shape attached to
    the workspace rather than one built from its own properties, so it has to care which frame that
    shape is in. A gauge volume, a beam and the detectors are all described in the lab frame, while
    the sample shape is in the lab frame only if something has already rotated it there. See
    outstandingSampleRotation.

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
    return {
        "SetSampleMaterial",   "CreateSampleShape",     "DefineGaugeVolume",           "CylinderAbsorption",
        "FlatPlateAbsorption", "AnnularRingAbsorption", "CuboidGaugeVolumeAbsorption", "WeightedGaugeVolumeAbsorption"};
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

protected:
  void defineProperties() override;
  void retrieveProperties() override;
  std::string sampleXML() override;
  void initialiseCachedDistances() override;
  void calculateDistances(const Geometry::IDetector &detector, std::vector<double> &L2s) const override;
  /// Create the gague volume for the correction
  std::shared_ptr<const Geometry::IObject> constructGaugeVolume() const;

  double m_cubeSide; ///< The length of the side of an element cube in m

private:
  /// How much of the workspace's goniometer rotation the sample shape has not had applied to it
  Kernel::Matrix<double> outstandingSampleRotation() const;
  /// Map a lab frame point into the sample shape's own frame
  Kernel::V3D toShapeFrame(const Kernel::V3D &labFramePoint) const;

  /// Lab frame to sample shape frame: the inverse of the outstanding rotation, held inverted because
  /// that is the direction every use needs, and with a flag so that no rotation costs nothing.
  Kernel::Matrix<double> m_labToShapeFrame{3, 3, true};
  bool m_shapeIsInLabFrame{true};
};

} // namespace Algorithms
} // namespace Mantid
