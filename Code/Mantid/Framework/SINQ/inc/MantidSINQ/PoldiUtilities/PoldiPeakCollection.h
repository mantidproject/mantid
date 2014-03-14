#ifndef MANTID_SINQ_POLDIPEAKCOLLECTION_H
#define MANTID_SINQ_POLDIPEAKCOLLECTION_H

#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "boost/shared_ptr.hpp"

namespace Mantid {
namespace Poldi {

using namespace Mantid::DataObjects;

class PoldiPeakCollection;

typedef boost::shared_ptr<PoldiPeakCollection> PoldiPeakCollection_sptr;

class MANTID_SINQ_DLL PoldiPeakCollection
{
public:
    PoldiPeakCollection();
    ~PoldiPeakCollection() {}

    size_t peakCount() const;

    void addPeak(PoldiPeak_sptr newPeak);

    TableWorkspace_sptr asTableWorkspace();

private:
    void prepareTable(TableWorkspace_sptr table);
    void peaksToTable(TableWorkspace_sptr table);

    std::vector<PoldiPeak_sptr> m_peaks;
};

}
}

#endif // MANTID_SINQ_POLDIPEAKCOLLECTION_H
