// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"

namespace Mantid {

namespace API {
class Sample;
}
namespace Geometry {
class IDetector;
class IObject;
} // namespace Geometry

namespace Algorithms {
/** AbsorptionCorrectionPaalmanPings : TODO: DESCRIPTION
 */
/** A base class for absorption correction algorithms.

    Common Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as
   the input one. </LI>
    <LI> NumberOfWavelengthPoints - The number of wavelength points for which
   numerical integral is calculated (default: all points). </LI>
    </UL>

    This class, which must be overridden to provide the specific sample geometry
   and integration
    elements, uses a numerical integration method to calculate attenuation
   factors resulting
    from absorption and single scattering in a sample. Factors are calculated
   for each spectrum
    (i.e. detector position) and wavelength point, as defined by the input
   workspace.
    Path lengths through the sample are then calculated for the centre-point of
   each element
    and a numerical integration is carried out using these path lengths over the
   volume elements.

    This algorithm assumes that the beam comes along the Z axis, that Y is up
    and that the sample is at the origin.

    @author Russell Taylor, Tessella plc
    @date 04/02/2010
*/
class MANTID_ALGORITHMS_DLL AbsorptionCorrectionPaalmanPings
    : public API::Algorithm {
public:
  /// (Empty) Constructor
  AbsorptionCorrectionPaalmanPings();
  /// Algorithm's name
  const std::string name() const override {
    return "AbsorptionCorrectionPaalmanPings";
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "CorrectionFunctions\\AbsorptionCorrections";
  }
  /// Algorithm's summary
  const std::string summary() const override {
    return "Calculates an approximation of the attenuation due to absorption "
           "and single scattering in a generic sample shape. The sample shape "
           "can be defined by the CreateSampleShape algorithm.";
  }
  /// Algorithm's version
  int version() const override { return (1); }

private:
  /// Initialisation code
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  /// Execution code
  void exec() override;

  void retrieveBaseProperties();
  void constructSample(API::Sample &sample);
  void calculateDistances(const Geometry::IDetector &detector,
                          std::vector<double> &sample_L2s,
                          std::vector<double> &sample_container_L2s,
                          std::vector<double> &container_L2s,
                          std::vector<double> &container_sample_L2s) const;
  inline double
  doIntegration(const double linearCoefAbs, const double linearCoefTotScatt,
                const std::vector<double> &elementVolumes,
                const std::vector<double> &L1s, const std::vector<double> &L2s,
                const size_t startIndex, const size_t endIndex) const;
  void defineProperties();
  void retrieveProperties();
  void initialiseCachedDistances();

  API::MatrixWorkspace_sptr m_inputWS;     ///< A pointer to the input workspace
  const Geometry::IObject *m_sampleObject; ///< Local cache of sample object.
  const Geometry::IObject
      *m_containerObject;      ///< Local cache of container object.
  Kernel::V3D m_beamDirection; ///< The direction of the beam.
  std::vector<double> m_L1s,   ///< Cached L1 distances
      m_elementVolumes;        ///< Cached element volumes
  std::vector<Kernel::V3D> m_elementPositions; ///< Cached element positions
  size_t m_numVolumeElements;         ///< The number of volume elements
  double m_sampleVolume;              ///< The total volume of the sample
  std::vector<double> m_containerL1s, ///< Cached L1 distances
      m_containerElementVolumes;      ///< Cached element volumes
  std::vector<Kernel::V3D>
      m_containerElementPositions;     ///< Cached element positions
  size_t m_containerNumVolumeElements; ///< The number of volume elements
  double m_containerVolume;            ///< The total volume of the sample
  Kernel::Material m_material;
  Kernel::Material m_containerMaterial;
  double m_linearCoefTotScatt; ///< The total scattering cross-section in 1/m
  double m_containerLinearCoefTotScatt; ///< The total scattering cross-section
                                        ///< in 1/m
  int64_t m_num_lambda; ///< The number of points in wavelength, the rest is
  /// interpolated linearly
  int64_t m_xStep; ///< The step in bin number between adjacent points

  /// Create the gague volume for the correction
  std::shared_ptr<const Geometry::IObject> constructGaugeVolume();
  double m_cubeSide; ///< The length of the side of an element cube in m
};

} // namespace Algorithms
} // namespace Mantid
