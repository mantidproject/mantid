#ifndef MANTID_ALGORITHMS_MAXMIN_H_
#define MANTID_ALGORITHMS_MAXMIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Takes a 2D workspace as input and find the maximum(minimum) in each 1D
   spectrum.
    The algorithm creates a new 1D workspace containing all maxima(minima) as
   well as their X boundaries
    and error. This is used in particular for single crystal as a quick way to
   find strong peaks.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> ShowMin - boolean value to show the minimum (defult=false). If set to
   true, it will show minimum, else it will show maximum</LI>
    <LI> Range_lower - The X value to search from (default 0)</LI>
    <LI> Range_upper - The X value to search to (default max)</LI>
    <LI> StartSpectrum - Start spectrum number (default 0)</LI>
    <LI> EndSpectrum - End spectrum number  (default max)</LI>
    </UL>

    @date 12/27/2011
    Copyright &copy; 2007-2012 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
class DLLExport MaxMin : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "MaxMin"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Takes a 2D workspace as input and find the maximum (minimum) in "
           "each 1D spectrum.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"Max", "Min"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_MAXMIN_H_*/
