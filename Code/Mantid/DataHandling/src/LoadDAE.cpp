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

#include <cmath>
#include <boost/shared_ptr.hpp>

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
    static void loadData(const DataObjects::Histogram1D::RCtype::ptr_type& tcbs,int hist, int& ispec, idc_handle_t dae_handle, const int& lengthIn,
    		int* spectrum, DataObjects::Workspace2D_sptr localWorkspace)
    {
    	int ndims, dims_array[1];
        ndims = 1;
	dims_array[0] = lengthIn;

      // Read in spectrum number ispec from DAE
      IDCgetdat(dae_handle, ispec, 1, spectrum, dims_array, &ndims);
      // Put it into a vector, discarding the 1st entry, which is rubbish
      // But note that the last (overflow) bin is kept
      std::vector<double> v(spectrum + 1, spectrum + lengthIn);
      // Create and fill another vector for the errors, containing sqrt(count)
      std::vector<double> e(lengthIn-1);
      std::transform(v.begin(), v.end(), e.begin(), LoadDAE::dblSqrt);
      // Populate the workspace. Loop starts from 1, hence i-1
      localWorkspace->setData(hist, v, e);
      localWorkspace->setX(hist, tcbs);
      localWorkspace->setErrorHelper(hist,GaussianErrorHelper::Instance());
      localWorkspace->getAxis(1)->spectraNo(hist)= ispec;
      // NOTE: Raw numbers go straight into the workspace
      //     - no account taken of bin widths/units etc.
    }

    /// Initialisation method.
    void LoadDAE::init()
    {
      declareProperty("DAEname","", new MandatoryValidator<std::string>());
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output));

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("spectrum_min",0, mustBePositive);
      declareProperty("spectrum_max",0, mustBePositive->clone());
      declareProperty(new ArrayProperty<int>("spectrum_list"));
    }

    /// Function called by IDC routines to report an error
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
      int channelsPerSpectrum;
      if (IDCgetpari(dae_handle, "NTC1", &channelsPerSpectrum, sv_dims_array, &sv_ndims) != 0)
      {
        g_log.error("Unable to read NTC1 from DAE " + m_daename);
        throw Exception::FileError("Unable to read NTC1 from DAE " , m_daename);
      }

      // Read in the time bin boundaries
      const int lengthIn = channelsPerSpectrum + 1;
      float* timeChannels = new float[lengthIn];

      dims_array[0] = lengthIn;
      if (IDCgetparr(dae_handle, "RTCB1", timeChannels, dims_array, &sv_ndims) != 0)
      {
        g_log.error("Unable to read RTCB1 from DAE " + m_daename);
        throw Exception::FileError("Unable to read RTCB1 from DAE " , m_daename);
      }
      // Put the read in array into a vector (inside a shared pointer)
      boost::shared_ptr<std::vector<double> > timeChannelsVec
                          (new std::vector<double>(timeChannels, timeChannels + lengthIn));
      // Create an array to hold the read-in data
      int* spectrum = new int[lengthIn];

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

      int histTotal = total_specs * m_numberOfPeriods;
      int histCurrent = -1;
      // Loop over the number of periods in the raw file, putting each period in a separate workspace
      for (int period = 0; period < m_numberOfPeriods; ++period) {

        // Create the 2D workspace for the output
        DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
                 (WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1));
        // Set the unit on the workspace to TOF
        localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

        int counter = 0;
        for (int i = m_spec_min; i < m_spec_max; ++i)
        {
          // Shift the histogram to read if we're not in the first period
          int histToRead = i + period*total_specs;
          loadData(timeChannelsVec,counter,histToRead,dae_handle,lengthIn,spectrum,localWorkspace );
          counter++;
          if (++histCurrent % 10 == 0) progress(double(histCurrent)/histTotal);
          interruption_point();
        }
        // Read in the spectra in the optional list parameter, if set
        if (m_list)
        {
          for(unsigned int i=0; i < m_spec_list.size(); ++i)
          {
            loadData(timeChannelsVec,counter,m_spec_list[i],dae_handle,lengthIn,spectrum, localWorkspace );
            counter++;
            if (++histCurrent % 10 == 0) progress(double(histCurrent)/histTotal);
            interruption_point();
          }
        }
        // Just a sanity check
        assert(counter == total_specs);

        std::string outputWorkspace = "OutputWorkspace";
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
      delete[] timeChannels;
      delete[] spectrum;
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


    double LoadDAE::dblSqrt(double in)
    {
      return sqrt(in);
    }

  } // namespace DataHandling
} // namespace Mantid
