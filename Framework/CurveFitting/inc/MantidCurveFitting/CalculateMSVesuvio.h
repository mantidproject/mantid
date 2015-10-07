#ifndef MANTID_CURVEFITTING_CALCULATEMSVESUVIO_H_
#define MANTID_CURVEFITTING_CALCULATEMSVESUVIO_H_
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace CurveFitting {
//-----------------------------------------------------------------------------
// CurveFitting forward declarations
//-----------------------------------------------------------------------------
struct DetectorParams;
struct ResolutionParams;
namespace MSVesuvioHelper {
class RandomNumberGenerator;
struct Simulation;
struct SimulationWithErrors;
}

/**
  Calculates the multiple scattering & total scattering contributions
  for a flat-plate or cylindrical sample.

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory &
  NScD Oak Ridge National Laboratory

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
class DLLExport CalculateMSVesuvio : public API::Algorithm {
private:
  // Holds date on the compton scattering properties of an atom
  struct ComptonNeutronAtom {
    ComptonNeutronAtom() : mass(-1.0), sclength(-1.0), profile(-1.0) {}
    double mass;     // in amu
    double sclength; // 4pi/xsec
    double profile;  // s.d of J(y)
  };
  // Holds data about sample as a whole.
  struct SampleComptonProperties {
    SampleComptonProperties(const int nprops)
        : atoms(nprops), density(-1.0), totalxsec(-1.0), mu(-1.0) {}

    std::vector<ComptonNeutronAtom> atoms;
    double density;   // g/cm^3
    double totalxsec; // total free-scattering cross section
    double mu;        // attenuation factor (1/m)
  };

public:
  CalculateMSVesuvio();
  ~CalculateMSVesuvio();

  /// @copydoc Algorithm::name
  virtual const std::string name() const { return "CalculateMSVesuvio"; }
  /// @copydoc Algorithm::version
  virtual int version() const { return 1; }
  /// @copydoc Algorithm::category
  virtual const std::string category() const { return "ISIS"; }
  /// @copydoc Algorithm::summary
  virtual const std::string summary() const {
    return "Calculates the contributions of multiple scattering "
           "on a flat plate sample for VESUVIO";
  }

private:
  void init();
  void exec();

  void cacheInputs();
  void calculateMS(const size_t wsIndex, API::ISpectrum &totalsc,
                   API::ISpectrum &multsc) const;
  void simulate(const DetectorParams &detpar, const ResolutionParams &respar,
                MSVesuvioHelper::Simulation &simulCounts) const;
  void assignToOutput(const MSVesuvioHelper::SimulationWithErrors &avgCounts,
                      API::ISpectrum &totalsc, API::ISpectrum &multsc) const;
  double calculateCounts(const DetectorParams &detpar,
                         const ResolutionParams &respar,
                         MSVesuvioHelper::Simulation &simulation) const;

  // single-event helpers
  Kernel::V3D generateSrcPos(const double l1) const;
  double generateE0(const double l1, const double t2, double &weight) const;
  double generateTOF(const double gaussTOF, const double dtof,
                     const double dl1) const;
  bool generateScatter(const Kernel::V3D &startPos, const Kernel::V3D &direc,
                       double &weight, Kernel::V3D &scatterPt) const;
  std::pair<double, double> calculateE1Range(const double theta,
                                             const double en0) const;
  double partialDiffXSec(const double en0, const double en1,
                         const double theta) const;
  Kernel::V3D generateDetectorPos(const Kernel::V3D &nominalPos,
                                  const double energy,
                                  const Kernel::V3D &scatterPt,
                                  const Kernel::V3D &direcBeforeSc,
                                  double &scang, double &distToExit) const;
  double generateE1(const double angle, const double e1nom,
                    const double e1res) const;

  // Member Variables
  MSVesuvioHelper::RandomNumberGenerator *m_randgen; // random number generator

  size_t m_acrossIdx, m_upIdx, m_beamIdx; // indices of each direction
  Kernel::V3D m_beamDir;                  // Directional vector for beam
  double m_srcR2;                         // beam penumbra radius (m)
  double m_halfSampleHeight, m_halfSampleWidth, m_halfSampleThick; // (m)
  Geometry::Object const *m_sampleShape;  // sample shape
  SampleComptonProperties *m_sampleProps; // description of sample properties
  double m_detHeight, m_detWidth, m_detThick; // (m)
  double m_tmin, m_tmax, m_delt;              // min, max & dt TOF value
  double m_foilRes;                           // resolution in energy of foil

  size_t m_nscatters; // highest order of scattering to generate
  size_t m_nruns;     // number of runs per spectrum
  size_t m_nevents;   // number of single events per run

  API::Progress *m_progress;
  API::MatrixWorkspace_sptr m_inputWS;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CALCULATEMSVESUVIO_H_ */
