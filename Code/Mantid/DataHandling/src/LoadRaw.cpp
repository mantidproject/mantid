//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include <cmath>
#include <boost/shared_ptr.hpp>
#include "LoadRaw/isisraw.h"

namespace Mantid
{
  namespace DataHandling
  {

    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadRaw)

    using namespace Kernel;
    using namespace API;
    using namespace DataObjects;

    namespace fs = boost::filesystem; // to help clarify which bits are boost in code below

    Logger& LoadRaw::g_log = Logger::get("LoadRaw");

    /// Empty default constructor
    LoadRaw::LoadRaw()
    {
    }

    /** Initialisation method.
    * 
    */
    void LoadRaw::init()
    {
      declareProperty("Filename","",new MandatoryValidator);
      declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace","",Direction::Output));
      declareProperty("spectrum_min",0);
      declareProperty("spectrum_max",0);
      std::vector<int> sp_list(1,0);
      declareProperty("spectrum_list",sp_list);
      //    declareProperty(new ArrayProperty<int>("spec_list"));

    }

    /** Executes the algorithm. Reading in the file and creating and populating
    *  the output workspace
    * 
    *  @throw runtime_error Thrown if algorithm cannot execute
    */
    void LoadRaw::exec()
    {
      // Retrieve the filename from the properties

      m_filename = getPropertyValue("Filename");
      std::vector<int> spec_list=getProperty("spectrum_list");
      int spec_min = getProperty("spectrum_min");
      int spec_max = getProperty("spectrum_max");

      bool interval = false, list = false;

      ISISRAW iraw(NULL);
      if (iraw.readFromFile(m_filename.c_str()) != 0)
      {
        g_log.error("Unable to open file " + m_filename);
        throw Exception::FileError("Unable to open File:" , m_filename);	  
      }

      // Read the number of time channels from the RAW file 
      int channelsPerSpectrum, lengthIn, lengthOut;
      lengthIn = lengthOut = 1;
      channelsPerSpectrum = iraw.t_ntc1;

      // Read in the number of spectra in the RAW file
      int numberOfSpectra = iraw.t_nsp1;

      int minlist = *min_element(spec_list.begin(),spec_list.end());
      if(minlist == 0)
      {
        if (spec_list.size()!=1)
        {
          g_log.error("Invalid list of spectra");
          throw std::invalid_argument("Inconsistent properties defined"); 
        } 
        else
          list=false;
      }
      else
      {
        int maxlist = *max_element(spec_list.begin(),spec_list.end());
        if(maxlist!=0)
        {
          if(maxlist > numberOfSpectra)
          { 
            g_log.error("Invalid list of spectra");
            throw std::invalid_argument("Inconsistent properties defined"); 
          } 
          else
            list=true;
        }
      }

      if((spec_min==0 && spec_max != 0) || (spec_min!=0 && spec_max == 0)
        || spec_max < spec_min || spec_max > numberOfSpectra)
      {
        g_log.error("Invalid Spectrum min/max properties");
        throw std::invalid_argument("Inconsistent properties defined"); 
      }
      else
      {
        if( spec_min!=0 && spec_max != 0)
          if(spec_min!=spec_max)
            interval=true;
          else
          {
            g_log.error("Invalid Spectrum min/max properties");
            throw std::invalid_argument("Inconsistent properties defined"); 
          }
      }

      // Read in the time bin boundaries 
      lengthIn = channelsPerSpectrum + 1;    
      float* timeChannels = new float[lengthIn];
      iraw.getTimeChannels(timeChannels, lengthIn);
      // Put the read in array into a vector (inside a shared pointer)
      boost::shared_ptr<std::vector<double> > timeChannelsVec
        (new std::vector<double>(timeChannels, timeChannels + lengthIn));

      int* spectrum = new int[lengthIn];

      if( interval || list)
      {
        int total_specs;
        if(interval && list)
          total_specs=(spec_max-spec_min+1)+spec_list.size();
        if(interval && !list)
          total_specs=(spec_max-spec_min+1);
        if(!interval && list)
          total_specs=spec_list.size();

        // Create the 2D workspace for the output
        m_localWorkspace = boost::dynamic_pointer_cast<Workspace2D>
          (API::WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1));
        int counter=0;
        if(interval)
        {
          int i=spec_min;
          for( ;i <= spec_max;i++)
          {          
            loadData(timeChannelsVec,counter,i,iraw,lengthIn,spectrum);
            counter++;
          }
        }
        if(list)
        {
          for(unsigned int i=0; i < spec_list.size();i++)
          {
            loadData(timeChannelsVec,counter,spec_list[i],iraw,lengthIn,spectrum);
            counter++;
          }
        }
      }
      else
      {
        // Create the 2D workspace for the output
        m_localWorkspace = boost::dynamic_pointer_cast<Workspace2D>
          (API::WorkspaceFactory::Instance().create("Workspace2D",numberOfSpectra,lengthIn,lengthIn-1));

        // Loop over the spectra. Zeroth spectrum is garbage, so loop runs from 1 to NSP1
        for (int i = 1; i <= numberOfSpectra; i++)        
          loadData(timeChannelsVec,i-1,i,iraw,lengthIn,spectrum);        
      }
      // Assign the result to the output workspace property
      setProperty("OutputWorkspace",m_localWorkspace);

      // Clean up
      delete[] timeChannels;
      delete[] spectrum;

      // Set the unit on the workspace to TOF
      m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
      
      // Run the sub-algorithms (LoadInstrument & LoadRaw)
      runSubAlgorithms();

      /// @todo Need to deal with the connection between spectum number and detector
      
      return;
    }

    /** Read in a single spectrum from the raw file
     *  @param tcbs     The vector containing the time bin boundaries
     *  @param hist     The workspace index
     *  @param i        The spectrum number
     *  @param iraw     A reference to the ISISRAW object
     *  @param lengthIn The number of elements in a spectrum
     *  @param spectrum Pointer to the array into which the spectrum will be read
     */
    void LoadRaw::loadData(const DataObjects::Histogram1D::RCtype::ptr_type& tcbs,int hist, int& i, ISISRAW& iraw, int& lengthIn, int* spectrum)
    {
      // m_localWorkspace is a private class variable and does not need to be declared anywhere

      // Read in a spectrum
      memcpy(spectrum, iraw.dat1 + i * lengthIn, lengthIn * sizeof(int));
      // Put it into a vector, discarding the 1st entry, which is rubbish
      // But note that the last (overflow) bin is kept
      std::vector<double> v(spectrum + 1, spectrum + lengthIn);
      // Create and fill another vector for the errors, containing sqrt(count)
      std::vector<double> e(lengthIn-1);
      std::transform(v.begin(), v.end(), e.begin(), dblSqrt);
      // Populate the workspace. Loop starts from 1, hence i-1
      m_localWorkspace->setData(hist, v, e);
      m_localWorkspace->setX(hist, tcbs);
      m_localWorkspace->setErrorHelper(hist,GaussianErrorHelper::Instance());
      m_localWorkspace->getAxis(1)->spectraNo(hist)= i;
      // NOTE: Raw numbers go straight into the workspace 
      //     - no account taken of bin widths/units etc.
    }

    /// Run the sub-algorithms: LoadInstrument (or LoadInstrumentFromRaw) and LoadLog
    void LoadRaw::runSubAlgorithms()
    {
      // First deal with loading of instrument definition information

      // Determine the search directory for XML instrument definition files (IDFs)

      std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
      
      if ( directoryName.empty() )
        directoryName = "../Instrument";  // This is the assumed deployment directory for IDFs

      // Determine the name of IDF to search for.
      // Get hold of the 1st 3 letters of the raw file. Use only these three letter to construct the
      // IDF filename to search for

      fs::path l_path( m_filename );
      std::string instrumentID = l_path.leaf().substr(0,3);  // get the 1st 3 letters of filename part

      // force ID to upper case

      for (unsigned int i = 0; i < instrumentID.size(); i++) 
        instrumentID[i] = toupper(instrumentID[i]); 

      std::string fullPathIDF1 = directoryName + "/" + instrumentID + "_Definition.xml";
      std::string fullPathIDF2 = directoryName + "/" + instrumentID + "_definition.xml"; // to allow lower case


      // If IDF present in search directory run LoadInstrument otherwise LoadInstrumentFromRaw

      fs::path l_pathIDF1( fullPathIDF1 );  // create boost path to do the check below
      fs::path l_pathIDF2( fullPathIDF2 );  // lower case 

      if ( fs::exists( l_pathIDF1 ) || fs::exists( l_pathIDF2 ) )
      {
        Algorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");

        if ( fs::exists( l_pathIDF1 ) )
          loadInst->setPropertyValue("Filename", fullPathIDF1);
        else
          loadInst->setPropertyValue("Filename", fullPathIDF2);

        // Set the workspace property to be the same one filled above
        loadInst->setProperty<Workspace_sptr>("Workspace",m_localWorkspace);

        // Now execute the sub-algorithm. Catch and log any error, but don't stop.
        try
        {
          loadInst->execute();
        }
        catch (std::runtime_error& err)
        {
          g_log.error("Unable to successfully run LoadInstrument sub-algorithm");
        }

        if ( ! loadInst->isExecuted() )
        {
          g_log.error("Unable to successfully run LoadInstrument sub-algorithm");
        }

      }
      else
      {
        g_log.information() << "Instrument definition file not found. Attempt to load information about \n"
          << "the instrument from raw data file.\n";

        Algorithm_sptr loadInst = createSubAlgorithm("LoadInstrumentFromRaw");
        loadInst->setPropertyValue("Filename", m_filename);

        // Set the workspace property to be the same one filled above
        loadInst->setProperty<Workspace_sptr>("Workspace",m_localWorkspace);

        // Now execute the sub-algorithm. Catch and log any error, but don't stop.
        try
        {
          loadInst->execute();
        }
        catch (std::runtime_error& err)
        {
          g_log.error("Unable to successfully run LoadInstrumentFromRaw sub-algorithm");
        }

        if ( ! loadInst->isExecuted() )
        {
          g_log.error("Unable to successfully run LoadInstrumentFromRaw sub-algorithm");
        }
      }
      
      // Now determine the spectra to detector map calling sub-algoorithm LoadMappingTable
      // There is a small penalty in re-opening the raw file but nothing major. 
      Algorithm_sptr loadmap= createSubAlgorithm("LoadMappingTable");
      loadmap->setPropertyValue("Filename", m_filename);
      loadmap->setProperty<Workspace_sptr>("Workspace",m_localWorkspace);
      try
      {
    	loadmap->execute();  
      }catch (std::runtime_error& err)
      {
    	  g_log.error("Unable to successfully execute LoadMappingTable sub-algorithm");
      }
      
      if ( ! loadmap->isExecuted() )
      {
          g_log.error("LoadMappingTable sub-algorithm is not executed");
       }


      // Now do LoadLog
      Algorithm_sptr loadLog = createSubAlgorithm("LoadLog");
      // Pass through the same input filename
      loadLog->setPropertyValue("Filename",m_filename);
      // Set the workspace property to be the same one filled above
      loadLog->setProperty<Workspace_sptr>("Workspace",m_localWorkspace);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadLog->execute();
      }
      catch (std::runtime_error& err)
      {
        g_log.error("Unable to successfully run LoadLog sub-algorithm");
      }

      if ( ! loadLog->isExecuted() )
      {
        g_log.error("Unable to successfully run LoadLog sub-algorithm");
      }

    }

    double LoadRaw::dblSqrt(double in)
    {
      return sqrt(in);
    }

  } // namespace DataHandling
} // namespace Mantid
