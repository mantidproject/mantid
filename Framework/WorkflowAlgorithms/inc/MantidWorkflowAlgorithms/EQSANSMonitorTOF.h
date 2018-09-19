#ifndef MANTID_ALGORITHMS_EQSANSMONITORTOF_H_
#define MANTID_ALGORITHMS_EQSANSMONITORTOF_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Determine the wavelength from the TOF in the beam monitor histogram.  The
   algorithm has to modify
    TOF values to correct for the fact that T_0 is not properly recorded by the
   DAS.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
// Pulse widge (micro sec per angstrom)
const double PULSEWIDTH = 20.0;
// Chopper phase offset (micro sec)
const double CHOPPER_PHASE_OFFSET[2][4] = {{9507., 9471., 9829.7, 9584.3},
                                           {19024., 18820., 19714., 19360.}};
// Chopper angles (degree)
const double CHOPPER_ANGLE[4] = {129.605, 179.989, 230.010, 230.007};
// Chopper location (mm)
const double CHOPPER_LOCATION[4] = {5700., 7800., 9497., 9507.};

class DLLExport EQSANSMonitorTOF : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "EQSANSMonitorTOF"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts the TOF into a wavelength for the beam monitor. This "
           "algorithm needs to be run once on every data set.";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Workflow\\SANS"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Compute TOF offset
  double getTofOffset(API::MatrixWorkspace_const_sptr inputWS,
                      bool frame_skipping, double source_to_monitor);
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSMONITORTOF_H_*/
