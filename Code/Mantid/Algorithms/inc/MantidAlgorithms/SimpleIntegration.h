#ifndef MANTID_ALGORITHMS_SIMPLEINTEGRATION_H_
#define MANTID_ALGORITHMS_SIMPLEINTEGRATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Takes a workspace as input and sums each Histogram1D contained within
    it, storing the result as a Workspace of Histogram1Ds with one Y & E value
    and two X values indicating the range which the integration covers.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> Range_lower - The X value to integrate from (default 0)</LI>
    <LI> Range_upper - The X value to integrate to (default max)</LI>
    <LI> StartSpectrum - Workspace index number to integrate from (default 0)</LI>
    <LI> EndSpectrum - Workspace index number to integrate to (default max)</LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 05/10/2007
	@author L C Chapon, ISIS, Rutherford Appleton Laboratory
	@Date 01/03/2009 Modified to account for distribution.
    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport SimpleIntegration : public API::Algorithm
{
public:
  /// Default constructor
  SimpleIntegration() : API::Algorithm() {};
  /// Destructor
  virtual ~SimpleIntegration() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "Integration";}
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  // Overridden Algorithm methods
  void init();
  void exec();

  /// The value in X to start the integration from
  double m_MinRange;
  /// The value in X to finish the integration at
  double m_MaxRange;
  /// The spectrum to start the integration from
  int m_MinSpec;
  /// The spectrum to finish the integration at
  int m_MaxSpec;

  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SIMPLEINTEGRATION_H_*/
