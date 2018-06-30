#ifndef MANTID_ALGORITHMS_SOFQWPOLYGON_H_
#define MANTID_ALGORITHMS_SOFQWPOLYGON_H_
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Rebin2D.h"
#include "MantidAlgorithms/SofQCommon.h"
#include <list>

namespace Mantid {
namespace Algorithms {

/**
Converts a 2D workspace that has axes of energy transfer against spectrum number
to
one that gives intensity as a function of momentum transfer against energy. This
version
uses proper parallelpiped rebinning to compute the overlap of the various
overlapping
weights

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

@author Martyn Giggg
@date 2011-07-15

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SofQWPolygon : public Rebin2D {
public:
  /// Default constructor
  SofQWPolygon();
  /// Algorithm's name for identification
  const std::string name() const override { return "SofQWPolygon"; }
  const std::string alias() const override { return "SofQW2"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate the intensity as a function of momentum transfer and "
           "energy.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"SofQW", "SofQWNormalisedPolygon", "Rebin2D"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Inelastic\\SofQW"; }

private:
  /// Initialize the algorithm
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Init variables cache base on the given workspace
  void initCachedValues(API::MatrixWorkspace_const_sptr workspace);
  /// Init the theta index
  void initThetaCache(const API::MatrixWorkspace &workspace);

  SofQCommon m_EmodeProperties;
  /// Output Q axis
  std::vector<double> m_Qout;
  /// Input theta points
  std::vector<double> m_thetaPts;
  /// Theta width
  double m_thetaWidth;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SOFQWPOLYGON_H_ */
