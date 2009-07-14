//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadSNSNexus.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidGeometry/Detector.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/LogParser.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <cmath>
#include <sstream>
#include <limits>

namespace Mantid
{
namespace NeXus
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSNSNexus)

using namespace Kernel;
using namespace API;

/// Empty default constructor
LoadSNSNexus::LoadSNSNexus() {}

/// Initialisation method.
void LoadSNSNexus::init()
{
    std::vector<std::string> exts;
    exts.push_back("NXS");
    exts.push_back("nxs");
    declareProperty("Filename","",new FileValidator(exts));
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("SpectrumMin", EMPTY_INT(), mustBePositive);
    declareProperty("SpectrumMax", EMPTY_INT(), mustBePositive->clone());
    declareProperty(new ArrayProperty<int>("SpectrumList"));
}

/// Class for comparing bank names in format "bank123" according to the numeric part of the name
class CompareBanks
{
public:
    bool operator()(const std::string& s1, const std::string& s2)
    {
        int i1 = atoi( s1.substr(4,s1.size()-4).c_str() );
        int i2 = atoi( s2.substr(4,s2.size()-4).c_str() );
        return i1 < i2;
    }
};

/** Executes the algorithm. Reading in the file and creating and populating
*  the output workspace
* 
*  @throw Exception::FileError If the Nexus file cannot be found/opened
*  @throw std::invalid_argument If the optional properties are set to invalid values
*/
void LoadSNSNexus::exec()
{
    // Create the root Nexus class
    NXRoot root(getPropertyValue("Filename"));

    int nPeriods = root.groups().size();
    WorkspaceGroup_sptr sptrWSGrp= WorkspaceGroup_sptr(new WorkspaceGroup);

    if (nPeriods > 1)
    {
        setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(sptrWSGrp));
    }

    int period = 0;
    for(std::vector<NXClassInfo>::const_iterator it=root.groups().begin();it!=root.groups().end();it++)
    if (it->nxclass == "NXentry")
    {
        ++period;
        Workspace_sptr ws = loadEntry(root.openEntry(it->nxname),period);

        // Save the workspace property
        if (nPeriods == 1)
            setProperty("OutputWorkspace",ws);
        else // for higher periods create new workspace properties
        {
            std::ostringstream suffix;
            suffix << '_' << period;
            std::string wsName = getProperty("OutputWorkspace");
            wsName += suffix.str();
            std::string wsPropertyName = "OutputWorkspace" + suffix.str();
            declareProperty(new WorkspaceProperty<Workspace>(wsPropertyName,wsName,Direction::Output));
            setProperty(wsPropertyName,ws);
            sptrWSGrp->add(wsName);
        }
    }

}

/** Loads one entry from an SNS Nexus file
 *  @param entry The entry to read the data from
 *  @param period The period of the data
 *  @return A shared pointer to the created workspace
 */
API::Workspace_sptr LoadSNSNexus::loadEntry(NXEntry entry,int period)
{
    // To keep sorted bank names
    std::set<std::string,CompareBanks> banks;

    // Find out the workspace dimensions
    int nSpectra = 0;
    int nBins = 0;
    for(std::vector<NXClassInfo>::const_iterator it=entry.groups().begin();it!=entry.groups().end();it++)
    if (it->nxclass == "NXdata")
    {
            NXData dataGroup = entry.openNXData(it->nxname);
            NXFloat data = dataGroup.openFloatData();
            if (data.rank() != 3) throw std::runtime_error("SNS NXdata is expected to be a 3D array");
            if (nBins == 0) nBins = data.dim2();
            nSpectra += data.dim0() * data.dim1();
            banks.insert(it->nxname);
    }

    DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",nSpectra,nBins+1,nBins));
    ws->setTitle(entry.name());
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    int spec = 0;
    Progress progress(this,0,1,nSpectra);
    for(std::set<std::string,CompareBanks>::const_iterator it=banks.begin();it!=banks.end();it++)
    {
        NXData dataGroup = entry.openNXData(*it);
        if (spec == 0)
        {
            NXFloat timeBins = dataGroup.openNXFloat("time_of_flight");
            timeBins.load();
            MantidVec& X = ws->dataX(0);
            for(int i=0;i<X.size();i++)
                X[i] = timeBins[i];
        }
        NXFloat data = dataGroup.openFloatData();
        for(int i = 0;i<data.dim0();i++)
            for(int j = 0; j < data.dim1(); j++)
            {
                if (spec > 0) ws->dataX(spec) = ws->dataX(0);
                data.load(i,j);
                ////-- simulate input --
                //for(int k=0;k<nBins;k++)
                //    data()[k] = float(i*1000 + j*100 + k);
                ////--------------------
                MantidVec& Y = ws->dataY(spec);
                Y.assign(data(),data()+nBins);
                MantidVec& E = ws->dataE(spec);
                std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
                spec++;
                progress.report();
            }
    }

    return ws;
}

/** Creates a list of selected spectra to load from input interval and list properties.
 *  @return An integer vector with spectra numbers to load. If an empty vector is returned
 *  load all spectra in the file.
 */
std::vector<int> LoadSNSNexus::getSpectraSelection()
{
    std::vector<int> spec_list = getProperty("SpectrumList");
    int spec_max = getProperty("SpectrumMax");
    int spec_min = getProperty("SpectrumMin");
    bool is_list = !spec_list.empty();
    bool is_interval = (spec_max != EMPTY_INT());
    if ( spec_max == EMPTY_INT() ) spec_max = 0;

    // Compile a list of spectra numbers to load
    std::vector<int> spec;
    if( is_interval )
    {
        if ( spec_max < spec_min )
        {
            g_log.error("Invalid Spectrum min/max properties");
            throw std::invalid_argument("Inconsistent properties defined");
        }
        for(int i=spec_min;i<=spec_max;i++)
            spec.push_back(i);
        if (is_list)
        {
            for(size_t i=0;i<spec_list.size();i++)
            {
                int s = spec_list[i];
                if ( s < 0 ) continue;
                if (s < spec_min || s > spec_max)
                    spec.push_back(s);
            }
        }
    }
    else if (is_list)
    {
        spec_max=0;
        spec_min=std::numeric_limits<int>::max();
        for(size_t i=0;i<spec_list.size();i++)
        {
            int s = spec_list[i];
            if ( s < 0 ) continue;
            spec.push_back(s);
            if (s > spec_max) spec_max = s;
            if (s < spec_min) spec_min = s;
        }
    }
    else
    {
        //spec_min=0;
        //spec_max=nSpectra;
        //for(int i=spec_min;i<=spec_max;i++)
        //    spec.push_back(i);
    }

    return spec;
}

double LoadSNSNexus::dblSqrt(double in)
{
    return sqrt(in);
}
} // namespace DataHandling
} // namespace Mantid
