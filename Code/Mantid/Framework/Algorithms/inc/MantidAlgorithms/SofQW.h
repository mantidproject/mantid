#ifndef MANTID_ALGORITHMS_SOFQW_H_
#define MANTID_ALGORITHMS_SOFQW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SofQCommon.h"

namespace Mantid {
namespace Algorithms {
/** Converts a 2D workspace that has axes of energy transfer against spectrum
number to
one that gives intensity as a function of momentum transfer against energy.

Required Properties:
<UL>
<LI> InputWorkspace  - Reduced data in units of energy transfer. Must have
common bins. </LI>
<LI> OutputWorkspace - The name to use for the q-w workspace. </LI>
<LI> QAxisBinning    - The bin parameters to use for the q axis. </LI>
<LI> Emode           - The energy mode (direct or indirect geometry). </LI>
<LI> Efixed          - Value of fixed energy: EI (emode=1) or EF (emode=2)
(meV). </LI>
</UL>

@author Russell Taylor, Tessella plc
@date 24/02/2010

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SofQW : public API::Algorithm {
public:
  /// (Empty) Constructor
  SofQW() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~SofQW() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SofQW"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const;

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Inelastic"; }
  /// Create the output workspace
  static API::MatrixWorkspace_sptr
  setUpOutputWorkspace(API::MatrixWorkspace_const_sptr inputWorkspace,
                       const std::vector<double> &binParams,
                       std::vector<double> &newAxis);
  /// Convert the workspace to a distribution
  void makeDistribution(API::MatrixWorkspace_sptr outputWS,
                        const std::vector<double> qAxis);
  /// Create the input properties on the given algorithm object
  static void createInputProperties(API::Algorithm &alg);
  /// Energy to K constant
  static double energyToK();

private:
  /// Initialization code
  void init();
  /// Execution code
  void exec();

  SofQCommon m_EmodeProperties;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SOFQW_H_*/
