#ifndef MANTID_CURVEFITTING_VESUVIOCALCULATEMS_H_
#define MANTID_CURVEFITTING_VESUVIOCALCULATEMS_H_
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidCurveFitting/MSVesuvioHelpers.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/V3D.h"

namespace Mantid {

namespace API {
class ISpectrum;
}

namespace Geometry {
class IObject;
}

namespace CurveFitting {
namespace Functions {
struct ResolutionParams;
}

namespace Algorithms {
struct DetectorParams;

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
class DLLExport VesuvioCalculateMS : public API::Algorithm {
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
  VesuvioCalculateMS();
  /// @copydoc Algorithm::name
  const std::string name() const override { return "VesuvioCalculateMS"; }
  /// @copydoc Algorithm::version
  int version() const override { return 1; }
  /// @copydoc Algorithm::category
  const std::string category() const override {
    return "CorrectionFunctions\\SpecialCorrections";
  }
  /// @copydoc Algorithm::summary
  const std::string summary() const override {
    return "Calculates the contributions of multiple scattering "
           "on a flat plate sample for VESUVIO";
  }

  const std::vector<std::string> seeAlso() const override {
    return {"MayersSampleCorrection", "MonteCarloAbsorption",
            "MultipleScatteringCylinderAbsorption"};
  }

private:
  void init() override;
  void exec() override;

  void cacheInputs();
  void calculateMS(const size_t wsIndex, API::ISpectrum &totalsc,
                   API::ISpectrum &multsc) const;
  void simulate(const DetectorParams &detpar,
                const Functions::ResolutionParams &respar,
                MSVesuvioHelper::Simulation &simulCounts) const;
  void assignToOutput(const MSVesuvioHelper::SimulationWithErrors &avgCounts,
                      API::ISpectrum &totalsc, API::ISpectrum &multsc) const;
  double calculateCounts(const DetectorParams &detpar,
                         const Functions::ResolutionParams &respar,
                         MSVesuvioHelper::Simulation &simulation) const;

  // single-event helpers
  Kernel::V3D generateSrcPos(const double l1) const;
  double generateE0(const double l1, const double t2, double &weight) const;
  double generateTOF(const double en0, const double dtof,
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
  std::unique_ptr<CurveFitting::MSVesuvioHelper::RandomVariateGenerator>
      m_randgen; // random number generator

  size_t m_acrossIdx, m_upIdx, m_beamIdx; // indices of each direction
  Kernel::V3D m_beamDir;                  // Directional vector for beam
  double m_srcR2;                         // beam penumbra radius (m)
  double m_halfSampleHeight, m_halfSampleWidth, m_halfSampleThick; // (m)
  Geometry::IObject const *m_sampleShape; // sample shape
  std::unique_ptr<SampleComptonProperties>
      m_sampleProps; // description of sample properties
  double m_detHeight, m_detWidth, m_detThick; // (m)
  double m_tmin, m_tmax, m_delt;              // min, max & dt TOF value
  double m_foilRes;                           // resolution in energy of foil

  size_t m_nscatters; // highest order of scattering to generate
  size_t m_nruns;     // number of runs per spectrum
  size_t m_nevents;   // number of single events per run

  std::unique_ptr<API::Progress> m_progress;
  API::MatrixWorkspace_sptr m_inputWS;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_VESUVIOCALCULATEMS_H_ */
