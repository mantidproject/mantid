#ifndef MANTID_ALGORITHMS_CONVERTSPECTRUMAXIS_H_
#define MANTID_ALGORITHMS_CONVERTSPECTRUMAXIS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid {
namespace Algorithms {
/** Converts the representation of the vertical axis (the one up the side of
    a matrix in MantidPlot) of a Workspace2D from its default of holding the
    spectrum number to the target unit given.
    At present, the only implemented unit is theta (actually TwoTheta). The
   spectra
    will be reordered by increasing theta and duplicates will not be aggregated.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. </LI>
    <LI> Target          - The unit to which the spectrum axis should be
   converted. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 01/09/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ConvertSpectrumAxis : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ConvertSpectrumAxis"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts the axis of a Workspace2D which normally holds spectrum "
           "numbers to some other unit, which will normally be some physical "
           "value about the instrument such as Q, Q^2 or theta.  'Note': After "
           "running this algorithm, some features will be unavailable on the "
           "workspace as it will have lost all connection to the instrument. "
           "This includes things like the 3D Instrument Display.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Transforms\\Units;Transforms\\Axes";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Getting Efixed
  double getEfixed(const Mantid::Geometry::IDetector &detector,
                   API::MatrixWorkspace_const_sptr inputWS, int emode) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTSPECTRUMAXIS_H_*/
