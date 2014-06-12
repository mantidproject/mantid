#ifndef MANTID_SINQ_POLDIINSTRUMENTADAPTER_H_
#define MANTID_SINQ_POLDIINSTRUMENTADAPTER_H_

#include "MantidKernel/System.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/ExperimentInfo.h"

#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h"
#include "MantidSINQ/PoldiUtilities/PoldiSourceSpectrum.h"

namespace Mantid
{
namespace Poldi
{

/** PoldiInstrumentAdapter :
    Adapter for constructing POLDI objects on the basis
    of Mantid's instrument-tools.

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

class MANTID_SINQ_DLL PoldiInstrumentAdapter
{
public:
    PoldiInstrumentAdapter(Geometry::Instrument_const_sptr mantidInstrument, const API::Run &runInformation);
    virtual ~PoldiInstrumentAdapter();

    PoldiAbstractChopper_sptr chopper() const;
    PoldiAbstractDetector_sptr detector() const;
    PoldiSourceSpectrum_sptr spectrum() const;

protected:
    PoldiInstrumentAdapter() { }

    void setDetector(Geometry::Instrument_const_sptr mantidInstrument);
    void setChopper(Geometry::Instrument_const_sptr mantidInstrument, const API::Run &runInformation);
    void setSpectrum(Geometry::Instrument_const_sptr mantidInstrument);

    PoldiAbstractChopper_sptr m_chopper;
    PoldiAbstractDetector_sptr m_detector;
    PoldiSourceSpectrum_sptr m_spectrum;
};

typedef boost::shared_ptr<PoldiInstrumentAdapter> PoldiInstrumentAdapter_sptr;

} // namespace Poldi
} // namespace Mantid

#endif  /* MANTID_SINQ_POLDIINSTRUMENTADAPTER_H_ */
