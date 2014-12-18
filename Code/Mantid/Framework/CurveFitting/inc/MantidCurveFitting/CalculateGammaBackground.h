#ifndef MANTID_CURVEFITTING_CALCULATEGAMMABACKGROUND_H_
#define MANTID_CURVEFITTING_CALCULATEGAMMABACKGROUND_H_

#include "MantidAPI/Algorithm.h"

#include <boost/unordered_map.hpp>

namespace Mantid {
namespace CurveFitting {
//---------------------------------------------------------------------------
// Forward declarations
//---------------------------------------------------------------------------
struct DetectorParams;
struct ResolutionParams;

/**

Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport CalculateGammaBackground : public API::Algorithm {
public:
  CalculateGammaBackground();
  ~CalculateGammaBackground();

  const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculates the background due to gamma rays produced when neutrons "
           "are absorbed by shielding.";
  }

  int version() const;
  const std::string category() const;

private:
  void init();
  void exec();

  /// Hold information about a single foil
  struct FoilInfo {
    double thetaMin;
    double thetaMax;
    double lorentzWidth;
    double gaussWidth;
  };

  /// Avoid nested try catch in openmp loop by returning boolean if spectrum was
  /// corrceted
  bool calculateBackground(size_t inputIndex, size_t outputIndex);
  /// Calculate & correct the given index of the input workspace
  void applyCorrection(const size_t inputIndex, const size_t outputIndex);
  /// Compute the expected spectrum from a given detector
  void calculateSpectrumFromDetector(const size_t inputIndex,
                                     const size_t outputIndex);
  /// Compute the expected background from the foils
  void calculateBackgroundFromFoils(const size_t inputIndex,
                                    const size_t outputIndex);
  /// Compute expected background from single foil for spectrum at wsIndex
  void calculateBackgroundSingleFoil(std::vector<double> &ctfoil,
                                     const size_t wsIndex,
                                     const FoilInfo &foilInfo,
                                     const Kernel::V3D &detPos,
                                     const DetectorParams &detPar,
                                     const ResolutionParams &detRes);
  /// Compute a TOF spectrum for the given inputs & spectrum
  void calculateTofSpectrum(std::vector<double> &result,
                            std::vector<double> &tmpWork, const size_t wsIndex,
                            const DetectorParams &detpar,
                            const ResolutionParams &respar);

  /// Check and store appropriate input data
  void retrieveInputs();
  /// Create the output workspaces
  void createOutputWorkspaces();
  /// Compute & store the parameters that are fixed during the correction
  void cacheInstrumentGeometry();
  /// Compute the theta range for a given foil
  std::pair<double, double>
  calculateThetaRange(const Geometry::IComponent_const_sptr &foilComp,
                      const double radius, const unsigned int horizDir) const;

  /// Input TOF data
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// Sorted indices to correct
  boost::unordered_map<size_t, size_t> m_indices;
  /// Function that defines the mass profile
  std::string m_profileFunction;
  /// The number of peaks in spectrum
  size_t m_npeaks;
  /// List of spectra numbers whose background sum is to be reversed
  std::set<specid_t> m_reversed;

  /// Sample position
  Kernel::V3D m_samplePos;
  /// Source to sample distance
  double m_l1;
  /// Radius of (imaginary) circle that foils sit on
  double m_foilRadius;
  /// Minimum in up dir to start integration over foil volume
  double m_foilUpMin;
  /// Minimum in up dir to stop integration over foil volume
  double m_foilUpMax;

  /// Description of foils in the position 0
  std::vector<FoilInfo> m_foils0;
  /// Description of foils in the position 0
  std::vector<FoilInfo> m_foils1;
  /// Stores the value of the calculated background
  API::MatrixWorkspace_sptr m_backgroundWS;
  /// Stores the corrected data
  API::MatrixWorkspace_sptr m_correctedWS;

  /// Pointer to progress reporting
  API::Progress *m_progress;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CALCULATEGAMMABACKGROUND_H_ */
