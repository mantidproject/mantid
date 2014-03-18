#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "boost/format.hpp"
#include "boost/algorithm/string/join.hpp"

#include "MantidSINQ/PoldiUtilities/MillerIndicesIO.h"
#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"

#include "MantidAPI/CompositeFunction.h"

namespace Mantid {
namespace Poldi {

using namespace Mantid::API;
using namespace Mantid::DataObjects;

PoldiPeakCollection::PoldiPeakCollection() :
    m_peaks()
{
}

PoldiPeakCollection::PoldiPeakCollection(TableWorkspace_sptr workspace) :
    m_peaks()
{
    if(workspace) {
        constructFromTableWorkspace(workspace);
    }
}

void PoldiPeakCollection::setProfileFunction(IPeakFunction_sptr peakProfile)
{
    m_profileTemplate = peakProfile;

    updatePeakProfileFunctions();
}

void PoldiPeakCollection::setBackgroundFunction(IFunction_sptr backgroundFunction)
{
    m_backgroundTemplate = backgroundFunction;

    updatePeakBackgroundFunctions();
}

void PoldiPeakCollection::setProfileTies(std::string profileTies)
{
    m_ties = profileTies;
}

const std::string &PoldiPeakCollection::getProfileTies() const
{
    return m_ties;
}

size_t PoldiPeakCollection::peakCount() const
{
    return m_peaks.size();
}

void PoldiPeakCollection::addPeak(PoldiPeak_sptr newPeak)
{
    m_peaks.push_back(newPeak);
}

PoldiPeak_sptr PoldiPeakCollection::peak(size_t index) const
{
    if(index >= m_peaks.size()) {
        throw std::range_error("Peak access index out of range.");
    }

    return m_peaks[index];
}

IFunction_sptr PoldiPeakCollection::getPeakProfile(size_t index) const
{
    if(index >= m_peakProfiles.size()) {
        throw std::range_error("Peak access index out of range.");
    }

    IPeakFunction_sptr currentPeakFunction = m_peakProfiles[index];
    PoldiPeak_sptr currentPeak = m_peaks[index];
    currentPeakFunction->setCentre(currentPeak->q());
    currentPeakFunction->setHeight(currentPeak->intensity());
    currentPeakFunction->setFwhm(currentPeak->fwhm());

    CompositeFunction_sptr peakProfile(new CompositeFunction);
    peakProfile->addFunction(currentPeakFunction);
    peakProfile->addFunction(m_backgrounds[index]);

    return peakProfile;
}

void PoldiPeakCollection::setProfileParameters(size_t index, IFunction_sptr fittedFunction)
{
    if(index >= m_peaks.size()) {
        throw std::range_error("Peak access index out of range.");
    }

    CompositeFunction_sptr totalFunction = boost::dynamic_pointer_cast<CompositeFunction>(fittedFunction);

    if(totalFunction) {
        IPeakFunction_sptr peakFunction = boost::dynamic_pointer_cast<IPeakFunction>(totalFunction->getFunction(0));

        if(peakFunction) {
            PoldiPeak_sptr peak = m_peaks[index];

            peak->setIntensity(UncertainValue(peakFunction->height(), peakFunction->getError(0)));
            peak->setQ(UncertainValue(peakFunction->centre(), peakFunction->getError(1)));
            peak->setFwhm(UncertainValue(peakFunction->fwhm(), getFwhmRelation(peakFunction) * peakFunction->getError(2)));
        }
    }
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
        newRow << MillerIndicesIO::toString((*peak)->hkl())
               << UncertainValueIO::toString((*peak)->d())
               << UncertainValueIO::toString((*peak)->q())
               << UncertainValueIO::toString((*peak)->intensity())
               << UncertainValueIO::toString((*peak)->fwhm());
    }
}

void PoldiPeakCollection::constructFromTableWorkspace(TableWorkspace_sptr tableWorkspace)
{
    if(checkColumns(tableWorkspace)) {
        size_t newPeakCount = tableWorkspace->rowCount();
        m_peaks.resize(newPeakCount);

        for(size_t i = 0; i < newPeakCount; ++i) {
            TableRow nextRow = tableWorkspace->getRow(i);
            std::string hklString, dString, qString, intensityString, fwhmString;
            nextRow >> hklString >> dString >> qString >> intensityString >> fwhmString;

            PoldiPeak_sptr peak = PoldiPeak::create(MillerIndicesIO::fromString(hklString),
                                                    UncertainValueIO::fromString(dString),
                                                    UncertainValueIO::fromString(intensityString),
                                                    UncertainValueIO::fromString(fwhmString));
            m_peaks[i] = peak;
        }
    }
}

bool PoldiPeakCollection::checkColumns(TableWorkspace_sptr tableWorkspace)
{
    if(tableWorkspace->columnCount() != 5) {
        return false;
    }

    std::vector<std::string> shouldNames;
    shouldNames.push_back("HKL");
    shouldNames.push_back("d");
    shouldNames.push_back("Q");
    shouldNames.push_back("Intensity");
    shouldNames.push_back("FWHM");

    std::vector<std::string> columnNames = tableWorkspace->getColumnNames();

    return columnNames == shouldNames;
}

void PoldiPeakCollection::addPeakFunctionFromTemplate()
{
    if(m_profileTemplate) {
        m_peakProfiles.push_back(boost::dynamic_pointer_cast<IPeakFunction>(m_profileTemplate->clone()));
    }

    if(m_backgroundTemplate) {
        m_backgrounds.push_back(m_backgroundTemplate->clone());
    }
}

void PoldiPeakCollection::updatePeakProfileFunctions()
{
    m_peakProfiles.clear();

    for(size_t i = 0; i < m_peaks.size(); ++i) {
        m_peakProfiles.push_back(boost::dynamic_pointer_cast<IPeakFunction>(m_profileTemplate->clone()));
    }
}

void PoldiPeakCollection::updatePeakBackgroundFunctions()
{
    m_backgrounds.clear();

    for(size_t i = 0; i < m_peaks.size(); ++i) {
        m_backgrounds.push_back(m_backgroundTemplate->clone());
    }
}

double PoldiPeakCollection::getFwhmRelation(IPeakFunction_sptr peakFunction)
{
    return peakFunction->fwhm() / peakFunction->getParameter(2);
}

}
}
