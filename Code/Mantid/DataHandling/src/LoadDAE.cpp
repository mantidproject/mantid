//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadDAE.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidAPI/SpectraDetectorMap.h"

#include <cmath>
#include <boost/shared_ptr.hpp>
#include "Poco/Path.h"

#include "LoadDAE/idc.h"

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadDAE)

    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& LoadDAE::g_log = Logger::get("LoadDAE");

    /// Empty default constructor
    LoadDAE::LoadDAE() :
      Algorithm(), m_daename(""), m_numberOfSpectra(0), m_numberOfPeriods(0),
      m_list(false), m_interval(false), m_spec_list(), m_spec_min(0), m_spec_max(0)
    {}


    /// load data from the DAE
    void LoadDAE::loadData(const DataObjects::Histogram1D::RCtype::ptr_type& tcbs,int hist, int& ispec, idc_handle_t dae_handle, const int& lengthIn,
    		int* spectrum, DataObjects::Workspace2D_sptr localWorkspace, int* allData)
    {
      int ndims, dims_array[1];
      ndims = 1;
      dims_array[0] = lengthIn;

      int *data = 0;

      if (allData) data = allData + ispec*lengthIn;
      else
      {
        // Read in spectrum number ispec from DAE
        if (IDCgetdat(dae_handle, ispec, 1, spectrum, dims_array, &ndims) != 0)
        {
          g_log.error("Unable to read DATA from DAE " + m_daename);
          throw Exception::FileError("Unable to read DATA from DAE " , m_daename);
        }
        data = spectrum;
      }

      // Put it into a vector, discarding the 1st entry, which is rubbish
      // But note that the last (overflow) bin is kept
      MantidVec& Y = localWorkspace->dataY(hist);
      Y.assign(data + 1, data + lengthIn);
      // Create and fill another vector for the errors, containing sqrt(count)
      MantidVec& E = localWorkspace->dataE(hist);
      std::transform(Y.begin(), Y.end(), E.begin(), LoadDAE::dblSqrt);
      // Populate the workspace. Loop starts from 1, hence i-1
      localWorkspace->setX(hist, tcbs);
      localWorkspace->getAxis(1)->spectraNo(hist)= ispec;
      // NOTE: Raw numbers go straight into the workspace
      //     - no account taken of bin widths/units etc.
    }

    /// Initialisation method.
    void LoadDAE::init()
    {
      declareProperty("DAEname","", new MandatoryValidator<std::string>(), "The name of and path to the input DAE host.");
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output), "The name of the workspace that will be created, filled with the read-in data and stored in the Analysis Data Service.\nIf the input data contain multiple periods higher periods will be stored in separate workspaces called OutputWorkspace_PeriodNo.");

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("spectrum_min",0, mustBePositive, "The number of the first spectrum to read.\nOnly used if spectrum_max is set.\nNot available for multiperiod data files.");
      declareProperty("spectrum_max",0, mustBePositive->clone(), "The number of the last spectrum to read.\nOnly used if explicitly set.\nNot available for multiperiod data files. ");
      declareProperty(new ArrayProperty<int>("spectrum_list"), "A comma-separated list of individual spectra to read.\nOnly used if explicitly set.\nNot available for multiperiod data files.");
    }

    /** Function called by IDC routines to report an error. Passes the error through to the logger
     * @param status  The status code of the error (disregarded)
     * @param code    The error code (disregarded)
     * @param message The error message - passed to the logger at error level
     */
    void LoadDAE::IDCReporter(int status, int code, const char* message)
    {
      g_log.error(message);
    }

    /** Executes the algorithm. Reading in the file and creating and populating
     *  the output workspace
     *
     *  @throw Exception::FileError If the DAE cannot be found/opened
     *  @throw std::invalid_argument If the optional properties are set to invalid values
     */
    void LoadDAE::exec()
    {
      int sv_dims_array[1] = { 1 }, sv_ndims = 1;   // used for rading single values with IDC routines
      int dims_array[2];
      // Retrieve the filename from the properties
      m_daename = getPropertyValue("DAEname");

      idc_handle_t dae_handle;

      // set IDC reporter function for errors
      IDCsetreportfunc(&LoadDAE::IDCReporter);

      if (IDCopen(m_daename.c_str(), 0, 0, &dae_handle) != 0)
      {
        g_log.error("Unable to open DAE " + m_daename);
        throw Exception::FileError("Unable to open DAE:" , m_daename);
      }

      // Read in the number of spectra in the DAE
      IDCgetpari(dae_handle, "NSP1", &m_numberOfSpectra, sv_dims_array, &sv_ndims);
      // Read the number of periods
      IDCgetpari(dae_handle, "NPER", &m_numberOfPeriods, sv_dims_array, &sv_ndims);
      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();

      // Call private method to validate the optional parameters, if set
      checkOptionalProperties();

      // Read the number of time channels (i.e. bins) from the RAW file
      if (IDCgetpari(dae_handle, "NTC1", &m_channelsPerSpectrum, sv_dims_array, &sv_ndims) != 0)
      {
        g_log.error("Unable to read NTC1 from DAE " + m_daename);
        throw Exception::FileError("Unable to read NTC1 from DAE " , m_daename);
      }

      // Read in the time bin boundaries
      const int lengthIn = m_channelsPerSpectrum + 1;
      boost::shared_ptr<float> timeChannels(new float[lengthIn]);

      dims_array[0] = lengthIn;
      if (IDCgetparr(dae_handle, "RTCB1", timeChannels.get(), dims_array, &sv_ndims) != 0)
      {
        g_log.error("Unable to read RTCB1 from DAE " + m_daename);
        throw Exception::FileError("Unable to read RTCB1 from DAE " , m_daename);
      }
      // Put the read in array into a vector (inside a shared pointer)
      boost::shared_ptr<std::vector<double> > timeChannelsVec
                          (new std::vector<double>(timeChannels.get(), timeChannels.get() + lengthIn));
      // Create an array to hold the read-in data
      boost::shared_ptr<int> spectrum(new int[lengthIn]);

      // Read the instrument name
      char* iName = 0;
      dims_array[0] = 4;
      if (IDCAgetparc(dae_handle, "NAME", &iName, dims_array, &sv_ndims) != 0)
      {
          g_log.error("Unable to read NAME from DAE " + m_daename);
          throw Exception::FileError("Unable to read NAME from DAE " , m_daename);
      };

      // Read the proton charge
      float rpb[32];
      dims_array[0] = 32;
      if (IDCgetparr(dae_handle, "RRPB", rpb, dims_array, &sv_ndims) != 0)
      {
          g_log.error("Unable to read RRPB from DAE " + m_daename);
          throw Exception::FileError("Unable to read RRPB from DAE " , m_daename);
      };
      m_proton_charge = rpb[8];

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
        // In this case want all the spectra, but zeroth spectrum is garbage so go from 1 to NSP1
        m_spec_min = 1;
        m_spec_max = m_numberOfSpectra + 1;
      }

      // Decide if we can read in all the data at once
      boost::shared_ptr<int> allData;
      int ndata = (m_numberOfSpectra+1)*(m_channelsPerSpectrum+1)*m_numberOfPeriods*4;
      if (ndata/1000000 < 10) // arbitrary number
      {
          dims_array[0] = ndata;
          allData.reset(new int[ ndata ]);
          // and read them in
          int ret = IDCgetpari(dae_handle, "CNT1", allData.get(), dims_array, &sv_ndims);
          if (ret < 0)
          {
              allData.reset();
              throw std::runtime_error("");
          }
      }

      // Create the 2D workspace for the output
      DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
               (WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1));
      // Set the unit on the workspace to TOF
      localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

      loadSpectraMap(dae_handle, localWorkspace);

      int histTotal = total_specs * m_numberOfPeriods;
      int histCurrent = -1;
      // Loop over the number of periods in the raw file, putting each period in a separate workspace
      for (int period = 0; period < m_numberOfPeriods; ++period) {

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
          // Shift the histogram to read if we're not in the first period
          int histToRead = i + period*total_specs;
          loadData(timeChannelsVec,counter,histToRead,dae_handle,lengthIn,spectrum.get(),localWorkspace,allData.get() );
          counter++;
          if (++histCurrent % 10 == 0) progress(double(histCurrent)/histTotal);
          interruption_point();
        }
        // Read in the spectra in the optional list parameter, if set
        if (m_list)
        {
          for(unsigned int i=0; i < m_spec_list.size(); ++i)
          {
            loadData(timeChannelsVec,counter,m_spec_list[i],dae_handle,lengthIn,spectrum.get(), localWorkspace,allData.get() );
            counter++;
            if (++histCurrent % 10 == 0) progress(double(histCurrent)/histTotal);
            interruption_point();
          }
        }

        // Just a sanity check
        assert(counter == total_specs);

        std::string outputWorkspace = "OutputWorkspace";
        if (period == 0)
        {
          // Only run the sub-algorithms once
          runLoadInstrument(localWorkspace, iName);
          //runLoadLog(localWorkspace );
          // Set the total proton charge for this run
          localWorkspace->getSample()->setProtonCharge(m_proton_charge);
        }
        if (period != 0)
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
        }

        // Assign the result to the output workspace property
        setProperty(outputWorkspace,localWorkspace);

      } // loop over periods

      if (IDCclose(&dae_handle) != 0)
      {
        g_log.error("Unable to close DAE " + m_daename);
        throw Exception::FileError("Unable to close DAE:" , m_daename);
      }
      // Clean up
      if (iName) delete[] iName;
    }

    /// Validates the optional 'spectra to read' properties, if they have been set
    void LoadDAE::checkOptionalProperties()
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


    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromRaw)
    void LoadDAE::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace, const char* iName)
    {
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
      if ( directoryName.empty() )
      {
          // This is the assumed deployment directory for IDFs, where we need to be relative to the
          // directory of the executable, not the current working directory.
          directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve("../Instrument").toString();
      }

      std::string instrumentID = iName; // get the instrument name
      size_t i = instrumentID.find_first_of(' '); // cut trailing spaces
      if (i != std::string::npos) instrumentID.erase(i);
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

      // If loading instrument definition file fails, run LoadInstrumentFromRaw instead
      if ( ! loadInst->isExecuted() )
      {
        //runLoadInstrumentFromRaw(localWorkspace);
      }
    }

    /** Populate spectra-detector map
        @param dae_handle The internal DAE identifier
        @param localWorkspace The workspace
     */
    void LoadDAE::loadSpectraMap(idc_handle_t dae_handle, DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Read in the number of detectors
      int ndet, dims_array[1],sv_ndims = 1;
      dims_array[0] = 1;
      if (IDCgetpari(dae_handle, "NDET", &ndet, dims_array, &sv_ndims) != 0)
      {
          g_log.error("Unable to read NDET from DAE " + m_daename);
          throw Exception::FileError("Unable to read NDET from DAE " , m_daename);
      };

      boost::shared_ptr<int> udet(new int[ndet]);
      //int* udet= new int[ndet];
      dims_array[0] = ndet;
      sv_ndims = 1;
      int res = 0;
      if ((res = IDCgetpari(dae_handle, "UDET", udet.get(), dims_array, &sv_ndims)) != 0)
      {
          g_log.error("Unable to read detector information (UDET) from DAE " + m_daename);
      }
      else
      {
          boost::shared_ptr<int> spec(new int[ndet]);
          if (IDCgetpari(dae_handle, "SPEC", spec.get(), dims_array, &sv_ndims) != 0)
          {
              g_log.error("Unable to read detector information (SPEC) from DAE " + m_daename);
              throw Exception::FileError("Unable to read detector information (SPEC) from DAE " , m_daename);
          }
          localWorkspace->mutableSpectraMap().populate(spec.get(), udet.get(), ndet);
      }

    }

    double LoadDAE::dblSqrt(double in)
    {
      return sqrt(in);
    }

  } // namespace DataHandling
} // namespace Mantid
