#ifndef MANTID_ALGORITHMS_SPHERICALABSORPTION_H_
#define MANTID_ALGORITHMS_SPHERICALABSORPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid {

// forward declaration from other Mantid modules
namespace API {
class Sample;
}

namespace Kernel {
class V3D;
}

namespace Geometry {
class IObject;
}

namespace Algorithms {
/** A spherical absorption correction algorithm.

    Common Properties:
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

    @author Vickie Lynch, SNS
    @date 08/16/2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SphericalAbsorption : public API::Algorithm {
public:
  /// (Empty) Constructor
  SphericalAbsorption();
  /// Algorithm's category for identification
  const std::string category() const override {
    return "CorrectionFunctions\\AbsorptionCorrections";
  }
  /// Algorithm's name
  const std::string name() const override { return "SphericalAbsorption"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates bin-by-bin or event correction factors for attenuation "
           "due to absorption and scattering in a 'spherical' sample.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"AbsorptionCorrection"};
  }

protected:
  API::MatrixWorkspace_sptr m_inputWS;     ///< A pointer to the input workspace
  const Geometry::IObject *m_sampleObject; ///< Local cache of sample object.
  Kernel::V3D m_beamDirection;             ///< The direction of the beam.
  std::vector<double> m_L1s,               ///< Cached L1 distances
      m_elementVolumes;                    ///< Cached element volumes
  std::vector<Kernel::V3D> m_elementPositions; ///< Cached element positions
  size_t m_numVolumeElements; ///< The number of volume elements
  double m_sampleVolume;      ///< The total volume of the sample

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void retrieveBaseProperties();
  void constructSample(API::Sample &sample);
  void calculateDistances(const Geometry::IDetector_const_sptr &detector,
                          std::vector<double> &L2s) const;
  inline double doIntegration(const double &lambda,
                              const std::vector<double> &L2s) const;
  inline double doIntegration(const double &lambda_i, const double &lambda_f,
                              const std::vector<double> &L2s) const;

  double m_refAtten;   ///< The attenuation cross-section in 1/m at 1.8A
  double m_scattering; ///< The scattering cross-section in 1/m
  int64_t n_lambda;    ///< The number of points in wavelength, the rest is
  /// interpolated linearly
  int64_t x_step;  ///< The step in bin number between adjacent points
  int64_t m_emode; ///< The energy mode: 0 - elastic, 1 - direct, 2 - indirect
  double m_lambdaFixed; ///< The wavelength corresponding to the fixed energy,
                        /// if provided
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SPHERICALABSORPTION_H_*/
