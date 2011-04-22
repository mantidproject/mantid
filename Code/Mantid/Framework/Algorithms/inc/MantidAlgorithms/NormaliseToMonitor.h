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
/** Normalises a 2D workspace by a specified monitor spectrum. By default ,the
    normalisation is done bin-by-bin following this formula:
    Norm(s_i)=(s_i/m_i)*Dlam_i*Sum(m_i)/Sum(Dlam_i)
    where s_i is the signal in bin i, m_i the count in the corresponding monitor bin,
    Dlam_i the width of the bin, Sum(m_i) is the integrated monitor count and
    Sum(Dlam_i) the sum of all bin widths (the full range).

    Optionally, can instead normalise by the integrated monitor count over a specified
    range in X. In this case all bins in all spectra will simply be divided by this
    integrated count. No bin width correction takes place in this case.

    The monitor spectrum can be provided either as an index in the main input workspace
    or as a separate single-spectrum workspace.

    Required Properties:
    <UL>
    <LI> InputWorkspace   - The name of the input workspace. Must be a histogram
                            and not a distribution.</LI>
    <LI> OutputWorkspace  - The name of the output workspace. </LI>
    <LI> MonitorSpectrum  - The spectrum number for the monitor to normalise with. </LI>
    <LI> MonitorWorkspace - A workspace containing the monitor spectrum. </LI>
    </UL>

    Optional Properties:
    These should be set to normalise by an integrated monitor count over the range given
    <UL>
    <LI> IntegrationRangeMin - The lower bound of the range to use. </LI>
    <LI> IntegrationRangeMax - The upper bound of the range to use. </LI>
    <LI> IncludePartialBins  - Scales counts in end bins if min/max not on bin boundary. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 30/09/2008

    Copyright &copy; 2008-11 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();

  void checkProperties(API::MatrixWorkspace_sptr inputWorkspace);
  API::MatrixWorkspace_sptr getInWSMonitorSpectrum(API::MatrixWorkspace_sptr inputWorkspace);
  API::MatrixWorkspace_sptr getMonitorWorkspace(API::MatrixWorkspace_sptr inputWorkspace);
  API::MatrixWorkspace_sptr extractMonitorSpectrum(API::MatrixWorkspace_sptr WS, const int index);
  bool setIntegrationProps();

  void normaliseByIntegratedCount(API::MatrixWorkspace_sptr inputWorkspace, 
                                  API::MatrixWorkspace_sptr& outputWorkspace);

  void normaliseBinByBin(API::MatrixWorkspace_sptr inputWorkspace,
                         API::MatrixWorkspace_sptr& outputWorkspace);
  void normalisationFactor(const MantidVec& X, MantidVec* Y, MantidVec* E);

  /// A single spectrum workspace containing the monitor
  API::MatrixWorkspace_sptr m_monitor;
  /// Whether the input workspace has common bins
  bool m_commonBins;
  /// The lower bound of the integration range
  double m_integrationMin;
  /// The upper bound of the integration range
  double m_integrationMax;
};

} // namespace Algorithm
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_NORMALISETOMONITOR_H_ */
