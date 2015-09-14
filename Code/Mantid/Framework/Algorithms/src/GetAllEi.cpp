//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GetAllEi.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Unit.h"
#include <boost/format.hpp>

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

      double chopSpeed,chopDelay;
      findChopSpeedAndDelay(inputWS,chopSpeed,chopDelay);

      ////---> recalculate chopper delay to monitor position:
      //TODO: until chopper class is fully defined use light-weight chopper-position component
      auto pInstrument = inputWS->getInstrument();
      auto lastChopPositionComponent = pInstrument->getComponentByName("chopper-position");
      if(!lastChopPositionComponent){
        throw std::runtime_error("Instrument does not have chopper position component named 'chopper-position'");
      }
      auto moderator       = pInstrument->getSource();
      double chopDistance  = lastChopPositionComponent->getDistance(*moderator);

      double velocity  = chopDistance/chopDelay;

      // recalculate delay time from chopper position to monitor position
      auto detector1   = inputWS->getDetector(wsIndex);
      double mon1Distance = detector1->getDistance(*moderator);
      double TOF0      = mon1Distance/velocity;

      //--->> below is reserved until full chopper's implementation is available;
      //auto nChoppers = pInstrument->getNumberOfChopperPoints();
      // get last chopper.
      /*
      if( nChoppers==0)throw std::runtime_error("Instrument does not have any choppers defined");

      auto lastChopper = pInstrument->getChopperPoint(nChoppers-1);
      ///<---------------------------------------------------
      */
      auto baseSpectrum = inputWS->getSpectrum(wsIndex);
      std::pair<double,double> TOF_range  = baseSpectrum->getXDataRange();

      double Period  = (0.5*1.e+6)/chopSpeed; // 0.5 because some choppers open twice.
      // Would be nice to have it 1 or 0.5 depending on chopper type, but
      // it looks like not enough information on what chopper is available on ws;
      std::vector<double> guess_opening;
      this->findGuessOpeningTimes(TOF_range,TOF0,Period,guess_opening);

      // convert to energy
      double unused(0.0);
      auto   destUnit = Kernel::UnitFactory::Instance().create("Energy");
      destUnit->initialize(mon1Distance, 0., 0., static_cast<int>(Kernel::DeltaEMode::Elastic),0.,unused);
      for(size_t i=0;i<guess_opening.size();i++){
        guess_opening[i] = destUnit->singleFromTOF(guess_opening[i]);
      }

      IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");


    }
    /**function calculates list of provisional chopper opening times.
    *@param TOF_range -- std::pair containing min and max time, of signal
    *                    measured on monitors
    *@param  ChopDelay -- the time of flight neutrons travel from source 
    *                     to the chopper opening.
    *@param  Period  -- period of chopper openings

    *@return guess_opening_times -- vector of times at which neutrons may
    *                               pass through the chopper.
    */
    void GetAllEi::findGuessOpeningTimes(const std::pair<double,double> &TOF_range,
      double ChopDelay,double Period,std::vector<double > & guess_opening_times){

        if(ChopDelay>= TOF_range.second){
          std::string chop = boost::str(boost::format("%.2g") % ChopDelay);
          std::string t_min =boost::str(boost::format("%.2g") % TOF_range.first);
          std::string t_max =boost::str(boost::format("%.2g") % TOF_range.second);
          throw std::runtime_error("Logical error: Chopper opening time: "+chop+
            " is outside of time interval: "+t_min+":"+t_max+
            " of the signal, measured on monitors.");
        }

        // number of times chopper with specified rotation period opens.
        size_t n_openings = static_cast<size_t>((TOF_range.second-ChopDelay)/Period)+1;
        // number of periods falling outside of the time period, measuring on monitor.
        size_t n_start(0);
        if(ChopDelay<TOF_range.first){
          n_start = static_cast<size_t>((TOF_range.first-ChopDelay)/Period)+1;
          n_openings -= n_start;
        }

        guess_opening_times.resize(n_openings);
        for(size_t i=n_start;i<n_openings+n_start;i++){
          guess_opening_times[i-n_start]= ChopDelay + static_cast<double>(i)*Period;
        }
    }

    /**Return average time series log value for the appropriately filtered log
    * @param inputWS      -- shared pointer to the input workspace containing
    *                        the log to process
    * @param propertyName -- log name
    * @param splitter     -- array of Kernel::splittingInterval data, used to
    *                        filter input events or empty array to use 
    *                        experiment start/end times.
    */
    double GetAllEi::getAvrgLogValue(const API::MatrixWorkspace_sptr &inputWS,
      const std::string &propertyName,std::vector<Kernel::SplittingInterval> &splitter){

        const std::string LogName = this->getProperty(propertyName);
        auto pIProperty  = (inputWS->run().getProperty(LogName));

        // this will always provide a defined pointer as this has been verified in validator.
        auto pTimeSeries = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(pIProperty);

        if(splitter.size()==0){
          auto TimeStart = inputWS->run().startTime();
          auto TimeEnd   = inputWS->run().endTime();
          pTimeSeries->filterByTime(TimeStart,TimeEnd);
        }else{
          pTimeSeries->filterByTimes(splitter);
        }
        if(pTimeSeries->size() == 0){
          throw std::runtime_error("Can not find average value for log: "+LogName+
            " As no valid log values are found.");
        }

        return pTimeSeries->getStatistics().mean;
    }
    /**Analyze chopper logs and identify chopper speed and delay
    @param  inputWS    -- sp to workspace with attached logs.
    @return chop_speed -- chopper speed in uSec
    @return chop_delay -- chopper delay in uSec
    */
    void GetAllEi::findChopSpeedAndDelay(const API::MatrixWorkspace_sptr &inputWS,
      double &chop_speed,double &chop_delay){

        //TODO: Make it dependent on inputWS time range
        //const double tolerance(1.e-6);
        const double tolerance(0);

        std::vector<Kernel::SplittingInterval> splitter;
        if(m_useFilterLog){
          const std::string FilterLogName = this->getProperty("FilterBaseLog");
          // pointer will not be null as this has been verified in validators
          auto pTimeSeries = dynamic_cast<Kernel::TimeSeriesProperty<double> *>
            (inputWS->run().getProperty(FilterLogName));

          // Define selecting function
          bool inSelection(false);
          // time interval to select (start-end)
          Kernel::DateAndTime startTime,endTime;
          /**Select time interval on the basis of previous time interval state */
          auto SelectInterval = [&](const Kernel::DateAndTime &time, double value){

            endTime = time;
            if(value>0){
              if (!inSelection){
                startTime = time+tolerance;
              }
              inSelection = true;
            }else{
              if (inSelection){
                inSelection = false;
                if(endTime>startTime) return true;
              }
            }
            return false;
          };
          //
          // Analyze filtering log
          auto dateAndTimes = pTimeSeries->valueAsCorrectMap();
          auto it = dateAndTimes.begin();
          // initialize selection log
          SelectInterval(it->first,it->second);
          if(dateAndTimes.size()==1){
            if(inSelection){
              startTime = inputWS->run().startTime();
              endTime    = inputWS->run().endTime();
              Kernel::SplittingInterval interval(startTime, endTime, 0);
              splitter.push_back(interval);
            }else{
              throw std::runtime_error("filter log :"+FilterLogName+" filters all data points. Nothing to do");
            }
          }

          it++;
          for (; it != dateAndTimes.end(); ++it) {
            if(SelectInterval(it->first,it->second)){
              Kernel::SplittingInterval interval(startTime, endTime - tolerance, 0);
              splitter.push_back(interval);
            }
          }
          // final interval
          if(inSelection && (endTime>startTime)){
            Kernel::SplittingInterval interval(startTime, endTime - tolerance, 0);
            splitter.push_back(interval);
          }
        }


        chop_speed = this->getAvrgLogValue(inputWS,"ChopperSpeedLog",splitter);
        chop_speed = std::fabs(chop_speed);
        if(chop_speed <1.e-7){
          throw std::runtime_error("Chopper speed can not be zero ");
        }
        chop_delay = std::fabs(this->getAvrgLogValue(inputWS,"ChopperDelayLog",splitter));

        // process chopper delay in the units of degree (phase)
        auto pProperty  = (inputWS->run().getProperty(this->getProperty("ChopperDelayLog")));
        std::string units = pProperty->units();
        if (units=="deg" || units.c_str()[0] == -80){
          chop_delay*=1.e+6/(360.*chop_speed); // conver in uSec
        }

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
      *                        log is of incorrect type
      * @param fail         -- fail or warn if appropriate log is not available.

      * @return             -- false if all properties are fine, or true if check is failed
      * modifies result    -- map containing check errors
      *                       if no error found the map is not modified and remains empty.
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
