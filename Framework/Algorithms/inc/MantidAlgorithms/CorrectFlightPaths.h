#ifndef MANTID_ALGORITHMS_CorrectFlightPaths_H_
#define MANTID_ALGORITHMS_CorrectFlightPaths_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Algorithms {
/** Correct flight paths

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
class DLLExport CorrectFlightPaths : public API::Algorithm {
public:
  /// Default constructor
  CorrectFlightPaths();
  /// Destructor
  virtual ~CorrectFlightPaths(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "CorrectFlightPaths"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Used to correct flight paths in 2D shaped detectors.";
  }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "Inelastic;CorrectionFunctions";
  }

private:
  // Overridden Algorithm methods
  void init();
  void exec();
  void initWorkspaces();
  double getRunProperty(std::string);
  double getInstrumentProperty(std::string);
  double calculateTOF(double);

  /// The user selected (input) workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// The output workspace, maybe the same as the input one
  API::MatrixWorkspace_sptr m_outputWS;

  Geometry::Instrument_const_sptr m_instrument;
  Geometry::IComponent_const_sptr m_sample;

  double m_l2;
  double m_wavelength;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CorrectFlightPaths_H_*/
