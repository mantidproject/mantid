//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadInstrumentFromSNSNexus.h"
#include "MantidNexus/LoadSNSNexus.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/LogParser.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/PhysicalConstants.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
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
LoadSNSNexus::LoadSNSNexus():m_L1(0) {}

/// Initialisation method.
void LoadSNSNexus::init()
{
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
		    "The name of the SNS Nexus file to load" );      
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("SpectrumMin", EMPTY_INT(), mustBePositive);
    declareProperty("SpectrumMax", EMPTY_INT(), mustBePositive->clone());
    declareProperty(new ArrayProperty<int>("SpectrumList"));
}

/** Executes the algorithm. Reading in the file and creating and populating
*  the output workspace
* 
*  @throw Exception::FileError If the Nexus file cannot be found/opened
*  @throw std::invalid_argument If the optional properties are set to invalid values
*/
void LoadSNSNexus::exec()
{
    std::cerr << "00:" << std::endl; // REMOVE
    // Create the root Nexus class
    NXRoot root(getPropertyValue("Filename"));

    int nPeriods = root.groups().size();
    WorkspaceGroup_sptr sptrWSGrp= WorkspaceGroup_sptr(new WorkspaceGroup);
    std::cerr << "01:" << std::endl; // REMOVE
    if (nPeriods > 1)
    {
        setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(sptrWSGrp));
    }
    std::cerr << "02:" << std::endl; // REMOVE
    double progress_step = 1.0/nPeriods;
    int period = 0;
    // Loop over entries loading each entry into a separate workspace
    for(std::vector<NXClassInfo>::const_iterator it=root.groups().begin();it!=root.groups().end();it++)
    if (it->nxclass == "NXentry")
    {

        std::cerr << "03a:" << std::endl; // REMOVE
        ++period;
        double progress_start = (period-1)*progress_step;
        double progress_end = progress_start + progress_step;
        std::cerr << "04:" << std::endl; // REMOVE
        Workspace_sptr ws = loadEntry(root.openEntry(it->nxname),period,progress_start,progress_end);
        std::cerr << "05:" << nPeriods << std::endl; // REMOVE
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
 *  @param progress_start The starting progress
 *  @param progress_end The ending progress
 *  @return A shared pointer to the created workspace
 */
API::Workspace_sptr LoadSNSNexus::loadEntry(NXEntry entry,int period, double progress_start, double progress_end)
{
    std::cerr << "loadEntry00:" << std::endl;// REMOVE
    // To keep sorted bank names
    std::set<std::string,CompareBanks> banks;
    std::set<std::string> monitors;
    std::cerr << "loadEntry01:" << std::endl;// REMOVE
    // Calculate the workspace dimensions
    int nSpectra = 0;
    int nBins = 0;
        std::cerr << "loadEntry02:" << std::endl;// REMOVE
    for(std::vector<NXClassInfo>::const_iterator it=entry.groups().begin();it!=entry.groups().end();it++)
    {
      if (it->nxclass == "NXdata") // Count detectors
      {    std::cerr << "loadEntry03:" << it->nxname << std::endl;// REMOVE
              NXData dataGroup = entry.openNXData(it->nxname);
              NXInt data = dataGroup.openIntData();
              if (data.rank() != 3) throw std::runtime_error("SNS NXdata is expected to be a 3D array");
              if (nBins == 0) nBins = data.dim2();
              nSpectra += data.dim0() * data.dim1();
              banks.insert(it->nxname); // sort the bank names
      }
      else if (it->nxclass == "NXmonitor") // Count monitors
      {    std::cerr << "loadEntry04:" << std::endl;// REMOVE
          nSpectra += 1;
          monitors.insert(it->nxname);
      }
    }

    // Create the output workspace
    std::cerr << "loadEntry05:" << std::endl;// REMOVE
    DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",nSpectra,nBins+1,nBins));
    ws->setTitle(entry.name());
    
    std::string run_num = entry.getString("run_number");
    //The run object is responsible for deleting the property
    ws->mutableRun().addLogData(new PropertyWithValue<std::string>("run_number", run_num));

    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    ws->setYUnit("Counts");

    // Init loaded spectra counter
    int spec = 0;

    // Load monitor readings
    std::cerr << "loadEntry06:" << std::endl;// REMOVE
    for(std::set<std::string>::const_iterator it=monitors.begin();it!=monitors.end();it++)
    {
        NXData dataGroup = entry.openNXData(*it);
        NXFloat timeBins = dataGroup.openNXFloat("time_of_flight");
        timeBins.load();
        MantidVec& X = ws->dataX(spec);
        X.assign(timeBins(),timeBins()+nBins+1);
        NXInt data = dataGroup.openIntData();
        data.load();
        MantidVec& Y = ws->dataY(spec);
        Y.assign(data(),data()+nBins);
        MantidVec& E = ws->dataE(spec);
        std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
        spec++;
    }
    std::cerr << "loadEntry07:" << std::endl;// REMOVE
    int specData = spec;

    // Load detector readings
    Progress progress(this,progress_start,progress_end,nSpectra);
    for(std::set<std::string,CompareBanks>::const_iterator it=banks.begin();it!=banks.end();it++)
    {
        NXData dataGroup = entry.openNXData(*it);
        if (spec == specData)
        {
            NXFloat timeBins = dataGroup.openNXFloat("time_of_flight");
            timeBins.load();
            MantidVec& X = ws->dataX(spec);
            X.assign(timeBins(),timeBins()+nBins+1);
        }
        NXInt data = dataGroup.openIntData();
    std::cerr << "loadEntry08:" << data.dim0() << ", " << data.dim1() << std::endl;// REMOVE
        for(int i = 0;i<data.dim0();i++)
            for(int j = 0; j < data.dim1(); j++)
            {

                if (spec > specData)
                {
                    MantidVec& newX = ws->dataX(spec);
                    MantidVec& oldX = ws->dataX(specData);
                    newX.assign(oldX.begin(),oldX.end());
                }
                data.load(1,i,j);
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
    std::cerr << "loadEntry09:" << std::endl;// REMOVE
    boost::shared_array<int> spectra(new int[nSpectra]);
    for(int i=0;i<nSpectra;i++)
    {
        spectra[i] = i + 1;
        ws->getAxis(1)->spectraNo(i)= i+1;
    }
    ws->mutableSpectraMap().populate(spectra.get(),spectra.get(),nSpectra);

    NXFloat proton_charge = entry.openNXFloat("proton_charge");
    proton_charge.load();
    //ws->getSample()->setProtonCharge(*proton_charge());
    ws->mutableRun().setProtonCharge(*proton_charge());
    std::cerr << "loadEntry11:" << std::endl;// REMOVE

    //---- Now load the instrument using the LoadInstrumentFromSNSNexus algorithm ----
    //Create the algorithm
    LoadInstrumentFromSNSNexus ld;
    ld.initialize();
    //Same filename
    ld.setPropertyValue("Filename", getPropertyValue("Filename"));
    //Point to the local workspace
    ld.setProperty("Workspace", boost::dynamic_pointer_cast<Workspace>(ws));
    //Execute the instrument loading.
    ld.execute();

    std::cerr << "loadEntry12:" << std::endl;// REMOVE
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
