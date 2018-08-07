#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE2_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE2_H_

#include "MantidAlgorithms/ReflectometryWorkflowBase2.h"

namespace Mantid {
// Forward declaration
namespace API {
class SpectrumInfo;
}
namespace Geometry {
class ReferenceFrame;
}
namespace HistogramData {
class HistogramX;
class HistogramY;
class HistogramE;
} // namespace HistogramData
namespace Algorithms {

/** ReflectometryReductionOne2 : Reflectometry reduction of a single input TOF
 workspace to an IvsQ workspace. Version 2 of the algorithm.

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
class DLLExport ReflectometryReductionOne2 : public ReflectometryWorkflowBase2 {
public:
  /// Algorithm's name for identification
  const std::string name() const override {
    return "ReflectometryReductionOne";
  };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Reduces a single TOF/Lambda reflectometry run into a mod Q vs I/I0 "
           "workspace. Performs monitor normalization and transmission "
           "corrections.";
  }
  /// Algorithm's version for identification.
  int version() const override { return 2; };
  const std::vector<std::string> seeAlso() const override {
    return {"ReflectometryReductionOneAuto"};
  }
  /// Algorithm's category for identification.
  const std::string category() const override { return "Reflectometry"; };

private:
  /** Overridden Algorithm methods **/

  // Initialize the algorithm
  void init() override;
  // Execute the algorithm
  void exec() override;
  // Validate inputs
  std::map<std::string, std::string> validateInputs() override;
  // Set default names for output workspaces
  void setDefaultOutputWorkspaceNames();
  // Create a direct beam workspace from input workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr
  makeDirectBeamWS(Mantid::API::MatrixWorkspace_sptr inputWS);
  // Performs direct beam correction
  Mantid::API::MatrixWorkspace_sptr
  directBeamCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // Performs transmission or algorithm correction
  Mantid::API::MatrixWorkspace_sptr
  transOrAlgCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS,
                       const bool detectorWSReduced);
  // Performs transmission corrections
  Mantid::API::MatrixWorkspace_sptr
  transmissionCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS,
                         const bool detectorWSReduced);
  // Performs transmission corrections using alternative correction algorithms
  Mantid::API::MatrixWorkspace_sptr
  algorithmicCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // Performs monitor corrections
  Mantid::API::MatrixWorkspace_sptr
  monitorCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // convert to momentum transfer
  Mantid::API::MatrixWorkspace_sptr
  convertToQ(Mantid::API::MatrixWorkspace_sptr inputWS);
  // Get the twoTheta width of a given detector
  double getDetectorTwoThetaRange(const size_t spectrumIdx);
  // Utility function to create name for diagnostic workspaces
  std::string createDebugWorkspaceName(const std::string &inputName);
  // Utility function to output a diagnostic workspace to the ADS
  void outputDebugWorkspace(API::MatrixWorkspace_sptr ws,
                            const std::string &wsName,
                            const std::string &wsSuffix, const bool debug,
                            int &step);
  // Create the output workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr makeIvsLam();
  // Do the reduction by summation in Q
  Mantid::API::MatrixWorkspace_sptr
  sumInQ(API::MatrixWorkspace_sptr detectorWS);
  // Do the summation in Q for a single input value
  void sumInQProcessValue(const int inputIdx, const double twoTheta,
                          const double bTwoTheta,
                          const HistogramData::HistogramX &inputX,
                          const HistogramData::HistogramY &inputY,
                          const HistogramData::HistogramE &inputE,
                          const std::vector<size_t> &detectors,
                          const size_t outSpecIdx,
                          API::MatrixWorkspace_sptr IvsLam,
                          std::vector<double> &outputE);
  // Share counts to a projected value for summation in Q
  void sumInQShareCounts(const double inputCounts, const double inputErr,
                         const double bLambda, const double lambdaMin,
                         const double lambdaMax, const size_t outSpecIdx,
                         API::MatrixWorkspace_sptr IvsLam,
                         std::vector<double> &outputE);
  void findWavelengthMinMax(API::MatrixWorkspace_sptr inputWS);
  // Construct the output workspace
  void findIvsLamRange(API::MatrixWorkspace_sptr detectorWS,
                       const std::vector<size_t> &detectors,
                       const double lambdaMin, const double lambdaMax,
                       double &projectedMin, double &projectedMax);
  // Construct the output workspace
  Mantid::API::MatrixWorkspace_sptr
  constructIvsLamWS(API::MatrixWorkspace_sptr detectorWS);
  // Whether summation should be done in Q or the default lambda
  bool summingInQ();
  // Get projected coordinates onto twoThetaR
  void getProjectedLambdaRange(const double lambda, const double twoTheta,
                               const double bLambda, const double bTwoTheta,
                               const std::vector<size_t> &detectors,
                               double &lambdaTop, double &lambdaBot,
                               const bool outerCorners = true);
  // Check whether two spectrum maps match
  void verifySpectrumMaps(API::MatrixWorkspace_const_sptr ws1,
                          API::MatrixWorkspace_const_sptr ws2,
                          const bool severe);

  // Find and cache constants
  void findDetectorGroups();
  void findTheta0();
  // Accessors for detectors and theta and lambda values
  const std::vector<std::vector<size_t>> &detectorGroups() const {
    return m_detectorGroups;
  };
  double theta0() { return m_theta0; }
  double twoThetaR(const std::vector<size_t> &detectors);
  size_t twoThetaRDetectorIdx(const std::vector<size_t> &detectors);
  double wavelengthMin() { return m_wavelengthMin; };
  double wavelengthMax() { return m_wavelengthMax; };
  size_t findIvsLamRangeMinDetector(const std::vector<size_t> &detectors);
  size_t findIvsLamRangeMaxDetector(const std::vector<size_t> &detectors);
  double findIvsLamRangeMin(Mantid::API::MatrixWorkspace_sptr detectorWS,
                            const std::vector<size_t> &detectors,
                            const double lambda);
  double findIvsLamRangeMax(Mantid::API::MatrixWorkspace_sptr detectorWS,
                            const std::vector<size_t> &detectors,
                            const double lambda);

  API::MatrixWorkspace_sptr m_runWS;
  const API::SpectrumInfo *m_spectrumInfo;
  boost::shared_ptr<const Mantid::Geometry::ReferenceFrame> m_refFrame;
  bool m_convertUnits;          // convert the input workspace to lambda
  bool m_normaliseMonitors;     // normalise by monitors and direct beam
  bool m_normaliseTransmission; // transmission or algorithmic correction
  bool m_sum;                   // whether to do summation
  double m_theta0;              // horizon angle
  // groups of spectrum indices of the detectors of interest
  std::vector<std::vector<size_t>> m_detectorGroups;
  // Store the min/max wavelength we're interested in. These will be the
  // input Wavelength min/max if summing in lambda, or the projected
  // versions of these if summing in Q
  double m_wavelengthMin;
  double m_wavelengthMax;
  // True if partial bins should be included in the summation in Q
  bool m_partialBins;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE2_H_ */
