//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadNexusLogs.h"
#include "MantidNexusCPP/NeXusException.hpp"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/LogParser.h"
#include "MantidAPI/FileProperty.h"
#include <cctype>

#include <Poco/Path.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormat.h>

#include <boost/scoped_array.hpp>
#include "MantidDataHandling/LoadTOFRawNexus.h"

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadNexusLogs)

    /// Sets documentation strings for this algorithm
    void LoadNexusLogs::initDocs()
    {
      this->setWikiSummary("Loads run logs (temperature, pulse charges, etc.) from a NeXus file and adds it to the run information in a [[workspace]].");
      this->setOptionalMessage("Loads run logs (temperature, pulse charges, etc.) from a NeXus file and adds it to the run information in a workspace.");
    }
    
    using namespace Kernel;
    using API::WorkspaceProperty;
    using API::MatrixWorkspace;
    using API::MatrixWorkspace_sptr;
    using API::FileProperty;
    using std::size_t;

    /// Empty default constructor
    LoadNexusLogs::LoadNexusLogs()
    {}

    /// Initialisation method.
    void LoadNexusLogs::init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut));
      std::vector<std::string> exts;
      exts.push_back(".nxs");
      exts.push_back(".n*");
      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                      "The name of the Nexus file to load" );
      declareProperty(new PropertyWithValue<bool>("OverwriteLogs", true, Direction::Input));


    }

    /** Executes the algorithm. Reading in the file and creating and populating
    *  the output workspace
    * 
    *  @throw Exception::FileError If the Nexus file cannot be found/opened
    *  @throw std::invalid_argument If the optional properties are set to invalid values
    */
    void LoadNexusLogs::exec()
    {
      std::string filename = getPropertyValue("Filename");
      MatrixWorkspace_sptr workspace = getProperty("Workspace");
      

      // Find the entry name to use (normally "entry" for SNS, "raw_data_1" for ISIS)
      std::string entry_name = LoadTOFRawNexus::getEntryName(filename);

      ::NeXus::File file(filename);
      // Find the root entry
      try
      {
        file.openGroup(entry_name, "NXentry");
      }
      catch(::NeXus::Exception&)
      {
        throw std::invalid_argument("Unknown NeXus file format found in file '" + filename + "'");
      }

      // print out the entry level fields
      std::map<std::string,std::string> entries = file.getEntries();
      std::map<std::string,std::string>::const_iterator iend = entries.end();
      for(std::map<std::string,std::string>::const_iterator it = entries.begin();
          it != iend; it++)
      {
        std::string group_name(it->first);
        std::string group_class(it->second);
        if( group_name == "DASlogs" || group_class == "IXrunlog" ||
            group_class == "IXselog" )
        {
          loadLogs(file, group_name, group_class, workspace);
        }
      }
      try
      {
        // Read the start and end time strings
        file.openData("start_time");
        Kernel::DateAndTime start(file.getStrData());
        file.closeData();
        file.openData("end_time");
        Kernel::DateAndTime end(file.getStrData());
        file.closeData();
        workspace->mutableRun().setStartAndEndTime(start, end);
      }
      catch(::NeXus::Exception&)
      {}

      if( !workspace->run().hasProperty("gd_prtn_chrg") )
      {
        // Try pulling it from the main proton_charge entry first
        try
        {
          file.openData("proton_charge");
          std::vector<double> values;
          file.getDataCoerce(values);
          std::string units;
          file.getAttr("units", units);
          double charge = values.front();
          if( units.find("picoCoulomb") != std::string::npos )
          {
            charge *= 1.e-06/3600.;
          }
          workspace->mutableRun().setProtonCharge(charge);
        }
        catch(::NeXus::Exception&)
        {
          // Try and integrate the proton logs
          try
          {
            //Use the DAS logs to integrate the proton charge (if any).
            workspace->mutableRun().integrateProtonCharge();
          }
          catch (Exception::NotFoundError &)
          {
            //Ignore not found property error.
          }
        }
      }

      // Close the file
      file.close();

      // Filter logs
      // If we loaded an icp_event log then create the running log to
      // allow filtering 
      if( workspace->run().hasProperty("icp_event") )
      {
        Kernel::Property *log = workspace->run().getProperty("icp_event");
        LogParser parser(log);
        workspace->mutableRun().addProperty(parser.createRunningLog());
      }
    }

    /**
     * Load log entries from the given group
     * @param file :: A reference to the NeXus file handle opened such that the
     * next call can be to open the named group
     * @param entry_name :: The name of the log entry
     * @param_entry_class :: The class type of the log entry
     * @param workspace :: A pointer to the workspace to store the logs
     */
    void LoadNexusLogs::loadLogs(::NeXus::File & file, const std::string & entry_name, 
                               const std::string & entry_class, 
                               MatrixWorkspace_sptr workspace) const
    {
      file.openGroup(entry_name, entry_class);
      std::map<std::string,std::string> entries = file.getEntries();
      std::map<std::string,std::string>::const_iterator iend = entries.end();
      for(std::map<std::string,std::string>::const_iterator itr = entries.begin();
          itr != iend; itr++)
      {
        std::string log_class = itr->second;
        if( log_class == "NXlog" || log_class == "NXpositioner" ) 
        {
          loadNXLog(file, itr->first, log_class, workspace);
        }
        else if( log_class == "IXseblock" )
        {
          loadSELog(file, itr->first, workspace);
        }
      }

      file.closeGroup();
    }
    
    /**
     * Load an NX log entry a group type that has value and time entries.
     * @param file :: A reference to the NeXus file handle opened at the parent group
     * @param entry_name :: The name of the log entry
     * @param entry_class :: The type of the entry
     * @param workspace :: A pointer to the workspace to store the logs
     */
    void LoadNexusLogs::loadNXLog(::NeXus::File & file, const std::string & entry_name, 
                                  const std::string & entry_class,
                                  MatrixWorkspace_sptr workspace) const
    {
      file.openGroup(entry_name, entry_class);
      // Validate the NX log class.
      std::map<std::string, std::string> entries = file.getEntries();
      if ((entries.find("value") == entries.end()) ||
          (entries.find("time") == entries.end()) )
      {
        g_log.warning() << "Invalid NXlog entry " << entry_name 
                        << " found. Did not contain 'value' and 'time'.\n";
        file.closeGroup();
        return;
      }
      // whether or not to overwrite logs on workspace
      bool overwritelogs = this->getProperty("OverwriteLogs");
      try
      {
        Kernel::Property *logValue = createTimeSeries(file, entry_name);
        workspace->mutableRun().addProperty(logValue, overwritelogs);
      }
      catch(::NeXus::Exception &e)
      {
        g_log.warning() << "NXlog entry " << entry_name 
                        << " gave an error when loading:'" << e.what() << "'.\n";
      }

      file.closeGroup();
    }

    /**
     * Load an SE log entry
     * @param file :: A reference to the NeXus file handle opened at the parent group
     * @param entry_name :: The name of the log entry
     * @param workspace :: A pointer to the workspace to store the logs
     */
    void LoadNexusLogs::loadSELog(::NeXus::File & file, const std::string & entry_name, 
                                MatrixWorkspace_sptr workspace) const
    {
      // Open the entry
      file.openGroup(entry_name, "IXseblock");
      std::string propName = entry_name;
      if (workspace->run().hasProperty(propName))
      {
        propName = "selog_" + propName;
      }
      // There are two possible entries:
      //   value_log - A time series entry. This can contain a corrupt value entry so if it does use the value one
      //   value - A single value float entry
      Kernel::Property *logValue(NULL);
      std::map<std::string, std::string> entries = file.getEntries();
      if( entries.find("value_log") != entries.end() )
      {
        try
        {
          try
          {
            file.openGroup("value_log", "NXlog");
          }
          catch(::NeXus::Exception&)
          {
            file.closeGroup();
            throw;
          }
          logValue = createTimeSeries(file, propName);
          file.closeGroup();
        }
        catch(::NeXus::Exception& e)
        {
          g_log.warning() << "IXseblock entry '" << entry_name << "' gave an error when loading "
                          << "a time series:'" << e.what() << "'. Skipping entry\n";
          file.closeGroup(); //value_log
          file.closeGroup();//entry_name
          return;
        }
      }
      else if( entries.find("value") != entries.end() )
      {
        try
        {
          // This may have a larger dimension than 1 bit it has no time field so take the first entry
          file.openData("value");
          ::NeXus::Info info = file.getInfo();
          if( info.type == ::NeXus::FLOAT32 )
          {
            boost::scoped_array<float> value(new float[info.dims[0]]);
            file.getData(value.get());
            file.closeData();
            logValue = new Kernel::PropertyWithValue<double>(propName, static_cast<double>(value[0]), true);
          }
          else
          {
            file.closeGroup();
            return;
          }
        }
        catch(::NeXus::Exception& e)
        {
          g_log.warning() << "IXseblock entry " << entry_name << " gave an error when loading "
                          << "a single value:'" << e.what() << "'.\n";
          file.closeData();
          file.closeGroup();
          return;
        }
      }
      else
      {
        g_log.warning() << "IXseblock entry " << entry_name 
                        << " cannot be read, skipping entry.\n";
        file.closeGroup();
        return;
      }
      workspace->mutableRun().addProperty(logValue);
      file.closeGroup();
    }

    /**
     * Creates a time series property from the currently opened log entry. It is assumed to
     * have been checked to have a time field and the value entry's name is given as an argument
     * @param file :: A reference to the file handle
     * @param prop_name :: The name of the property
     * @returns A pointer to a new property containing the time series
     */
    Kernel::Property * LoadNexusLogs::createTimeSeries(::NeXus::File & file, 
                                                       const std::string & prop_name) const
    {
      file.openData("time");
      //----- Start time is an ISO8601 string date and time. ------
      std::string start;
      try 
      {
        file.getAttr("start", start);
      }
      catch (::NeXus::Exception &)
      {
        //Some logs have "offset" instead of start
        try 
        {
          file.getAttr("offset", start);
        }
        catch (::NeXus::Exception &)
        {
          g_log.warning() << "Log entry has no start time indicated.\n";
          file.closeData();
          throw;
        }
      }
      //Convert to date and time
      Kernel::DateAndTime start_time = Kernel::DateAndTime(start);
      std::string time_units;
      file.getAttr("units", time_units);
      if( time_units.find("second") != 0 && time_units != "minutes" )
      {
        file.closeData();
        throw ::NeXus::Exception("Unsupported time unit '" + time_units + "'");
      }
      //--- Load the seconds into a double array ---
      std::vector<double> time_double;
      try 
      {
        file.getDataCoerce(time_double);
      }
      catch (::NeXus::Exception &e)
      {
        g_log.warning() << "Log entry 's time field could not be loaded: '" << e.what() << "'.\n";
        file.closeData();
        throw;
      }
      file.closeData(); // Close time data
      // Convert to seconds if needed
      if( time_units == "minutes" )
      {
        std::transform(time_double.begin(),time_double.end(), time_double.begin(),
                       std::bind2nd(std::multiplies<double>(),60.0));
      }
      // Now the values: Could be a string, int or double
      file.openData("value");
      // Get the units of the property
      std::string value_units("");
      try
      {
        file.getAttr("units", value_units);
      }
      catch (::NeXus::Exception &)
      {
        //Ignore missing units field.
        value_units = "";
      }

      // Now the actual data
      ::NeXus::Info info = file.getInfo();
      // Check the size
      if( size_t(info.dims[0]) != time_double.size() )
      {
        file.closeData();
        throw ::NeXus::Exception("Invalid value entry for time series");
      }
      if( file.isDataInt() ) // Int type
      {
        std::vector<int> values;
        try
        {
          file.getDataCoerce(values);
          file.closeData();
        }
        catch(::NeXus::Exception&)
        {
          file.closeData();
          throw;
        }
        //Make an int TSP
        TimeSeriesProperty<int> * tsp = new TimeSeriesProperty<int>(prop_name);
        tsp->create(start_time, time_double, values);
        tsp->setUnits(value_units);
        return tsp;
      }
      else if( info.type == ::NeXus::CHAR )
      {
        std::string values;
        const int item_length = info.dims[1];        
        try
        {
          const int nitems = info.dims[0];
          const int total_length = nitems*item_length;
          boost::scoped_array<char> val_array(new char[total_length]);
          file.getData(val_array.get());
          file.closeData();
          values = std::string(val_array.get(), total_length);
        }
        catch(::NeXus::Exception&)
        {
          file.closeData();
          throw;
        }
        // The string may contain non-printable (i.e. control) characters, replace these
        std::replace_if(values.begin(), values.end(), iscntrl, ' '); 
        TimeSeriesProperty<std::string> * tsp = new TimeSeriesProperty<std::string>(prop_name);
        std::vector<DateAndTime> times;
        DateAndTime::createVector(start_time, time_double, times);
        const size_t ntimes = times.size();
        for(size_t i = 0; i < ntimes; ++i)
        {
          std::string value_i = std::string(values.data() + i*item_length, item_length);
          tsp->addValue(times[i], value_i);
        }
        tsp->setUnits(value_units);
        return tsp;
      }
      else if( info.type == ::NeXus::FLOAT32 || info.type == ::NeXus::FLOAT64 )
      {
        std::vector<double> values;
        try
        {
          file.getDataCoerce(values);
          file.closeData();
        }
        catch(::NeXus::Exception&)
        {
          file.closeData();
          throw;
        }
        TimeSeriesProperty<double> * tsp = new TimeSeriesProperty<double>(prop_name);
        tsp->create(start_time, time_double, values);
        tsp->setUnits(value_units);
        return tsp;
      }
      else
      {
        throw ::NeXus::Exception("Invalid value type for time series. Only int, double or strings are "
                                 "supported");
      }
    }

  } // namespace DataHandling
} // namespace Mantid
