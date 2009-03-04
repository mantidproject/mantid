#ifndef MANTID_ALGORITHMS_POINTBYPOINTVCORRECTION_H_
#define MANTID_ALGORITHMS_POINTBYPOINTVCORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
namespace Mantid
{
namespace Algorithms
{
/** Normalises a 2D workspace by a specified monitor spectrum. By default ,the
    normalisation is done bin-by-bin following this formula:
    Norm(s_i)=(s_i/m_i)*Dlam_i*Sum(m_i)/Sum(Dlam_i)
    where s_i is the signal in bin i, m_i the count in the corresponding monitor bin,
    Dlam_i the width of the bin, Sum(m_i) is the integrated monitor count and
    Sum(Dlam_i) the sum of all bin widths (the full range).

    Optionally, can instead normalise by the integrated monitor count over a specified
    range in X. In this case, the range of the output workspace will have its limits
    at the closest bins within the range values given (i.e. bins may be removed with
    respect to the start and end of the input workspace, but the bin boundaries will
    remain the same). No bin width correction takes place in this case.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input Workspace2D. Must be a histogram with
              common bins and not a distribution.</LI>
    <LI> OutputWorkspace - The name of the output Workspace2D. </LI>
    <LI> MonitorSpectrum - The spectrum number for the monitor to normalise with </LI>
    </UL>

    Optional Properties:
    These should be set to normalise by an integrated monitor count over the range given
    <UL>
    <LI> IntegrationRangeMin - The lower bound of the range to use </LI>
    <LI> IntegrationRangeMax - The upper bound of the range to use </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 30/09/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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
class DLLExport PointByPointVCorrection : public API::Algorithm
{
public:
  PointByPointVCorrection();
  virtual ~PointByPointVCorrection();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "PointByPointVCorrection"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction";}

private:
  // Overridden Algorithm methods
  void init();
  void exec();
  void check_validity(Mantid::API::MatrixWorkspace_sptr& w1,
		  Mantid::API::MatrixWorkspace_sptr& w2,
		  Mantid::API::MatrixWorkspace_sptr& out);
  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // namespace Algorithm
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_NORMALISETOMONITOR_H_ */
