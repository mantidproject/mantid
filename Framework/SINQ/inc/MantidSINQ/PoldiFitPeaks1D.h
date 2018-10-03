#ifndef MANTID_SINQ_POLDIFITPEAKS1D_H_
#define MANTID_SINQ_POLDIFITPEAKS1D_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"

namespace Mantid {
namespace Poldi {
/** PoldiFitPeaks1D :

  PoldiFitPeaks1D fits multiple peaks to POLDI auto-correlation data.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 17/03/2014

  Copyright © 2014 PSI-MSS

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
class MANTID_SINQ_DLL PoldiFitPeaks1D : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "PoldiPeakFit1D fits peak profiles to POLDI auto-correlation data.";
  }

  int version() const override;
  const std::string category() const override;

protected:
  void setPeakFunction(const std::string &peakFunction);
  PoldiPeakCollection_sptr getInitializedPeakCollection(
      const DataObjects::TableWorkspace_sptr &peakTable) const;

  API::IFunction_sptr getPeakProfile(const PoldiPeak_sptr &poldiPeak) const;
  void
  setValuesFromProfileFunction(PoldiPeak_sptr poldiPeak,
                               const API::IFunction_sptr &fittedFunction) const;
  double getFwhmWidthRelation(API::IPeakFunction_sptr peakFunction) const;

  API::IAlgorithm_sptr
  getFitAlgorithm(const DataObjects::Workspace2D_sptr &dataWorkspace,
                  const PoldiPeak_sptr &peak,
                  const API::IFunction_sptr &profile);

  PoldiPeakCollection_sptr m_peaks;
  std::string m_profileTemplate;
  API::IFunction_sptr m_backgroundTemplate;
  std::string m_profileTies;

  double m_fwhmMultiples{1.0};

private:
  void init() override;
  void exec() override;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDIFITPEAKS1D_H_ */
