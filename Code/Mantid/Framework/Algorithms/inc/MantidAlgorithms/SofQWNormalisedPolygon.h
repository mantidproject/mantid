#ifndef MANTID_ALGORITHMS_SOFQWNORMALISEDPOLYGON_H_
#define MANTID_ALGORITHMS_SOFQWNORMALISEDPOLYGON_H_
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Rebin2D.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidGeometry/IDetector.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include <list>
#include "MantidAlgorithms/SofQCommon.h"

namespace Mantid {
namespace Algorithms {

/**
Converts a 2D workspace that has axes of energy transfer against spectrum number
to
one that gives intensity as a function of momentum transfer against energy. This
version
uses proper parallelpiped rebinning to compute the overlap of the various
overlapping
weights.

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

@date 2012/05/04

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
class DLLExport SofQWNormalisedPolygon : public Rebin2D {
public:
  /// Default constructor
  SofQWNormalisedPolygon();
  /// Algorithm's name for identification
  virtual const std::string name() const;
  virtual const std::string alias() const { return "SofQW3"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculate the intensity as a function of momentum transfer and "
           "energy.";
  }

  /// Algorithm's version for identification
  virtual int version() const;
  /// Algorithm's category for identification
  virtual const std::string category() const;

private:
  /// Initialize the algorithm
  void init();
  /// Run the algorithm
  void exec();

  /// Calculate the Q value for given conditions.
  double calculateQ(const double efixed, const int emode, const double deltaE,
                    const double twoTheta, const double azimuthal) const;
  /// Init variables cache base on the given workspace
  void initCachedValues(const API::MatrixWorkspace_const_sptr &workspace);
  /// Init the theta index
  void
  initAngularCachesNonPSD(const API::MatrixWorkspace_const_sptr &workspace);
  /// Get angles and calculate angular widths.
  void initAngularCachesPSD(const API::MatrixWorkspace_const_sptr &workspace);

  /// Create the output workspace
  DataObjects::RebinnedOutput_sptr
  setUpOutputWorkspace(API::MatrixWorkspace_const_sptr inputWorkspace,
                       const std::vector<double> &binParams,
                       std::vector<double> &newAxis);

  SofQCommon m_EmodeProperties;
  /// Output Q axis
  std::vector<double> m_Qout;
  /// Single value theta width
  double m_thetaWidth;
  /// Array for the two theta angles
  std::vector<double> m_theta;
  /// Array for the azimuthal angles
  std::vector<double> m_phi;
  /// Array for the theta widths
  std::vector<double> m_thetaWidths;
  /// Array for the azimuthal widths
  std::vector<double> m_phiWidths;
  /// Offset for finding neighbor in nearest tube
  int m_detNeighbourOffset;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SOFQWNORMALISEDPOLYGON_H_ */
