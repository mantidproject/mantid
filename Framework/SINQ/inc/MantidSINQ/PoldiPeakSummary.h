#ifndef MANTID_POLDI_POLDIPEAKSUMMARY_H_
#define MANTID_POLDI_POLDIPEAKSUMMARY_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"

#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"

namespace Mantid {
namespace Poldi {

/** PoldiPeakSummary

  This small algorithm produces a summary table for a given
  PoldiPeakCollection, similar to what the original data
  analysis software produced.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 03/12/2014

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
class MANTID_SINQ_DLL PoldiPeakSummary : public API::Algorithm {
public:
  PoldiPeakSummary();
  virtual ~PoldiPeakSummary();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

protected:
  DataObjects::TableWorkspace_sptr
  getSummaryTable(const PoldiPeakCollection_sptr &peakCollection) const;
  DataObjects::TableWorkspace_sptr getInitializedResultWorkspace() const;

  void storePeakSummary(API::TableRow tableRow,
                        const PoldiPeak_sptr &peak) const;

private:
  void init();
  void exec();
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_POLDI_POLDIPEAKSUMMARY_H_ */
