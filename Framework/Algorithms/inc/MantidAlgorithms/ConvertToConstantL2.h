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

 Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
  double getRunProperty(std::string);
  double getInstrumentProperty(std::string);
  double calculateTOF(double);

  /// The user selected (input) workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// The output workspace, maybe the same as the input one
  API::MatrixWorkspace_sptr m_outputWS;

  Geometry::Instrument_const_sptr m_instrument;

  double m_l2;
  double m_wavelength;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_ConvertToConstantL2_H_*/
