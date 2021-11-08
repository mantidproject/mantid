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
#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Crystal {

// fit to ln(1/A*) = pc[0][i] + pc[1][i]*muR + pc[2][i]*muR^2 + pc[3][i]*muR^3 + pc[4][i]*muR^4 + pc[5][i]*muR^5
// pc[6][i]*muR^6 after Dwiggins, jr., acta cryst. A, 31, 146 (1975) - (but for sphere instread of cylinder)
const double pc[7][19] = {{-1.2804e-06, -8.8188e-07, -2.8020e-06, -1.4515e-05, -3.4850e-05, -5.1271e-05, -6.0559e-05,
                           -6.0009e-05, -5.2599e-05, -3.9402e-05, -2.3666e-05, -8.3626e-06, 7.9032e-06, 2.3195e-05,
                           3.5901e-05, 4.6653e-05, 5.4746e-05, 5.7827e-05, 5.9823e-05},
                          {1.6671e-04, 1.6744e-04, 2.4817e-04, 5.4908e-04, 9.9680e-04, 1.3171e-03, 1.4560e-03,
                           1.3684e-03, 1.1337e-03, 7.7590e-04, 3.7077e-04, -2.0375e-05, -4.1922e-04, -7.9296e-04,
                           -1.1040e-03, -1.3625e-03, -1.5562e-03, -1.6364e-03, -1.6821e-03},
                          {-3.1578e-03, -3.2939e-03, -4.3863e-03, -7.1551e-03, -1.0572e-02, -1.2470e-02, -1.2636e-02,
                           -1.0860e-02, -7.9476e-03, -4.0954e-03, 2.6006e-05, 3.9546e-03, 7.7966e-03, 1.1362e-02,
                           1.4335e-02, 1.6766e-02, 1.8578e-02, 1.9395e-02, 1.9802e-02},
                          {1.8010e-02, 1.9196e-02, 2.4484e-02, 3.3778e-02, 4.2198e-02, 4.2703e-02, 3.5952e-02,
                           2.1928e-02, 4.4292e-03, -1.5913e-02, -3.6388e-02, -5.5511e-02, -7.3449e-02, -8.9814e-02,
                           -1.0349e-01, -1.1452e-01, -1.2272e-01, -1.2672e-01, -1.2849e-01},
                          {6.3162e-02, 6.4550e-02, 6.5798e-02, 6.8961e-02, 7.9435e-02, 1.0484e-01, 1.4070e-01,
                           1.8598e-01, 2.3309e-01, 2.8188e-01, 3.2779e-01, 3.6930e-01, 4.0657e-01, 4.3975e-01,
                           4.6742e-01, 4.8958e-01, 5.0608e-01, 5.1476e-01, 5.1823e-01},
                          {-1.4952e+00, -1.4956e+00, -1.4931e+00, -1.4907e+00, -1.4896e+00, -1.4931e+00, -1.4983e+00,
                           -1.5057e+00, -1.5110e+00, -1.5159e+00, -1.5184e+00, -1.5190e+00, -1.5181e+00, -1.5166e+00,
                           -1.5147e+00, -1.5127e+00, -1.5111e+00, -1.5094e+00, -1.5089e+00},
                          {0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00,
                           0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00,
                           0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00}};

const double MAX_WAVELENGTH = 50.0; // max in lamda_weight table

const double STEPS_PER_ANGSTROM = 100; // resolution of lamda table

const int NUM_WAVELENGTHS = static_cast<int>(std::ceil(MAX_WAVELENGTH * STEPS_PER_ANGSTROM));

const double radtodeg_half = 180.0 / M_PI / 2.;
/** Calculates anvred correction factors for attenuation due to absorption and
   scattering in a spherical sample.

    Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as
   the input one. </LI>
    <LI> PreserveEvents - Keep the output workspace as an EventWorkspace, if the
   input has events.
    <LI> OnlySphericalAbsorption - All corrections done if false (default). If
   true, only the spherical absorption correction.
    <LI> LinearScatteringCoef - Linear scattering coefficient in 1/cm
    <LI> LinearAbsorptionCoef - Linear absorption coefficient at 1.8 Angstroms
   in 1/cm.
    <LI> Radius - Radius of the sample in centimeters.
    </UL>

    @author Vickie Lynch, Dennis Mikkelson SNS
    @date 06/14/2011
*/
class MANTID_CRYSTAL_DLL AnvredCorrection : public API::Algorithm {
public:
  /// (Empty) Constructor
  AnvredCorrection();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "AnvredCorrection"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates anvred correction factors for attenuation due to "
           "absorption and scattering in a spherical sample";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LorentzCorrection"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Crystal\\Corrections;CorrectionFunctions\\AbsorptionCorrections";
  }

protected:
  /** A virtual function in which additional properties of an algorithm should
   * be declared.
   *  Called by init().
   */
  virtual void defineProperties() { /*Empty in base class*/
  }
  /// A virtual function in which additional properties should be retrieved into
  /// member variables.
  virtual void retrieveProperties() { /*Empty in base class*/
  }

  API::MatrixWorkspace_sptr m_inputWS; ///< A pointer to the input workspace
  /// Shared pointer to the event workspace
  DataObjects::EventWorkspace_sptr eventW;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Event execution code
  void execEvent();
  /// Algorithm cleanup
  void cleanup();
  /// validate inputs
  std::map<std::string, std::string> validateInputs() override;

  void retrieveBaseProperties();
  // void constructSample(API::Sample& sample);
  double getEventWeight(double lamda, double two_theta);
  void BuildLamdaWeights();
  double absor_sphere(double &twoth, double &wl);
  void scale_init(const Geometry::IDetector &det, const Geometry::Instrument_const_sptr &inst, double &L2,
                  double &depth, double &pathlength, std::string &bankName);
  void scale_exec(std::string &bankName, double &lambda, double &depth, const Geometry::Instrument_const_sptr &inst,
                  double &pathlength, double &value);

  double m_smu;                       ///< linear scattering coefficient in 1/cm
  double m_amu;                       ///< linear absoprtion coefficient in 1/cm
  double m_radius;                    ///< sample radius in cm
  double m_power_th;                  ///< Power of lamda in BuildLamdaWeights
  std::vector<double> m_lamda_weight; ///< lmabda weights
  bool m_onlySphericalAbsorption;
  bool m_returnTransmissionOnly;
  bool m_useScaleFactors;
};

} // namespace Crystal
} // namespace Mantid
