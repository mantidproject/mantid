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
  /// Default constructor
  MaxMin() : API::Algorithm(){};
  /// Destructor
  virtual ~MaxMin(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "MaxMin"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Takes a 2D workspace as input and find the maximum (minimum) in "
           "each 1D spectrum.";
  }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Arithmetic"; }

private:
  // Overridden Algorithm methods
  void init();
  void exec();

  /// The flag to show minimum
  bool showMin;
  /// The value in X to start the search from
  double m_MinRange;
  /// The value in X to finish the search at
  double m_MaxRange;
  /// The spectrum to start the integration from
  int m_MinSpec;
  /// The spectrum to finish the integration at
  int m_MaxSpec;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_MAXMIN_H_*/
