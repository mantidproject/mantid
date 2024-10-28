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

// fit to ln(1/A*) = sum_{icoef=0}^{N=7} pc[7-icoef][ith]*(muR)^icoef
// A*(muR=0) = 1 so pc[7][ith] = 0 (fixed not fitted)
// Fit performed in MATLAB using least-squares minimisation
// after Dwiggins, jr., acta cryst. A, 31, 146 (1975) - (but for sphere instread of cylinder)
const double pc[8][19] = {{-6.4910e-07, -6.8938e-07, -7.8149e-07, 8.1682e-08, 1.8008e-06, 3.3916e-06, 4.5095e-06,
                           4.7970e-06, 4.4934e-06, 3.6700e-06, 2.5881e-06, 1.5007e-06, 3.7669e-07, -7.9487e-07,
                           -1.7935e-06, -2.5563e-06, -3.1113e-06, -3.3993e-06, -3.5091e-06},
                          {1.0839e-05, 1.1582e-05, 1.1004e-05, -2.2848e-05, -8.1974e-05, -1.3268e-04, -1.6486e-04,
                           -1.6839e-04, -1.5242e-04, -1.1949e-04, -7.8682e-05, -3.7973e-05, 2.9117e-06, 4.4823e-05,
                           8.0464e-05, 1.0769e-04, 1.2753e-04, 1.3800e-04, 1.4190e-04},
                          {8.7140e-05, 9.0870e-05, 1.6706e-04, 6.9008e-04, 1.4781e-03, 2.0818e-03, 2.3973e-03,
                           2.3209e-03, 1.9935e-03, 1.4508e-03, 8.1903e-04, 1.9608e-04, -4.1128e-04, -1.0205e-03,
                           -1.5374e-03, -1.9329e-03, -2.2212e-03, -2.3760e-03, -2.4324e-03},
                          {-2.9549e-03, -3.1360e-03, -4.2431e-03, -8.1103e-03, -1.2989e-02, -1.6012e-02, -1.6815e-02,
                           -1.4962e-02, -1.1563e-02, -6.8581e-03, -1.7302e-03, 3.2400e-03, 7.9409e-03, 1.2528e-02,
                           1.6414e-02, 1.9394e-02, 2.1568e-02, 2.2758e-02, 2.3182e-02},
                          {1.7934e-02, 1.9304e-02, 2.4706e-02, 3.6759e-02, 4.8351e-02, 5.1049e-02, 4.5368e-02,
                           3.0864e-02, 1.2086e-02, -1.0254e-02, -3.2992e-02, -5.4495e-02, -7.4205e-02, -9.2818e-02,
                           -1.0855e-01, -1.2068e-01, -1.2954e-01, -1.3451e-01, -1.3623e-01},
                          {6.2799e-02, 6.3892e-02, 6.4943e-02, 6.4881e-02, 7.2169e-02, 9.5669e-02, 1.3082e-01,
                           1.7694e-01, 2.2559e-01, 2.7655e-01, 3.2483e-01, 3.6888e-01, 4.0783e-01, 4.4330e-01,
                           4.7317e-01, 4.9631e-01, 5.1334e-01, 5.2318e-01, 5.2651e-01},
                          {-1.4949e+00, -1.4952e+00, -1.4925e+00, -1.4889e+00, -1.4867e+00, -1.4897e+00, -1.4948e+00,
                           -1.5025e+00, -1.5084e+00, -1.5142e+00, -1.5176e+00, -1.5191e+00, -1.5187e+00, -1.5180e+00,
                           -1.5169e+00, -1.5153e+00, -1.5138e+00, -1.5125e+00, -1.5120e+00},
                          {0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00,
                           0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00,
                           0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00, 0.0000e+00}};

const double MAX_WAVELENGTH = 50.0; // max in lamda_weight table

const double STEPS_PER_ANGSTROM = 100; // resolution of lamda table

const int NUM_WAVELENGTHS = static_cast<int>(std::ceil(MAX_WAVELENGTH * STEPS_PER_ANGSTROM));

const double radtodeg = 180.0 / M_PI;
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
class MANTID_CRYSTAL_DLL AnvredCorrection final : public API::Algorithm {
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
  static double calc_Astar(const double theta, const double mur);

protected:
  /** A virtual function in which additional properties of an algorithm should
   * be declared.
   *  Called by init().
   */
  virtual void defineProperties() { /*Empty in base class*/ }
  /// A virtual function in which additional properties should be retrieved into
  /// member variables.
  virtual void retrieveProperties() { /*Empty in base class*/ }

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
  double getEventWeight(const double lamda, const double two_theta, bool &muRTooLarge);
  void BuildLamdaWeights();
  double absor_sphere(const double twoth, const double wl, bool &muRTooLarge);
  void scale_init(const Geometry::Instrument_const_sptr &inst, const double L2, const double depth, double &pathlength,
                  const std::string &bankName);
  double scale_exec(std::string &bankName, const double lambda, const double depth,
                    const Geometry::Instrument_const_sptr &inst, const double pathlength, double eventWeight);

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
