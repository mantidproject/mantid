#ifndef MANTID_ALGORITHMS_POINTBYPOINTVCORRECTION_H_
#define MANTID_ALGORITHMS_POINTBYPOINTVCORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** First attempt at spectrum by spectrum division for vanadium normalisation
   correction.

    @author Laurent Chapon, ISIS Facility, Rutherford Appleton Laboratory
    @date 04/03/2009

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
class DLLExport PointByPointVCorrection : public API::Algorithm {
public:
  PointByPointVCorrection();
  ~PointByPointVCorrection() override;
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PointByPointVCorrection"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Spectrum by spectrum division for vanadium normalisation "
           "correction.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Diffraction\\Corrections;CorrectionFunctions\\SpecialCorrections";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  void check_validity(API::MatrixWorkspace_const_sptr &w1,
                      API::MatrixWorkspace_const_sptr &w2,
                      API::MatrixWorkspace_sptr &out);
  void check_masks(const API::MatrixWorkspace_const_sptr &w1,
                   const API::MatrixWorkspace_const_sptr &w2,
                   const int &index) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_POINTBYPOINTVCORRECTION_H_ */
