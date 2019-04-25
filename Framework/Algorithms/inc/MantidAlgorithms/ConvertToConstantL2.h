// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_ConvertToConstantL2_H_
#define MANTID_ALGORITHMS_ConvertToConstantL2_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Algorithms {
/** Convert workspace to have a constant L2

 Required Properties:
 <UL>
 <LI> InputWorkspace - The name of the Workspace to take as input </LI>
 <LI> OutputWorkspace - The name of the workspace in which to store the result
 </LI>
 </UL>


 @author Ricardo Ferraz Leal
 @date 30/01/2013
 */
class DLLExport ConvertToConstantL2 : public API::Algorithm {
public:
  /// Default constructor
  ConvertToConstantL2();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "ConvertToConstantL2"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Used to convert flight paths to have a constant l2 in 2D shaped "
           "detectors.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CorrectTOFAxis"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Inelastic\\Corrections;CorrectionFunctions\\InstrumentCorrections";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  void initWorkspaces();
  void getWavelengthFromRun();
  void getL2FromInstrument();
  double calculateTOF(double);

  /// The user selected (input) workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// The output workspace, maybe the same as the input one
  API::MatrixWorkspace_sptr m_outputWS;

  Geometry::Instrument_const_sptr m_instrument;

  double m_l2;
  double m_wavelength;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_ConvertToConstantL2_H_*/
