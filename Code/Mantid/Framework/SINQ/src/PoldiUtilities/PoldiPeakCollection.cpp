#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "boost/format.hpp"

namespace Mantid {
namespace Poldi {

using namespace Mantid::API;
using namespace Mantid::DataObjects;

PoldiPeakCollection::PoldiPeakCollection() :
    m_peaks()
{
}

size_t PoldiPeakCollection::peakCount() const
{
    return m_peaks.size();
}

void PoldiPeakCollection::addPeak(PoldiPeak_sptr newPeak)
{
    m_peaks.push_back(newPeak);
}

TableWorkspace_sptr PoldiPeakCollection::asTableWorkspace()
{
    TableWorkspace_sptr peaks = boost::dynamic_pointer_cast<TableWorkspace>(WorkspaceFactory::Instance().createTable());

    prepareTable(peaks);
    peaksToTable(peaks);

    return peaks;
}

void PoldiPeakCollection::prepareTable(TableWorkspace_sptr table)
{
    table->addColumn("str", "HKL");
    table->addColumn("str", "d");
    table->addColumn("str", "Q");
    table->addColumn("str", "Intensity");
    table->addColumn("str", "FWHM");
}

void PoldiPeakCollection::peaksToTable(TableWorkspace_sptr table)
{
    for(std::vector<PoldiPeak_sptr>::const_iterator peak = m_peaks.begin(); peak != m_peaks.end(); ++peak) {
        TableRow newRow = table->appendRow();
        MillerIndices hkl = (*peak)->hkl();
        std::string hklString = (boost::format("%i %i %i") % hkl[0] % hkl[1] % hkl[2]).str();
        newRow << hklString << std::string((*peak)->d()) << std::string((*peak)->q()) << std::string((*peak)->intensity()) << std::string((*peak)->fwhm());
    }
}

}
}
