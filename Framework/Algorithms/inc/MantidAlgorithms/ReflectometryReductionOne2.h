#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE2_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE2_H_

#include "MantidAlgorithms/ReflectometryWorkflowBase2.h"
#include "MantidAPI/SpectraDetectorTypes.h"

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
  // Create the output workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr makeIvsLam();
  // Construct the output workspace
  Mantid::API::MatrixWorkspace_sptr
  constructIvsLamWS(API::MatrixWorkspace_sptr detectorWS);
  // Whether summation should be done in Q or the default lambda
  bool summingInQ();
  // Get angle details for a specific detector
  void getDetectorDetails(const size_t spIdx,
                          const API::SpectrumInfo &spectrumInfo, double &theta,
                          double &bTwoTheta);
  // Get projected coordinates onto thetaR
  void getProjectedLambdaRange(const double lambda, const double theta,
                               const double bLambda, const double bTwoTheta,
                               double &lambdaTop, double &lambdaBot);
  // Check whether two spectrum maps match
  void verifySpectrumMaps(API::MatrixWorkspace_const_sptr ws1,
                          API::MatrixWorkspace_const_sptr ws2,
                          const bool severe);
  std::string createProcessingCommandsFromDetectorWS(
      API::MatrixWorkspace_const_sptr originWS,
      API::MatrixWorkspace_const_sptr hostWS);
  int mapSpectrumIndexToWorkspace(const spec2index_map &map,
                                  const size_t mapIdx,
                                  API::MatrixWorkspace_const_sptr destWS);

  // Find and cache constants
  void initRun();
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
  size_t centreDetectorIdx() { return m_centreDetectorIdx; };

  API::MatrixWorkspace_sptr m_runWS;
  const API::SpectrumInfo *m_spectrumInfo;
  bool m_convertUnits;             // convert the input workspace to lambda
  bool m_normalise;                // normalise by monitors etc.
  bool m_sum;                      // whether to do summation
  double m_thetaMin;               // min angle in detectors of interest
  double m_thetaMax;               // max angle in detectors of interest
  double m_theta0;                 // horizon angle
  double m_thetaR;                 // reference angle for summation in Q
  double m_lambdaMin;              // min wavelength in area of interest
  double m_lambdaMax;              // max wavelength in area of interest
  std::vector<size_t> m_detectors; // workspace indices of detectors of interest
  size_t m_centreDetectorIdx;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE2_H_ */
