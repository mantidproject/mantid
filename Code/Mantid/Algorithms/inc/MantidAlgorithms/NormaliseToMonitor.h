#ifndef MANTID_ALGORITHMS_NORMALISETOMONITOR_H_
#define MANTID_ALGORITHMS_NORMALISETOMONITOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Normalises a 2D workspace by a specified monitor spectrum. The output workspace will
    have its data divided by the bin width, whether or not the input one does.
    Optionally, can instead normalise by the integrated monitor count over a specified
    range in X. In this case, the range of the output workspace will have its limits
    at the closest bins within the range values given (i.e. bins may be removed with
    respect to the start and end of the input workspace, but the bin boundaries will
    remain the same).

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input Workspace2D. </LI>
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
class DLLExport NormaliseToMonitor : public API::Algorithm
{
public:
  NormaliseToMonitor();
  virtual ~NormaliseToMonitor();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "NormaliseToMonitor"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  // Overridden Algorithm methods
  void init();
  void exec();

  const bool checkProperties();
  void findMonitorIndex(API::Workspace_const_sptr inputWorkspace);
  API::Workspace_sptr normaliseByIntegratedCount(API::Workspace_sptr inputWorkspace);
  void doUndoDistribution(API::Workspace_sptr workspace, const bool forwards = true);

  int m_monitorIndex;
  double m_integrationMin;
  double m_integrationMax;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // namespace Algorithm
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_NORMALISETOMONITOR_H_ */
