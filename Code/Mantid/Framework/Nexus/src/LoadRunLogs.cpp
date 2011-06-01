//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadRunLogs.h"
#include "MantidNexus/NeXusException.hpp"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FileProperty.h"
#include <cctype>

#include <Poco/Path.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormat.h>

#include <boost/scoped_array.hpp>

namespace Mantid
{
  namespace NeXus
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadRunLogs)

    /// Sets documentation strings for this algorithm
    void LoadRunLogs::initDocs()
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
    LoadRunLogs::LoadRunLogs()
    {}

    /// Initialisation method.
    void LoadRunLogs::init()
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
    void LoadRunLogs::exec()
    {
      std::string filename = getPropertyValue("Filename");
      ::NeXus::File file(filename);
      MatrixWorkspace_sptr workspace = getProperty("Workspace");
      
      // Find the root entry
      try
      {
        file.openGroup("entry", "NXentry"); // SNS style root
      }
      catch(::NeXus::Exception&)
      {
        try
        {
          file.openGroup("raw_data_1", "NXentry"); //ISIS style root
        }
        catch(::NeXus::Exception&)
        {
          throw std::invalid_argument("Unknown NeXus file format found in file '" + filename + "'");
        }
      }
        // print out the entry level fields
      std::map<std::string,std::string> entries = file.getEntries();
      std::map<std::string,std::string>::const_iterator iend = entries.end();
      for(std::map<std::string,std::string>::const_iterator it = entries.begin();
          it != iend; it++)
      {
        std::string entry_name(it->first);
        std::string entry_class(it->second);
        if( entry_name == "DASlogs" || entry_class == "IXrunlog" ||
            entry_class == "IXselog" )
        {
          loadLogs(file, entry_name, entry_class, workspace);
        }
      }
      file.close();
    }

    /**
     * Load log entries from the given group
     * @param file :: A reference to the NeXus file handle opened such that the
     * next call can be to open the named group
     * @param entry_name :: The name of the log entry
     * @param_entry_class :: The class type of the log entry
     * @param workspace :: A pointer to the workspace to store the logs
     */
    void LoadRunLogs::loadLogs(::NeXus::File & file, const std::string & entry_name, 
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
        if( log_class == "NXlog" || entry_class == "NXpositioner" ) 
        {
          loadNXLog(file, itr->first, workspace);
        }
        else if( log_class == "IXseblock" )
        {
          loadSELog(file, itr->first, workspace);
        }
      }

      file.closeGroup();
    }
    
    /**
     * Load an NX log entry
     * @param file :: A reference to the NeXus file handle opened at the parent group
     * @param entry_name :: The name of the log entry
     * @param workspace :: A pointer to the workspace to store the logs
     */
    void LoadRunLogs::loadNXLog(::NeXus::File & file, const std::string & entry_name, 
                                MatrixWorkspace_sptr workspace) const
    {
      file.openGroup(entry_name, "NXlog");
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
    void LoadRunLogs::loadSELog(::NeXus::File & file, const std::string & entry_name, 
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
      //   value_log - A time series entry
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
          g_log.warning() << "IXseblock entry " << entry_name << " gave an error when loading "
                          << "a time series:'" << e.what() << "'.\n";
          file.closeGroup();
          return;
        }
      }
      else if( entries.find("value") != entries.end() )
      {
        float val_array[1];
        try
        {
          file.openData("value");
          file.getData(val_array);
          file.closeData();
        }
        catch(::NeXus::Exception& e)
        {
          g_log.warning() << "IXseblock entry " << entry_name << " gave an error when loading "
                          << "a single value:'" << e.what() << "'.\n";
          file.closeData();
          file.closeGroup();
          return;
        }
        logValue = new Kernel::PropertyWithValue<double>(propName, static_cast<double>(val_array[0]), true);
      }
      else
      {
        g_log.warning() << "IXseblock entry " << entry_name 
                        << " does not contain a value or value_log field, skipping entry.";
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
    Kernel::Property * LoadRunLogs::createTimeSeries(::NeXus::File & file, 
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
          boost::scoped_array<char> val_array(new char[nitems*item_length]);
          file.getData(val_array.get());
          file.closeData();
          values = std::string(val_array.get());
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
