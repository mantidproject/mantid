#ifndef MANTID_ALGORITHMS_QXY_H_
#define MANTID_ALGORITHMS_QXY_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** This algorithm rebins a 2D workspace in units of wavelength into 2D Q.
    The result is stored in a 2D workspace with units of Q on both axes.
    @todo Doesn't (yet) calculate the errors.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The corrected data in units of wavelength. </LI>
    <LI> OutputWorkspace - The workspace in which to store data as x & y
   components of Q. </LI>
    <LI> MaxQxy          - The upper limit of the Qx-Qy grid (goes from -MaxQxy
   to +MaxQxy). </LI>
    <LI> DeltaQ          - The dimension of a Qx-Qy cell. </LI>
    <LI> AccountForGravity - If true, account for gravity. </LI>
    <LI> SolidAngleWeighting - If true, pixels will be weighted by their solid
   angle. </LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 09/04/2009

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
class DLLExport Qxy : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "Qxy"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs the final part of a SANS (LOQ/SANS2D) two dimensional (in "
           "Q) data reduction.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"Q1D"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  std::vector<double> logBinning(double min, double max, int num);
  API::MatrixWorkspace_sptr
  setUpOutputWorkspace(API::MatrixWorkspace_const_sptr inputWorkspace);
  double getQminFromWs(const API::MatrixWorkspace &inputWorkspace);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_QXY_H_*/
