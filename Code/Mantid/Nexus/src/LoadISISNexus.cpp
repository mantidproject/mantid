//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadISISNexus.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidGeometry/Detector.h"
#include "MantidAPI/SpectraDetectorMap.h"

#include "Poco/Path.h"

#include <cmath>
#include <sstream>

namespace Mantid
{
namespace NeXus
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadISISNexus)

using namespace Kernel;
using namespace API;

// Initialise the logger
Logger& LoadISISNexus::g_log = Logger::get("LoadISISNexus");

/// Empty default constructor
LoadISISNexus::LoadISISNexus() : 
Algorithm(), m_filename(), m_numberOfSpectra(0), m_numberOfPeriods(0),
    m_list(false), m_interval(false), m_spec_list(), m_spec_min(0), m_spec_max(0)
{}

/// Initialisation method.
void LoadISISNexus::init()
{
    std::vector<std::string> exts;
    exts.push_back("NXS");
    exts.push_back("nxs");
    declareProperty("Filename","",new FileValidator(exts));
    declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output));

    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("spectrum_min",0, mustBePositive);
    declareProperty("spectrum_max",0, mustBePositive->clone());
    declareProperty(new ArrayProperty<int>("spectrum_list"));
}

/** Executes the algorithm. Reading in the file and creating and populating
*  the output workspace
* 
*  @throw Exception::FileError If the Nexus file cannot be found/opened
*  @throw std::invalid_argument If the optional properties are set to invalid values
*/
void LoadISISNexus::exec()
{
    // Retrieve the filename from the properties
    m_filename = getPropertyValue("Filename");

    // Open NeXus file
    NXstatus stat=NXopen(m_filename.c_str(), NXACC_READ, &m_fileID);
    if(stat==NX_ERROR)
    {
        g_log.error("Unable to open File: " + m_filename);  
        throw Exception::FileError("Unable to open File:" , m_filename);  
    }

    // Open the raw data group 'raw_data_1'
    openNexusGroup("raw_data_1","NXentry");

    // Read in the instrument name from the Nexus file
    m_instrument_name = getNexusString("name");

    openNexusGroup("detector_1","NXdata");

    readDataDimensions();

    const int lengthIn = m_numberOfChannels + 1;

    // Need to extract the user-defined output workspace name
    Property *ws = getProperty("OutputWorkspace");
    std::string localWSName = ws->value();
    // If multiperiod, will need to hold the Instrument, Sample & SpectraDetectorMap for copying
    boost::shared_ptr<IInstrument> instrument;
    boost::shared_ptr<Sample> sample;

    // Call private method to validate the optional parameters, if set
    checkOptionalProperties();

    getTimeChannels();

    closeNexusGroup();// go back to raw_data_1

    // Calculate the size of a workspace, given its number of periods & spectra to read
    int total_specs;
    if( m_interval || m_list)
    {
        total_specs = m_spec_list.size();
        if (m_interval)
        {
            total_specs += (m_spec_max-m_spec_min+1);
            m_spec_max += 1;
        }
    }
    else
    {
        total_specs = m_numberOfSpectra;
        // for nexus return all spectra
        m_spec_min = 0; // changed to 0 for NeXus, was 1 for Raw
        m_spec_max = m_numberOfSpectra;  // was +1?
    }

    std::string outputWorkspace = "OutputWorkspace";

    // Create the 2D workspace for the output
    DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1));
    // Set the unit on the workspace to TOF
    localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");



    // Loop over the number of periods in the Nexus file, putting each period in a separate workspace
    for (int period = 0; period < m_numberOfPeriods; ++period) {

        openNexusGroup("detector_1","NXdata");

        if ( period > 0 )
        {
            localWorkspace =  boost::dynamic_pointer_cast<DataObjects::Workspace2D>
                (WorkspaceFactory::Instance().create(localWorkspace));
            localWorkspace->newSample();
            localWorkspace->newInstrumentParameters();
        }

        int counter = 0;
        for (int i = m_spec_min; i < m_spec_max; ++i)
        {
            loadData(period, counter,i,localWorkspace ); // added -1 for NeXus
            counter++;
        }
        // Read in the spectra in the optional list parameter, if set
        if (m_list)
        {
            for(unsigned int i=0; i < m_spec_list.size(); ++i)
            {
                loadData(period, counter,m_spec_list[i],localWorkspace );
                counter++;
            }
        }

        closeNexusGroup();// go up from detector_1 to raw_data_1

        // Just a sanity check
        assert(counter == total_specs);

        std::string outputWorkspace = "OutputWorkspace";
        if (period == 0)
        {
            // Only run the sub-algorithms once
            runLoadInstrument(localWorkspace );
            loadMappingTable(localWorkspace );
            loadProtonCharge(localWorkspace);
//            runLoadLog(localWorkspace );
        }
        else   // We are working on a higher period of a multiperiod raw file
        {
            // Create a WorkspaceProperty for the new workspace of a higher period
            // The workspace name given in the OutputWorkspace property has _periodNumber appended to it
            //                (for all but the first period, which has no suffix)
            std::stringstream suffix;
            suffix << (period+1);
            outputWorkspace += suffix.str();
            std::string WSName = localWSName + "_" + suffix.str();
            declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(outputWorkspace,WSName,Direction::Output));
            g_log.information() << "Workspace " << WSName << " created. \n";
//            runLoadLog(localWorkspace );
        }

        // Assign the result to the output workspace property
        setProperty(outputWorkspace,localWorkspace);

    }

    // Close the Nexus file
    NXclose(&m_fileID);
}

/// Validates the optional 'spectra to read' properties, if they have been set
void LoadISISNexus::checkOptionalProperties()
{
    Property *specList = getProperty("spectrum_list");
    m_list = !(specList->isDefault());
    Property *specMax = getProperty("spectrum_max");
    m_interval = !(specMax->isDefault());

    // If a multiperiod dataset, ignore the optional parameters (if set) and print a warning
    if ( m_numberOfPeriods > 1)
    {
        if ( m_list || m_interval )
        {
            m_list = false;
            m_interval = false;
            g_log.warning("Ignoring spectrum properties in this multiperiod dataset");
        }
    }

    // Check validity of spectra list property, if set
    if ( m_list )
    {
        m_list = true;
        m_spec_list = getProperty("spectrum_list");
        const int minlist = *min_element(m_spec_list.begin(),m_spec_list.end());
        const int maxlist = *max_element(m_spec_list.begin(),m_spec_list.end());
        if ( maxlist > m_numberOfSpectra || minlist == 0)
        {
            g_log.error("Invalid list of spectra");
            throw std::invalid_argument("Inconsistent properties defined"); 
        } 
    }

    // Check validity of spectra range, if set
    if ( m_interval )
    {
        m_interval = true;
        m_spec_min = getProperty("spectrum_min");
        m_spec_max = getProperty("spectrum_max");
        if ( m_spec_max < m_spec_min || m_spec_max > m_numberOfSpectra )
        {
            g_log.error("Invalid Spectrum min/max properties");
            throw std::invalid_argument("Inconsistent properties defined"); 
        }
    }
}

/** Reads in a string value from the nexus file
 *  @param name The name of the data value in the nexus file. The name can be either absolute or relative
 *  @return The string's value
 *  @throw std::runtime_error in case of any error
 */
std::string LoadISISNexus::getNexusString(const std::string& name)const
{
    NXstatus stat=NXopendata(m_fileID,name.c_str());
    if(stat==NX_ERROR)
    {
        g_log.error("Cannot access data "+name);
        throw std::runtime_error("Cannot access data "+name);
    }

    int rank,dims[4],type;
    NXgetinfo(m_fileID,&rank,dims,&type);

    if (dims[0] == 0) return "";
    if (type != NX_CHAR)
    {
        g_log.error("Data "+name+" does not have NX_CHAR type as expected.");
        throw std::runtime_error("Data "+name+" does not have NX_CHAR type as expected.");
    }
    if (rank > 1) 
    {
        std::ostringstream ostr; ostr<<"Rank of data "<<name<<" is expected to be 2 ("<<rank<<" was found)";
        g_log.error(ostr.str());
        throw std::runtime_error(ostr.str());
    }

    boost::shared_array<char> buff( new char[dims[0]+1] );
    if (NX_ERROR == NXgetdata(m_fileID,buff.get()))
    {
        g_log.error("Error occured when reading data "+name);
        throw std::runtime_error("Error occured when reading data "+name);
    }
    buff[dims[0]]='\0'; // null terminate for copy
    stat=NXclosedata(m_fileID);

    return std::string( buff.get() );
}

/** Opens a Nexus group. This makes it the working group.
 *  @param name The group's name
 *  @param nx_class Nexus class of the group
 *  @throw std::runtime_error if the group cannot be opened
 */
void LoadISISNexus::openNexusGroup(const std::string& name, const std::string& nx_class)const
{
    if (NX_ERROR == NXopengroup(m_fileID,name.c_str(),nx_class.c_str())) 
    {
        g_log.error("Cannot open group "+name+" of class "+nx_class);
        throw std::runtime_error("Cannot open group "+name+" of class "+nx_class);
    }

    //boost::shared_array<char> nxname( new char[NX_MAXNAMELEN] );
    //boost::shared_array<char> nxclass( new char[NX_MAXNAMELEN] );
    //int nxdatatype;

    //int stat = NX_OK;
    //while(stat != NX_EOD)
    //{
    //    stat=NXgetnextentry(m_fileID,nxname,nxclass,&nxdatatype);
    //    std::cerr<<nxname<<"  "<<nxclass<<"  "<<nxdatatype<<"\n";
    //}
    //std::cerr<<'\n';
}

/// Closes Nexus group
void LoadISISNexus::closeNexusGroup()const
{
    NXclosegroup(m_fileID);
}

/** Group /raw_data_1/detector_1 must be open to call this function
 */
void LoadISISNexus::readDataDimensions()
{
    if (NX_ERROR == NXopendata(m_fileID,"counts"))
    {
        g_log.error("Cannot open data counts");
        throw std::runtime_error("Cannot open data counts");
    };
    int rank,type,dims[4];
    NXgetinfo(m_fileID,&rank,dims,&type);

    // Number of time channels and number of spectra made public
    m_numberOfPeriods  = dims[0];
    m_numberOfSpectra  = dims[1];
    m_numberOfChannels = dims[2];

    // Allocate memory for the counts buffer
    m_data.reset( new int[m_numberOfChannels] );

    NXclosedata(m_fileID);
}

void LoadISISNexus::getTimeChannels()
{
    boost::shared_array<float> timeChannels( new float[m_numberOfChannels + 1] );

    if (NX_ERROR == NXopendata(m_fileID,"time_of_flight"))
    {
        g_log.error("Cannot open data time_of_flight");
        throw std::runtime_error("Cannot open data time_of_flight");
    };
    int rank,type,dims[4];
    NXgetinfo(m_fileID,&rank,dims,&type);

    if (m_numberOfChannels + 1 != dims[0])
    {
        g_log.error("Number of time channels does not match the data dimensions");
        throw std::runtime_error("Number of time channels does not match the data dimensions");
    }

    if (NX_ERROR == NXgetdata(m_fileID,timeChannels.get()))
    {
        g_log.error("Error reading time_of_flight");
        throw std::runtime_error("Error reading time_of_flight");
    };

    m_timeChannelsVec.reset(new MantidVec(timeChannels.get(), timeChannels.get() + m_numberOfChannels + 1));

    NXclosedata(m_fileID);
}

/** Group /raw_data_1/detector_1 must be open to call this function
 */
void LoadISISNexus::loadData(int period, int hist, int& i, DataObjects::Workspace2D_sptr localWorkspace)
{
    if (NX_ERROR == NXopendata(m_fileID,"counts"))
    {
        g_log.error("Cannot open data counts");
        throw std::runtime_error("Cannot open data counts");
    };

    int rank,type,dims[4];
    NXgetinfo(m_fileID,&rank,dims,&type);

    int start[3], size[3];
    start[0] = period;
    start[1] = hist;
    start[2] = 0;

    size[0] = 1;
    size[1] = 1;
    size[2] = m_numberOfChannels;

    if (NX_ERROR == NXgetslab(m_fileID, m_data.get(), start, size))
    {
        g_log.error("Error reading counts");
        throw std::runtime_error("Error reading counts");
    };

    MantidVec& Y = localWorkspace->dataY(hist);
    Y.assign(m_data.get(), m_data.get() + m_numberOfChannels);
    // Create and fill another vector for the errors, containing sqrt(count)
    MantidVec& E = localWorkspace->dataE(hist);
    std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
    // Populate the workspace. Loop starts from 1, hence i-1
    localWorkspace->setX(hist, m_timeChannelsVec);

    NXclosedata(m_fileID);
}

/// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromNexus)
void LoadISISNexus::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
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
    catch (std::runtime_error& err)
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

/**  Populate the ws's spectra-detector map. The Nexus file must be in /raw_data_1 group.
 *   After finishing the file remains in the same group (/raw_data_1 group is open).
 *   @param ws The workspace which spectra-detector map is to be populated.
 */
void LoadISISNexus::loadMappingTable(DataObjects::Workspace2D_sptr ws)
{
    // Read in detector ids from isis compatibility section
    openNexusGroup("isis_vms_compat","IXvms");

    if (NX_ERROR == NXopendata(m_fileID,"UDET"))
    {
        g_log.error("Cannot open UDET");
        throw std::runtime_error("Cannot open UDET");
    };

    int rank,type,dims[4];
    NXgetinfo(m_fileID,&rank,dims,&type);

    int ndet = dims[0];

    boost::shared_array<int> udet(new int[ndet]);

    if (NX_ERROR == NXgetdata(m_fileID,udet.get()))
    {
        g_log.error("Error reading UDET");
        throw std::runtime_error("Error reading UDET");
    };

    NXclosedata(m_fileID);  // UDET
    closeNexusGroup(); // isis_vms_compat

    openNexusGroup("detector_1","NXdata");

    if (NX_ERROR == NXopendata(m_fileID,"spectrum_index"))
    {
        g_log.error("Cannot open spectrum_index");
        throw std::runtime_error("Cannot open spectrum_index");
    };

    NXgetinfo(m_fileID,&rank,dims,&type);

    if (dims[0] != ndet)
    {
        g_log.error("Cannot open load spectra-detector map: data sizes do not match.");
        throw std::runtime_error("Cannot open load spectra-detector map: data sizes do not match.");
    }

    boost::shared_array<int> spec(new int[ndet]);

    if (NX_ERROR == NXgetdata(m_fileID,spec.get()))
    {
        g_log.error("Error reading spectrum_index");
        throw std::runtime_error("Error reading spectrum_index");
    };

    NXclosedata(m_fileID); // spectrum_index
    closeNexusGroup(); // detector_1

    for(int i=0;i<ndet;i++)
    {
        ws->getAxis(1)->spectraNo(i)= spec[i];
    }

    //Populate the Spectra Map with parameters
    ws->mutableSpectraMap().populate(spec.get(),udet.get(),ndet);

}

/**  Loag the proton charge from the file.
 *   Group /raw_data_1 must be open.
 */
void LoadISISNexus::loadProtonCharge(DataObjects::Workspace2D_sptr ws)
{
    if (NX_ERROR == NXopendata(m_fileID,"proton_charge"))
    {
        g_log.error("Cannot open proton_charge");
        throw std::runtime_error("Cannot open proton_charge");
    };

    if (NX_ERROR == NXgetdata(m_fileID,&m_proton_charge))
    {
        g_log.error("Error reading proton_charge");
        throw std::runtime_error("Error reading proton_charge");
    };

    ws->getSample()->setProtonCharge(m_proton_charge);

    NXclosedata(m_fileID);
}

double LoadISISNexus::dblSqrt(double in)
{
    return sqrt(in);
}

} // namespace DataHandling
} // namespace Mantid
