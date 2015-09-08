//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GetAllEi.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
  namespace Algorithms {

    DECLARE_ALGORITHM(GetAllEi)

    /// Empty default constructor
    GetAllEi::GetAllEi() : Algorithm() {}

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

      declareProperty("ChopperSpeedLog", "Chopper_Speed","Default name of the instrument log,"
        "containing chopper angular velocity.");
      declareProperty("ChopperDelayLog", "Chopper_Delay", "Default name of the instrument log, "
        "containing chopper delay time or chopper phase v.r.t. pulse time.");
      declareProperty("GoodCurrentLog", "good-uah-log", "Default name of the instrument log,"
        "containing good current, used to identify the experiment start time.");

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

      specid_t specNum = getProperty("MonitorSpectraID");
      size_t wsIndex = static_cast<size_t>(specNum);
      if (wsIndex != 0){
        wsIndex = inputWS->getIndexFromSpectrumNumber(specNum);
      }

    }

    /**Validates if input workspace contains all necessary logs and if all
    these logs are the logs of appropriate type
    @return list of invalid logs or empty list if no errors is found.
    */
    std::map<std::string, std::string> GetAllEi::validateInputs() {

      /** Lambda to validate if appropriate log is present in workspace
      and if it's present, it is a time-series property
      * @param Algo         -- pointer to current algorithm
      * @param prop_name    -- the name of the log to check
      * @param inputWS      -- the workspace to check for logs
      * @param err_presence -- core error message to return if no log found
      * @param err_type     -- core error message to return if 
      log is of incorrect type

      * @return result      -- map containing check errors -- 
      if no error found the map is not modified
      and remains empty.
      */
      auto check_time_series_property = [](GetAllEi const* const Algo, 
        const std::string &prop_name,const API::MatrixWorkspace_sptr &inputWS,
        const std::string &err_presence,const std::string &err_type,
        std::map<std::string, std::string> &result)
      {
          const std::string LogName = Algo->getProperty(prop_name);
          try{
            Kernel::Property * pProp = inputWS->run().getProperty(LogName);
            auto pTSProp  = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(pProp);
            if (pTSProp){
              result[prop_name] = "Workspace contains " + err_type +LogName+
                " But its type is not a timeSeries property";
            }
          }catch(std::runtime_error &){
            result[prop_name] = "Workspace has to contain " + err_presence + LogName;
          }

      };

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


      check_time_series_property(this,"ChopperSpeedLog",inputWS,
        "chopper speed log with name: ", "chopper speed log ",result);
      check_time_series_property(this,"ChopperDelayLog",inputWS,
        "property related to chopper delay log with name: ",
        "chopper delay log ",result);
      check_time_series_property(this,"good-uah-log",inputWS,
        "good current log: ","good current log ",result);



      return result;
    }


  } // namespace Algorithms
} // namespace Mantid
