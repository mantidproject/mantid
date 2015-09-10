//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GetAllEi.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"

namespace Mantid {
  namespace Algorithms {

    DECLARE_ALGORITHM(GetAllEi)

    /// Empty default constructor
    GetAllEi::GetAllEi() : Algorithm(),
      m_useFilterLog(true){}

    /// Initialization method.
    void GetAllEi::init() {

      declareProperty(
        new API::WorkspaceProperty<API::MatrixWorkspace>("Workspace", "",
        Kernel::Direction::Input),
        "The input workspace containing the monitor's spectra measured after the last chopper");
      auto nonNegative = boost::make_shared<Kernel::BoundedValidator<int>>();
      nonNegative->setLower(-1);
      declareProperty("MonitorSpectraID", 0, nonNegative,
        "The workspace index (ID) of the spectra, containing monitor's"
        "signal to analyze.\n"
        "If omitted, spectra with number 0 (Not workspace ID!) will be used.\n");

      declareProperty("ChopperSpeedLog", "Chopper_Speed","Name of the instrument log,"
        "containing chopper angular velocity.");
      declareProperty("ChopperDelayLog", "Chopper_Delay", "Name of the instrument log, "
        "containing chopper delay time or chopper phase v.r.t. pulse time.");
      declareProperty("FilterBaseLog", "proton_charge", "Name of the instrument log,"
        "with positive values indicating that instrument is running\n"
        "and 0 or negative that it is not.\n"
        "The log is used to identify time interval to evaluate chopper speed and chopper delay which matter.\n"
        "If such log is not present, log values are calculated from experiment start&end times.");

      declareProperty(new API::WorkspaceProperty<API::Workspace>("OutputWorkspace", "",
        Kernel::Direction::Output),
        "Name of the output matrix workspace, containing single spectra with"
        " monitor peaks energies\n"
        "together with total intensity within each peak.");
    }
    /** Executes the algorithm -- found all existing monitor peaks. */
    void GetAllEi::exec() {
      // Get pointers to the workspace, parameter map and table
      API::MatrixWorkspace_sptr inputWS = getProperty("Workspace");

      // at this stage all properties are validated so its safe to access them without 
      // big additional checks.
      specid_t specNum = getProperty("MonitorSpectraID");
      size_t wsIndex = static_cast<size_t>(specNum);
      if (wsIndex != 0){
        wsIndex = inputWS->getIndexFromSpectrumNumber(specNum);
      }

      double chop_speed,chop_delay;
      find_chop_speed_and_delay(inputWS,chop_speed,chop_delay);

    }
    /**Return average time series log value for the appropriately filtered log
    @param inputWS      -- shared pointer to the input workspace containing
                           the log to process
    @param propertyName -- log name
    @param splitter     -- array of Kernel::splittingInterval data, used to
                           filter input events or empty array to use 
                           experiment start/end times.
    */
    double GetAllEi::getAvrgLogValue(const API::MatrixWorkspace_sptr &inputWS,
      const std::string &propertyName,std::vector<Kernel::SplittingInterval> &splitter){

        const std::string LogName = this->getProperty(propertyName);
        auto pIProperty  = (inputWS->run().getProperty(LogName));
        auto pTimeSeries = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(pIProperty);

        // this will always provide a defined pointer as this has been verified earlier.
        if(splitter.size()==0){
          auto TimeStart = inputWS->run().startTime();
          auto TimeEnd   = inputWS->run().endTime();
          pTimeSeries->filterByTime(TimeStart,TimeEnd);
        }else{
          pTimeSeries->filterByTimes(splitter);
        }

        return pTimeSeries->getStatistics().mean;
    }
    /**Analyze chopper logs and identify chopper speed and delay
    @param  inputWS    -- sp to workspace with attached logs.
    @return chop_speed -- chopper speed in uSec
    @return chop_delay -- chopper delay in uSec
    */
    void GetAllEi::find_chop_speed_and_delay(const API::MatrixWorkspace_sptr &inputWS,
      double &chop_speed,double &chop_delay){

        std::vector<Kernel::SplittingInterval> splitter;
        if(m_useFilterLog){
          const std::string ChopDelayLogName = this->getProperty("FilterBaseLog");
        }


        chop_speed = this->getAvrgLogValue(inputWS,"ChopperSpeedLog",splitter);
        chop_delay = this->getAvrgLogValue(inputWS,"ChopperDelayLog",splitter);

        //auto goodCurrentMap = pGoodCurProp->valueAsMap();


    }

    /**Validates if input workspace contains all necessary logs and if all
    these logs are the logs of appropriate type
    @return list of invalid logs or empty list if no errors is found.
    */
    std::map<std::string, std::string> GetAllEi::validateInputs() {


      // Do Validation
      std::map<std::string, std::string> result;

      API::MatrixWorkspace_sptr inputWS = getProperty("Workspace");
      if (!inputWS){
        result["Workspace"]="Input workspace can not be identified";
        return result;
      }
      specid_t specNum = getProperty("MonitorSpectraID");
      size_t wsIndex = static_cast<size_t>(specNum);
      if (wsIndex != 0){
        try{
          wsIndex = inputWS->getIndexFromSpectrumNumber(specNum);
        }catch(std::runtime_error &){
          result["MonitorSpectraID"] = "Input workspace does not contain spectra with ID: "+boost::lexical_cast<std::string>(specNum);
          return result;
        }
      }

      /** Lambda to validate if appropriate log is present in workspace
      and if it's present, it is a time-series property
      * @param prop_name    -- the name of the log to check
      * @param err_presence -- core error message to return if no log found
      * @param err_type     -- core error message to return if 
      log is of incorrect type
      * @param fail         -- fail or warn if appropriate log is not available.

      * @return             -- false if all properties are fine, or true if check is failed
      * modifies result  -- map containing check errors -- 
      if no error found the map is not modified and remains empty.
      */
      auto check_time_series_property = [&](const std::string &prop_name,
        const std::string &err_presence,const std::string &err_type, bool fail)
      {
        const std::string LogName = this->getProperty(prop_name);
        try{
          Kernel::Property * pProp = inputWS->run().getProperty(LogName);
          auto pTSProp  = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(pProp);
          if (!pTSProp){
            if(fail)result[prop_name] = "Workspace contains " + err_type +LogName+
              " But its type is not a timeSeries property";
            return true;
          }
        }catch(std::runtime_error &){
          if(fail) result[prop_name] = "Workspace has to contain " + err_presence + LogName;
          return true;
        }
        return false;
      };

      check_time_series_property("ChopperSpeedLog",
        "chopper speed log with name: ", "chopper speed log ",true);
      check_time_series_property("ChopperDelayLog",
        "property related to chopper delay log with name: ",
        "chopper delay log ",true);
      bool failed = check_time_series_property("FilterBaseLog",
        "filter base log named: ","filter base log: ",false);
      if(failed){
        g_log.warning()<<" Can not find a log to identify good DAE operations.\n"
          " Assuming that good operations start from experiment time=0";
        m_useFilterLog = false;
      }else{
        m_useFilterLog = true;
      }

      return result;
    }


  } // namespace Algorithms
} // namespace Mantid
