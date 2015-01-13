#ifndef MANTID_ALGORITHMS_SOFQW2_H_
#define MANTID_ALGORITHMS_SOFQW2_H_
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SofQCommon.h"
#include "MantidAlgorithms/Rebin2D.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidGeometry/IDetector.h"
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
class DLLExport SofQW2 : public Rebin2D {
public:
  /// Default constructor
  SofQW2();
  /// Algorithm's name for identification
  virtual const std::string name() const { return "SofQW2"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculate the intensity as a function of momentum transfer and "
           "energy.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Inelastic"; }

private:
  /// Initialize the algorithm
  void init();
  /// Run the algorithm
  void exec();
  //      /// Rebin the inputQ
  //      void rebinToOutput(const Geometry::Quadrilateral & inputQ,
  //      API::MatrixWorkspace_const_sptr inputWS,
  //                         const size_t i, const size_t j,
  //                         API::MatrixWorkspace_sptr outputWS);
  /// Calculate the Q value for a direct instrument
  double calculateDirectQ(const double efixed, const double deltaE,
                          const double twoTheta, const double psi) const;
  /// Calculate the Q value for an indirect instrument
  double calculateIndirectQ(const double efixed, const double deltaE,
                            const double twoTheta, const double psi) const;
  //      /// Find the intersect region on the output grid
  //      bool getIntersectionRegion(API::MatrixWorkspace_const_sptr outputWS,
  //      const Geometry::Quadrilateral & inputQ,
  //                                 size_t &qstart, size_t &qend, size_t
  //                                 &en_start, size_t &en_end) const;
  /// Init variables cache base on the given workspace
  void initCachedValues(API::MatrixWorkspace_const_sptr workspace);
  /// Init the theta index
  void initThetaCache(API::MatrixWorkspace_const_sptr workspace);

  SofQCommon m_EmodeProperties;
  //---------------------------------------------------------------------------------
  /// Output Q axis
  std::vector<double> m_Qout;
  /// Input theta points
  std::vector<double> m_thetaPts;
  /// Theta width
  double m_thetaWidth;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SOFQW2_H_ */
