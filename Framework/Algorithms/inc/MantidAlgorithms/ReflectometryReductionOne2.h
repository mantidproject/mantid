#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE2_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE2_H_

#include "MantidAlgorithms/ReflectometryWorkflowBase2.h"

namespace Mantid {
// Forward declaration
namespace API {
class SpectrumInfo;
}

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
  // Create a direct beam workspace from input workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr
  makeDirectBeamWS(Mantid::API::MatrixWorkspace_sptr inputWS);
  // Performs direct beam correction
  Mantid::API::MatrixWorkspace_sptr
  directBeamCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // Performs transmission or algorithm correction
  Mantid::API::MatrixWorkspace_sptr
  transOrAlgCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // Performs transmission corrections
  Mantid::API::MatrixWorkspace_sptr
  transmissionCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // Performs transmission corrections using alternative correction algorithms
  Mantid::API::MatrixWorkspace_sptr
  algorithmicCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // Performs monitor corrections
  Mantid::API::MatrixWorkspace_sptr
  monitorCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // convert to momentum transfer
  Mantid::API::MatrixWorkspace_sptr
  convertToQ(Mantid::API::MatrixWorkspace_sptr inputWS);
  // Create the output workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr
  makeIvsLam(const bool convert, const bool normalise, const bool sum);
  // Construct the output workspace
  Mantid::API::MatrixWorkspace_sptr
  constructIvsLamWS(API::MatrixWorkspace_sptr detectorWS);
  // Whether summation should be done in Q or the default lambda
  bool sumInQ();
  // Get angle details for a specific detector
  void getDetectorDetails(const size_t spIdx,
                          const API::SpectrumInfo &spectrumInfo, double &theta,
                          double &bTwoTheta);
  // Get projected coordinates onto thetaR
  void getProjectedLambdaRange(const double lambda, const double theta,
                               const double bLambda, const double bTwoTheta,
                               double &lambdaTop, double &lambdaBot);
  // Find and cache constants required for summation in Q
  void findConstantsForSumInQ();
  void findLambdaMinMax();
  void findDetectorsOfInterest();
  void findThetaMinMax();
  void findThetaR();
  void findTheta0();
  // Accessors for theta and lambda values
  double lambdaMin() { return m_lambdaMin; }
  double lambdaMax() { return m_lambdaMax; }
  double thetaMin() { return m_thetaMin; }
  double thetaMax() { return m_thetaMax; }
  double theta0() { return m_theta0; }
  double thetaR() { return m_thetaR; }
  int centreDetectorIdx() { return m_centreDetectorIdx; };

  API::MatrixWorkspace_sptr m_runWS;
  const API::SpectrumInfo *m_spectrumInfo;
  double m_thetaMin;
  double m_thetaMax;
  double m_theta0;
  double m_thetaR;
  double m_lambdaMin;
  double m_lambdaMax;
  std::vector<size_t> m_detectors;
  int m_centreDetectorIdx;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE2_H_ */
