// LoadNexusProcessed
// @author Ronald Fowler, based on SaveNexus
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadNexusProcessed.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidNexus/NexusFileIO.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "Poco/Path.h"
#include <cmath>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace NeXus
  {

    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadNexusProcessed)

    using namespace Kernel;
    using namespace API;
    using namespace DataObjects;

    /// Default constructor
    LoadNexusProcessed::LoadNexusProcessed() :
    Algorithm(),                                    //call the constructor for the base class
      nexusFile(new NexusFileIO()), m_filename(), m_entrynumber(0)
    {}

    /// Delete NexusFileIO in destructor
    LoadNexusProcessed::~LoadNexusProcessed()
    {
      delete nexusFile;
    }

    /** Initialisation method.
    *
    */
    void LoadNexusProcessed::init()
    {
      // Declare required input parameters for algorithm
      std::vector<std::string> exts;
      exts.push_back("NXS");
      exts.push_back("nxs");
      exts.push_back("nx5");
      exts.push_back("NX5");
      exts.push_back("xml");
      exts.push_back("XML");
      // required
      declareProperty("FileName", "", new FileValidator(exts),
        "Name of the Nexus file to read, as a full or relative path");

      declareProperty(new WorkspaceProperty<Workspace> ("OutputWorkspace", "",
        Direction::Output),
        "The name of the workspace to be created as the output of the\n"
        "algorithm. For multiperiod files, one workspace may be\n"
        "generated for each period. Currently only one workspace can\n"
        "be saved at a time so multiperiod Mantid files are not\n"
        "generated");


      // optional
      BoundedValidator<int> *mustBePositive = new BoundedValidator<int> ();
      mustBePositive->setLower(0);
      declareProperty("EntryNumber", 0, mustBePositive,
        "The particular entry number to read (default: the last entry)" );
    }

    /** Executes the algorithm. Reading in the file and creating and populating
    *  the output workspace
    *
    *  @throw runtime_error Thrown if algorithm cannot execute
    */
    void LoadNexusProcessed::exec()
    {
      // Retrieve the filename from the properties
      m_filename = getPropertyValue("FileName");
      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();
      boost::shared_ptr<Sample> sample;
      //
      m_entrynumber = getProperty("EntryNumber");
      double startProg=0.0;
      double endProg=0.0;
      double p=1.0;
      int numberOfPeriods=0;

      //period  used for the no.of workspaces in .nxs file
      //it's one   when an EntryNumber is given from UI
      // if no EntryNumber number given from UI it's equal to 
      //the number of workspaces in .nxs file
      int period=0;
      //open the  .nxs file
      NXRoot* nexusRoot=new NXRoot (m_filename);
      if(nexusRoot)
      {
        //get the no.of workspaces in .nxs files
        std::vector<NXClassInfo> grpVec=nexusRoot->groups();
        // get the workspace count from .nxs file
        if(!grpVec.empty())
          numberOfPeriods=period=grpVec.size();
		if(m_entrynumber>period)
        {
          throw std::invalid_argument("Invalid Entry Number:Enter a valid number");
        }
        delete nexusRoot;
      }
          // create ws group
      WorkspaceGroup_sptr wsGrpSptr=WorkspaceGroup_sptr(new WorkspaceGroup);

      //if  EntryNumber property is 0( default value ) create ws group and 
      //load all the workspaces from  .nxs file to workspace group if the number of workspaces >1
      if(m_entrynumber==0)
      {
        if(numberOfPeriods>1)
        {
          //add  outputworkspace to workspace group
          if(wsGrpSptr)wsGrpSptr->add(localWSName);
          setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(wsGrpSptr));
		  m_entrynumber=1;
        }

        p= double(1)/period;
      }
      else
      {
        numberOfPeriods=period=1;
      }

      //below do...while loop is introduced to handle workspace groups
      //if no EntryNumber given from UI loop is  executed period times
      // if an EntryNumber is given from UI it's executed once to select the given workspace no. from.nxs file
      do{
        endProg+=p;
        if (nexusFile->openNexusRead(m_filename, m_entrynumber) != 0)
        {
          g_log.error("Failed to read file " + m_filename);
          throw Exception::FileError("Failed to read to file", m_filename);
        }
        if (nexusFile->getWorkspaceSize(m_numberofspectra, m_numberofchannels, m_xpoints, m_uniformbounds,
          m_axes, m_yunits) != 0)
        {
          g_log.error("Failed to read data size");
          throw Exception::FileError("Failed to read data size", m_filename);
        }

        int total_specs = m_numberofspectra;

        // create output workspace of required size
        DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          WorkspaceFactory::Instance().create("Workspace2D", total_specs, m_xpoints, m_numberofchannels));
        // set first axis name
        const size_t colon = m_axes.find(":");
        if (colon != std::string::npos)
        {
          try
          {
            localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(m_axes.substr(0, colon));
          } catch (std::runtime_error&)
          { 
            g_log.warning("Unable to set Axis(0) units");
          }
          // Now set the unit on the vertical axis if not spectrum number
          try
          {
            // Will just throw exception if spectraNumber - caught below
            localWorkspace->getAxis(1)->unit() = UnitFactory::Instance().create(m_axes.substr(colon+1));
          } catch (std::runtime_error&)
          {
            g_log.debug() << "Unable to set Axis(1) to " << m_axes.substr(colon+1) << "\n";
          }
        }
        // set Yunits
        if (m_yunits.size() > 0)
          localWorkspace->setYUnit(m_yunits);

        Histogram1D::RCtype xValues;
        xValues.access() = localWorkspace->dataX(0);
        if (m_uniformbounds)
          nexusFile->getXValues(xValues.access(), 0);
        int counter = 0;
        API::Progress progress(this,startProg,endProg,period*m_numberofspectra);
        for (int i = 1; i <= m_numberofspectra; ++i)
        {
          MantidVec& values = localWorkspace->dataY(counter);
          MantidVec& errors = localWorkspace->dataE(counter);
          nexusFile->getSpectra(values, errors, i);
          if (!m_uniformbounds)
          {
            nexusFile->getXValues(xValues.access(), i - 1);
          }
          localWorkspace->setX(counter,xValues);
          ++counter;
          progress.report();
        }

        sample = localWorkspace->getSample();
        nexusFile->readNexusProcessedSample(sample);
        // Run the LoadIntsturment algorithm if name available
        m_instrumentName = nexusFile->readNexusInstrumentName();
        if ( ! m_instrumentName.empty() )
          runLoadInstrument(localWorkspace);
        else
          g_log.warning("No instrument file name found in the Nexus file");
        // get any spectraMap info
        boost::shared_ptr<IInstrument> localInstrument = localWorkspace->getInstrument();
        nexusFile->readNexusProcessedSpectraMap(localWorkspace);
        nexusFile->readNexusProcessedAxis(localWorkspace);
        // Assign the result to the output workspace property
        std::string outputWorkspace = "OutputWorkspace";
        nexusFile->readNexusParameterMap(localWorkspace);
        if(numberOfPeriods>1)
        {	
          std::stringstream suffix;
          suffix << (m_entrynumber);
          std::string outws =outputWorkspace+"_"+suffix.str();
          std::string WSName = localWSName + "_" + suffix.str();
          declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(outws,WSName,Direction::Output));
          if(wsGrpSptr)wsGrpSptr->add(WSName);
          setProperty(outws,boost::dynamic_pointer_cast<DataObjects::Workspace2D>(localWorkspace));
          ++m_entrynumber;
        }
        else
        {	
          setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(localWorkspace));
        }

        nexusFile->closeNexusFile();

		//it was giving  an error message from nexus "Cannot open group" if the entry number exceeds
		// number of periods,so resetting it to number of periods after execution
		if(m_entrynumber>numberOfPeriods)
			m_entrynumber=numberOfPeriods;

        loadAlgorithmHistory(localWorkspace);
        startProg=endProg;
        --period;

      }while(period!=0);


      return;
    }

    /** Run the sub-algorithm LoadInstrument (as for LoadRaw)
    *  @param localWorkspace The workspace to insert the instrument into
    */
    void LoadNexusProcessed::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
      if (directoryName.empty())
      {
        // This is the assumed deployment directory for IDFs, where we need to be relative to the
        // directory of the executable, not the current working directory.
        directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve(
          "../Instrument").toString();
      }

      // For Nexus Mantid processed, Instrument XML file name is read from nexus 
      std::string instrumentID = m_instrumentName;
      // force ID to upper case
      std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
      std::string fullPathIDF = directoryName + "/" + instrumentID + "_Definition.xml";

      IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");
      loadInst->setPropertyValue("Filename", fullPathIDF);
      loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->execute();
      } catch (std::runtime_error&)
      {
        g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
      }

    }

    void LoadNexusProcessed::loadAlgorithmHistory(DataObjects::Workspace2D_sptr localWorkspace)
    {
      NXRoot root(m_filename);
      std::ostringstream path;
      int entrynumber = m_entrynumber ? m_entrynumber : 1;
      path << "mantid_workspace_"<<entrynumber<<"/process";
      NXMainClass process = root.openNXClass<NXMainClass>(path.str());
      for(size_t i=0;i<process.groups().size();i++)
      {
        NXClassInfo inf = process.groups()[i];
        if (inf.nxname.substr(0,16) != "MantidAlgorithm_") continue;
        NXNote history = process.openNXNote(inf.nxname);
        std::vector< std::string >& hst = history.data();
        if (hst.size() == 0) continue;

        std::string name,vers,dummy;
        std::istringstream ianame(hst[0]);
        ianame >> dummy >> name >> vers;
        int version = atoi( vers.substr(1).c_str() );

        time_t tim = createTime_t_FromString(hst[1].substr(16,21));
        size_t ii = hst[2].find("sec");
        double dur = atof(hst[2].substr(20,ii-21).c_str());
        API::AlgorithmHistory ahist(name,version,tim,dur);

        for(size_t i=4;i<hst.size();i++)
        {
          std::string str = hst[i];
          std::string name,value,deflt,direction;
          size_t i0 = str.find("Name:");
          size_t i1 = str.find(", Value:",i0+1);
          size_t i2 = str.find(", Default?:",i1+1);
          size_t i3 = str.find(", Direction",i2+1);
          name = str.substr(i0+6,i1-i0-6);
          value = str.substr(i1+9,i2-i1-9);
          deflt = str.substr(i2+12,i3-i2-12);
          direction = str.substr(i3+13);
          ahist.addProperty(name,value,deflt == "Yes");
        }
        localWorkspace->history().addAlgorithmHistory(ahist);

      }
    }

    /** Create time_t value from a string
    *  @param str The string with date and time in format: YYYY-MMM-DD HH:MM:SS
    */
    std::time_t LoadNexusProcessed::createTime_t_FromString(const std::string &str)
    {

      std::map<std::string,int> Month;
      Month["Jan"] = 1;
      Month["Feb"] = 2;
      Month["Mar"] = 3;
      Month["Apr"] = 4;
      Month["May"] = 5;
      Month["Jun"] = 6;
      Month["Jul"] = 7;
      Month["Aug"] = 8;
      Month["Sep"] = 9;
      Month["Oct"] = 10;
      Month["Nov"] = 11;
      Month["Dec"] = 12;

      std::tm time_since_1900;
      time_since_1900.tm_isdst = -1;

      // create tm struct
      time_since_1900.tm_year = atoi(str.substr(0,4).c_str()) - 1900;
      std::string month = str.substr(5,3);

      time_since_1900.tm_mon = Month[str.substr(5,3)];
      time_since_1900.tm_mday = atoi(str.substr(9,2).c_str());
      time_since_1900.tm_hour = atoi(str.substr(12,2).c_str());
      time_since_1900.tm_min = atoi(str.substr(15,2).c_str());
      time_since_1900.tm_sec = atoi(str.substr(18,2).c_str());

      return std::mktime(&time_since_1900);
    }

  } // namespace NeXus
} // namespace Mantid
