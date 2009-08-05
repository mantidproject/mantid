//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadISISNexus2.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidGeometry/Detector.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/LogParser.h"
#include "MantidAPI/XMLlogfile.h"

#include "Poco/Path.h"

#include <cmath>
#include <sstream>
#include <cctype>

namespace Mantid
{
namespace NeXus
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadISISNexus2)

using namespace Kernel;
using namespace API;

/// Empty default constructor
LoadISISNexus2::LoadISISNexus2() : 
Algorithm(), m_filename(), m_numberOfSpectra(0), m_numberOfPeriods(0),
    m_list(false), m_interval(false), m_spec_list(), m_spec_min(0), m_spec_max(EMPTY_INT())
{}

/// Initialisation method.
void LoadISISNexus2::init()
{
    std::vector<std::string> exts;
    exts.push_back("NXS");
    exts.push_back("nxs");
    exts.push_back("s*");
    declareProperty("Filename","",new FileValidator(exts));
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("SpectrumMin", 0, mustBePositive);
    declareProperty("SpectrumMax", EMPTY_INT(), mustBePositive->clone());
    declareProperty(new ArrayProperty<int>("SpectrumList"));
	  declareProperty("EntryNumber", 0, mustBePositive->clone(),
    "The particular entry number to read (default: Load all workspaces and creates a workspace group)");
}

/** Executes the algorithm. Reading in the file and creating and populating
*  the output workspace
* 
*  @throw Exception::FileError If the Nexus file cannot be found/opened
*  @throw std::invalid_argument If the optional properties are set to invalid values
*/
void LoadISISNexus2::exec()
{
    // Create the root Nexus class
    NXRoot root(getPropertyValue("Filename"));

    // Open the raw data group 'raw_data_1'
    NXEntry entry = root.openEntry("raw_data_1");

    // Read in the instrument name from the Nexus file
    m_instrument_name = entry.getString("name");

	 // Retrieve the entry number
	 m_entrynumber = getProperty("EntryNumber");

    NXInt spectrum_index = entry.openNXInt("instrument/detector_1/spectrum_index");
    spectrum_index.load();
    NXInt udet = entry.openNXInt("isis_vms_compat/UDET");
    udet.load();
    boost::shared_array<int> spectra(new int[udet.dim0()]);
    
    NXData nxData = entry.openNXData("detector_1");
    NXInt data = nxData.openIntData();

    m_numberOfPeriods  = data.dim0();
    m_numberOfSpectra  = udet.dim0();
    m_numberOfChannels = data.dim2();

    std::vector<int> spec_list = getSpectraSelection();

    int nmon = 0;

    if (udet.dim0() != spectrum_index.dim0())
    {
        for(std::vector<NXClassInfo>::const_iterator it=entry.groups().begin();it!=entry.groups().end();it++)
            if (it->nxclass == "NXmonitor") // Count monitors
            {
                NXInt index = entry.openNXInt(std::string(it->nxname) + "/spectrum_index");
                index.load();
                if (spec_list.size() == 0 || std::find(spec_list.begin(),spec_list.end(),*index()) != spec_list.end())
                    m_monitors[*index()] = it->nxname;
                NXInt mon = entry.openNXInt(std::string(it->nxname) + "/monitor_number");
                mon.load();
                spectra[*index()-1] = *mon();
                nmon ++ ;
            }
    }
    for(int i=nmon;i<udet.dim0();i++)
        spectra[i] = i + 1;

	if(m_entrynumber!=0)
	{
		if(m_entrynumber>m_numberOfPeriods)
		{
			throw std::invalid_argument("Invalid Entry Number:Enter a valid number");
		}
		else
			m_numberOfPeriods=1;

	}
	

    if (spectrum_index.dim0() + nmon != udet.dim0() ||
        data.dim1() != spectrum_index.dim0())
        throw std::runtime_error("Spectra - detector mismatch");

    const int lengthIn = m_numberOfChannels + 1;

    // Need to extract the user-defined output workspace name
    Property *ws = getProperty("OutputWorkspace");
    std::string localWSName = ws->value();
	WorkspaceGroup_sptr wsGrpSptr=WorkspaceGroup_sptr(new WorkspaceGroup);
	if(m_numberOfPeriods>1)
	{	
		if(wsGrpSptr)wsGrpSptr->add(localWSName);
		setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(wsGrpSptr));
	}
    // If multiperiod, will need to hold the Instrument, Sample & SpectraDetectorMap for copying
    boost::shared_ptr<IInstrument> instrument;
    boost::shared_ptr<Sample> sample;

    NXFloat timeBins = nxData.openNXFloat("time_of_flight");
    timeBins.load();
    m_timeChannelsVec.reset(new MantidVec(timeBins(), timeBins() + m_numberOfChannels + 1));

    int total_specs = spec_list.size() > 0 ? spec_list.size() : m_numberOfSpectra;

    std::string outputWorkspace = "OutputWorkspace";

    // Create the 2D workspace for the output
    DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1));
    // Set the unit on the workspace to TOF
    localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    Progress prog(this,0.,1.,total_specs*m_numberOfPeriods);
    // Loop over the number of periods in the Nexus file, putting each period in a separate workspace
    for (int period = 0; period < m_numberOfPeriods; ++period) {
		
		if(m_entrynumber!=0)
		{
			period=m_entrynumber-1;
			if(period!=0)
			{
				// Only run the sub-algorithms once
				runLoadInstrument(localWorkspace );
				localWorkspace->mutableSpectraMap().populate(spectra.get(),udet(),udet.dim0());

				localWorkspace ->getSample()->setProtonCharge(entry.getFloat("proton_charge"));

				loadLogs(localWorkspace, entry);
			}
		}

        if (period == 0)
        {
            // Only run the sub-algorithms once
            runLoadInstrument(localWorkspace );
            localWorkspace->mutableSpectraMap().populate(spectra.get(),udet(),udet.dim0());

            localWorkspace ->getSample()->setProtonCharge(entry.getFloat("proton_charge"));

            loadLogs(localWorkspace, entry);
        }
        else   // We are working on a higher period of a multiperiod file
        {
            localWorkspace =  boost::dynamic_pointer_cast<DataObjects::Workspace2D>
                (WorkspaceFactory::Instance().create(localWorkspace));
            localWorkspace->newSample();
            localWorkspace->newInstrumentParameters();

            // Create a WorkspaceProperty for the new workspace of a higher period
            // The workspace name given in the OutputWorkspace property has _periodNumber appended to it
            //                (for all but the first period, which has no suffix)
            /*std::stringstream suffix;
            suffix << (period+1);
            outputWorkspace += suffix.str();
            std::string WSName = localWSName + "_" + suffix.str();
            declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(outputWorkspace,WSName,Direction::Output));
            g_log.information() << "Workspace " << WSName << " created. \n";*/
            loadLogs(localWorkspace,entry ,period+1);
        }

        data.open();
        for (int i = m_monitors.size(); i < total_specs; ++i)
        {
            int j = spec_list.size() > 0 ? spec_list[i] : i;
            data.load(period,j - nmon);
            MantidVec& Y = localWorkspace->dataY(i);
            Y.assign(data(),data()+m_numberOfChannels);
            MantidVec& E = localWorkspace->dataE(i);
            std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
            localWorkspace->getAxis(1)->spectraNo(i)= j + 1;
            localWorkspace->dataX(i) = *m_timeChannelsVec;
            prog.report();
        }

        int spec = 0;
        for(std::map<int,std::string>::const_iterator it=m_monitors.begin();it!=m_monitors.end();it++)
        {
            NXData monitor = entry.openNXData(it->second);
            NXFloat data = monitor.openFloatData();
            data.load(period,0);

            MantidVec& Y = localWorkspace->dataY(spec);
            Y.assign(data(),data()+m_numberOfChannels);
            MantidVec& E = localWorkspace->dataE(spec);
            std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
            localWorkspace->getAxis(1)->spectraNo(spec)= it->first;

            NXFloat timeBins = monitor.openNXFloat("time_of_flight");
            timeBins.load();
            localWorkspace->dataX(spec).assign(timeBins(),timeBins()+timeBins.dim0());
            spec++;
            prog.report();
        }

        // Assign the result to the output workspace property
       // setProperty(outputWorkspace,localWorkspace);
		std::string outws("");
		std::string outputWorkspace = "OutputWorkspace";
		if(m_numberOfPeriods>1)
		{

			std::stringstream suffix;
			suffix << (period+1);
			outws =outputWorkspace+"_"+suffix.str();
			std::string WSName = localWSName + "_" + suffix.str();
			declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(outws,WSName,Direction::Output));
			if(wsGrpSptr)wsGrpSptr->add(WSName);
			setProperty(outws,localWorkspace);
		}
		else
		{
			setProperty(outputWorkspace,boost::dynamic_pointer_cast<Workspace>(localWorkspace));
		}


    }

}

/// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromNexus)
void LoadISISNexus2::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
{
    // Determine the search directory for XML instrument definition files (IDFs)
    std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");      
    if ( directoryName.empty() )
    {
        // This is the assumed deployment directory for IDFs, where we need to be relative to the
        // directory of the executable, not the current working directory.
        directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve("../Instrument").toString();  
    }
    //const int stripPath = m_filename.find_last_of("\\/");
    // For Nexus, Instrument name given by MuonNexusReader from Nexus file
    std::string instrumentID = m_instrument_name; //m_filename.substr(stripPath+1,3);  // get the 1st 3 letters of filename part
    // force ID to upper case
    std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
    std::string fullPathIDF = directoryName + "/" + instrumentID + "_Definition.xml";

    IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");
    loadInst->setPropertyValue("Filename", fullPathIDF);
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);

    // Now execute the sub-algorithm. Catch and log any error, but don't stop.
    try
    {
        loadInst->execute();
    }
    catch (std::runtime_error&)
    {
        g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
    }

    // If loading instrument definition file fails, run LoadInstrumentFromNexus instead
    // This does not work at present as the example files do not hold the necessary data
    // but is a place holder. Hopefully the new version of Nexus Muon files should be more
    // complete.
    //if ( ! loadInst->isExecuted() )
    //{
    //    runLoadInstrumentFromNexus(localWorkspace);
    //}
}


/**  Load logs from Nexus file. Logs are expected to be in
 *   /raw_data_1/runlog group of the file. Call to this method must be done
 *   within /raw_data_1 group.
 *   @param ws The workspace to load the logs to.
 *   @param entry The Nexus entry
 *   @param period The period of this workspace
 */
void LoadISISNexus2::loadLogs(DataObjects::Workspace2D_sptr ws, NXEntry entry,int period)
{

    std::string stime = entry.getString("start_time");

    NXMainClass runlogs = entry.openNXClass<NXMainClass>("runlog");

    for(std::vector<NXClassInfo>::const_iterator it=runlogs.groups().begin();it!=runlogs.groups().end();it++)
    if (it->nxclass == "NXlog")
    {
        NXLog nxLog = runlogs.openNXLog(it->nxname);
        Kernel::Property* logv = nxLog.createTimeSeries();
        if (!logv) continue;
        ws->getSample()->addLogData(logv);
        if (it->nxname == "icp_event")
        {
            LogParser parser(logv);
            ws->getSample()->addLogData(parser.createPeriodLog(period));
            ws->getSample()->addLogData(parser.createAllPeriodsLog());
            ws->getSample()->addLogData(parser.createRunningLog());
        }
    }

    ws->populateInstrumentParameters();
}

/** Creates a list of selected spectra to load from input interval and list properties.
 *  @return An integer vector with spectra numbers to load. If an empty vector is returned
 *  load all spectra in the file.
 */
std::vector<int> LoadISISNexus2::getSpectraSelection()
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
        if ( spec_max < spec_min || spec_min >= m_numberOfSpectra || spec_max >= m_numberOfSpectra)
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
                if (s >= m_numberOfSpectra)
                {
                    g_log.error("Invalid Spectrum list property");
                    throw std::invalid_argument("Inconsistent properties defined");
                }
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

double LoadISISNexus2::dblSqrt(double in)
{
    return sqrt(in);
}

} // namespace DataHandling
} // namespace Mantid
