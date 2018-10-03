// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_ANVREDCORRECTION_H_
#define MANTID_CRYSTAL_ANVREDCORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Crystal {
const double pc[4][19] = {
    {1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
     1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
     1.0000},
    {1.9368, 1.8653, 1.6908, 1.4981, 1.3532, 1.2746, 1.2530, 1.2714, 1.3093,
     1.3559, 1.4019, 1.4434, 1.4794, 1.5088, 1.5317, 1.5489, 1.5608, 1.5677,
     1.5700},
    {0.0145, 0.1596, 0.5175, 0.9237, 1.2436, 1.4308, 1.4944, 1.4635, 1.3770,
     1.2585, 1.1297, 1.0026, 0.8828, 0.7768, 0.6875, 0.6159, 0.5637, 0.5320,
     0.5216},
    {1.1386, 1.0604, 0.8598, 0.6111, 0.3798, 0.1962, 0.0652, -0.0198, -0.0716,
     -0.0993, -0.1176, -0.1153, -0.1125, -0.1073, -0.1016, -0.0962, -0.0922,
     -0.0898, -0.0892}};

const double MAX_WAVELENGTH = 50.0; // max in lamda_weight table

const double STEPS_PER_ANGSTROM = 100; // resolution of lamda table

const int NUM_WAVELENGTHS =
    static_cast<int>(std::ceil(MAX_WAVELENGTH * STEPS_PER_ANGSTROM));

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
class DLLExport AnvredCorrection : public API::Algorithm {
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
  const std::vector<std::string> seeAlso() const override {
    return {"LorentzCorrection"};
  }
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

  void retrieveBaseProperties();
  // void constructSample(API::Sample& sample);
  double getEventWeight(double lamda, double two_theta);
  void BuildLamdaWeights();
  double absor_sphere(double &twoth, double &wl);
  void scale_init(const Geometry::IDetector &det,
                  Geometry::Instrument_const_sptr inst, double &L2,
                  double &depth, double &pathlength, std::string &bankName);
  void scale_exec(std::string &bankName, double &lambda, double &depth,
                  Geometry::Instrument_const_sptr inst, double &pathlength,
                  double &value);

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

#endif /* MANTID_CRYSTAL_ANVREDCORRECTION_H_*/
