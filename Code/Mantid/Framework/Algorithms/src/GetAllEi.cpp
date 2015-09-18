//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <boost/format.hpp>
#include <string>

#include "MantidAlgorithms/GetAllEi.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Unit.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
  namespace Algorithms {

    DECLARE_ALGORITHM(GetAllEi)

    /// Empty default constructor
    GetAllEi::GetAllEi() : Algorithm(),
      m_useFilterLog(true),m_min_Eresolution(0.08),
      // half maximal resolution for LET
      m_max_Eresolution(0.5e-3){}

    /// Initialization method.
    void GetAllEi::init() {

      declareProperty(
        new API::WorkspaceProperty<API::MatrixWorkspace>("Workspace", "",
        Kernel::Direction::Input),
        "The input workspace containing the monitor's spectra measured after the last chopper");
      auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int>>();
      mustBePositive->setLower(1);

      declareProperty("Monitor1SpecID", EMPTY_INT(), mustBePositive,
        "The workspace index (ID) of the spectra, containing first monitor's"
        "signal to analyze.");
      declareProperty("Monitor2SpecID", EMPTY_INT(), mustBePositive,
        "The workspace index (ID) of the spectra, containing second monitor's"
        "signal to analyze.");

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

      // build workspace to find monitor's peaks
      size_t det1WSIndex;
      auto workingWS = buildWorkspaceToFit(inputWS,det1WSIndex);

      // recalculate delay time from chopper position to monitor position
      auto detector1   = inputWS->getDetector(det1WSIndex);
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
      auto baseSpectrum = inputWS->getSpectrum(det1WSIndex);
      std::pair<double,double> TOF_range  = baseSpectrum->getXDataRange();

      double Period  = (0.5*1.e+6)/chopSpeed; // 0.5 because some choppers open twice.
      // Would be nice to have it 1 or 0.5 depending on chopper type, but
      // it looks like not enough information on what chopper is available on ws;
      std::vector<double> guess_opening;
      this->findGuessOpeningTimes(TOF_range,TOF0,Period,guess_opening);
      if(guess_opening.size()==0){
        throw std::runtime_error("Can not find any chopper opening time within TOF range: "
          +std::to_string(TOF_range.first)+':'+std::to_string(TOF_range.second));
      }else{
        g_log.debug()<<" Found : "<<guess_opening.size()<<
          " chopper prospective opening within time frame: "
          <<TOF_range.first<<" to: "<<TOF_range.second<<std::endl;
      }

      // convert to energy
      double unused(0.0);
      auto   destUnit = Kernel::UnitFactory::Instance().create("Energy");
      destUnit->initialize(mon1Distance, 0., 0., static_cast<int>(Kernel::DeltaEMode::Elastic),0.,unused);
      for(size_t i=0;i<guess_opening.size();i++){
        guess_opening[i] = destUnit->singleFromTOF(guess_opening[i]);
      }
      std::sort(guess_opening.begin(),guess_opening.end());



      std::vector<double> peaks_positions;
      std::vector<double> peaks_height;
      std::vector<double> peaks_width;
      std::vector<size_t> irange_min,irange_max;
      findBinRanges(workingWS->readX(0),guess_opening,
                    this->m_min_Eresolution,
                    irange_min,irange_max);

      double maxPeakEnergy(0);
      for(size_t i=0; i<guess_opening.size();i++){
        auto peakFitter = definePeakFinder(workingWS);

        double energy,height,sigma;
        bool found = findMonitorPeak(workingWS,peakFitter,irange_min[i],
          irange_max[i],energy,height,sigma);
        if(found){
          peaks_positions.push_back(energy);
          peaks_height.push_back(height);
          peaks_width.push_back(sigma);
          double peakEnery = height*sigma*std::sqrt(2.*M_PI);
          if(peakEnery>maxPeakEnergy){
            maxPeakEnergy = peakEnery;
          }
        }
      }
      workingWS.reset();

      size_t nPeaks = peaks_positions.size();
      if (nPeaks == 0){
        throw std::runtime_error("Can not identify any energy peaks");
      }
      auto result_ws =API::WorkspaceFactory::Instance().create("Workspace2D",1,
                             nPeaks , nPeaks);
      result_ws->setX(0, peaks_positions);
      MantidVec &Signal = result_ws->dataY(0);
      MantidVec &Error  = result_ws->dataE(0);
      for(size_t i=0;i<nPeaks;i++){
          Signal[i] = peaks_height[i];
          Error[i]  = peaks_width[i];
       }
       setProperty("OutputWorkspace",result_ws);


    }

    /**Get energy of monitor peak if one is present*/
    bool GetAllEi::findMonitorPeak(const API::MatrixWorkspace_sptr &inputWS,
      const API::IAlgorithm_sptr &peakFitter,
      size_t ind_min,size_t ind_max,
      double & energy,double & height,double &width){

        // interval too small -- can not be peak there
        if(std::fabs(double(ind_max-ind_min))<4)return false;

        double sMax,sMin,xOfMax;
        std::string peakGuessStr,
          bkgGuessStr,fitWindow,peakRangeStr;
        /**Lambda to identify guess values for a peak at given index*/
        auto peakGuess =[&](size_t index){
          sMin =  std::numeric_limits<double>::max();
          sMax = -sMin;
          auto X = inputWS->readX(index);
          auto S = inputWS->readY(index);
          double Intensity(0);
          fitWindow    = std::to_string(X[ind_min])+","+std::to_string(X[ind_max]);
          if(ind_max == S.size()){
            ind_max--;
          }
          for(size_t i=ind_min;i<ind_max;i++){
            if(S[i]<sMin)sMin=S[i];
            if(S[i]>sMax){
              sMax = S[i];
              xOfMax=X[i];
            }
            Intensity+=S[i];
          }
          Intensity*=std::fabs(X[ind_max]-X[ind_min]);
          // peak width on the half of the height 
          double fw = m_min_Eresolution*xOfMax/(2*std::sqrt(2*std::log(2.)));
          peakGuessStr = std::to_string(sMax)+","+std::to_string(xOfMax)+","+std::to_string(fw);
          bkgGuessStr  = std::to_string(sMin)+",0.";
          // identify maximal peak intensity on the basis of maximal instrument 
          // resolution. 
          //double iMax = 0.5*Smax*xOfMax*m_max_Eresolution*std::sqrt(M_PI/std::log(2.));
          //double iMin = 0.5*Smax*xOfMax*m_min_Eresolution*std::sqrt(M_PI/std::log(2.));
          double minHeight = Intensity/(0.5*xOfMax*m_min_Eresolution*std::sqrt(M_PI/std::log(2.)));
          double maxHeight = Intensity/(0.5*xOfMax*m_max_Eresolution*std::sqrt(M_PI/std::log(2.)));
          peakRangeStr = std::to_string(minHeight)+','+std::to_string(maxHeight);

        };
        //--------------------------------------------------------------------
        peakGuess(0);

        peakFitter->setProperty("PeakParameterValues",peakGuessStr);
        peakFitter->setProperty("BackgroundParameterValues",bkgGuessStr);
        peakFitter->setProperty("FitWindow",fitWindow);
        peakFitter->setProperty("PeakRange",peakRangeStr);
        peakFitter->setRethrows(false);

        bool failed=false;
        try{ // this is bug, it should not throw anymore
          peakFitter->execute();
        }catch(...){
          failed = true;
        }

        if(!peakFitter->isExecuted()||failed)return false;


// Does not work?
//        DataObjects::TableWorkspace_const_sptr result = peakFitter->getProperty("ParameterTableWorkspace");
//        auto pRes =const_cast<DataObjects::TableWorkspace *>(dynamic_cast<const DataObjects::TableWorkspace *>(result.get()));
        DataObjects::TableWorkspace_sptr pRes = boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(
           API::AnalysisDataService::Instance().retrieve("_fittedPeakParams"));
        if(!pRes){
          throw std::runtime_error("Can not convert result of fitting algorithm into table workspace");
        }
        height = pRes->Double(0,1);
        energy = pRes->Double(1,1);
        width  = pRes->Double(2,1);
        double A0  = pRes->Double(3,1);
        double A1  = pRes->Double(4,1);
      //

      return true;

    }

    /**Find indexes of each expected peak intervals from monotonous array of ranges.
    @param eBins   -- bin ranges to look through
    @param guess_energies -- vector of guess energies to look for
    @param irangeMin  -- start indexes of energy intervals in the guess_energies vector.
    @param irangeMax  -- final indexes of energy intervals in the guess_energies vector.
    */
    void GetAllEi::findBinRanges(const MantidVec & eBins,
      const std::vector<double> & guess_energy,double Eresolution,std::vector<size_t> & irangeMin,
      std::vector<size_t> & irangeMax){


      auto inRange = [&](size_t index,const double &eGuess){
        return (eBins[index]> eGuess*(1-2*Eresolution) && 
               (eBins[index]<=eGuess*(1+2*Eresolution)));
      };

      size_t nBins = eBins.size();
      size_t startIndex(0);
      bool within=false;
      for(size_t nGuess = 0;nGuess<guess_energy.size();nGuess++){
        within=false;
        for(size_t i=startIndex;i<nBins;i++){
            if(inRange(i,guess_energy[nGuess])){
              if(!within){
                within = true;
                if(i>1){
                  irangeMin.push_back(i-1);
                }else{
                  irangeMin.push_back(0);
                }
              }
            }else{
              if(within){
                irangeMax.push_back(i);
                startIndex=i;
                within = false;
                break;
              }
            }
        }
      }
      if(within){
           irangeMax.push_back(nBins-1);
      }
      // if array decreasing rather then increasing, indexes behave differently
      if(irangeMax[0]<irangeMin[0]){
        irangeMax.swap(irangeMin);
      }
    }


    /**Function implements common part of setting peak finder algorithm
    @param inputWS -- shared pointer to workspace to fit
    */
    API::IAlgorithm_sptr GetAllEi::definePeakFinder(const API::MatrixWorkspace_sptr &inputWS){

      API::IAlgorithm_sptr finder = createChildAlgorithm("FitPeak");
      finder->initialize();
      finder->setProperty("InputWorkspace",inputWS);
      finder->setProperty("OutputWorkspace","_fittedPeak");
      finder->setProperty("ParameterTableWorkspace","_fittedPeakParams");
      finder->setProperty("PeakFunctionType","Gaussian (Height, PeakCentre, Sigma)");
      finder->setProperty("BackgroundType","Linear (A0, A1)");
      finder->setProperty("Minimizer","Simplex");

      return finder;
    }
    /**Build 2-spectra workspace in units of energy, used as source
    *to identify actual monitors spectra 
    *@param inputWS shared pointer to initial workspace
    *@param wsIndex0 -- returns workspace index for first detector.
    *@return shared pointer to intermediate workspace, in units of energy
    *        used to fit monitor's spectra.
    */
    API::MatrixWorkspace_sptr GetAllEi::buildWorkspaceToFit(
      const API::MatrixWorkspace_sptr &inputWS, size_t &wsIndex0){



        // at this stage all properties are validated so its safe to access them without
        // additional checks.
        specid_t specNum1 = getProperty("Monitor1SpecID");
        wsIndex0 = inputWS->getIndexFromSpectrumNumber(specNum1);
        specid_t specNum2 = getProperty("Monitor2SpecID");
        size_t wsIndex1 = inputWS->getIndexFromSpectrumNumber(specNum2);
        auto pSpectr1   = inputWS->getSpectrum(wsIndex0);
        auto pSpectr2   = inputWS->getSpectrum(wsIndex1);
        // assuming equally binned ws.
        //auto bins       = inputWS->dataX(wsIndex0);
        auto bins = pSpectr1->dataX();
        size_t XLength  = bins.size();
        size_t YLength  = inputWS->dataY(wsIndex0).size();
        auto working_ws =API::WorkspaceFactory::Instance().create(inputWS,2,
                             XLength, YLength);
        // copy data --> very bad as implicitly assigns pointer
        // to bins array and bins array have to exist out of this routine
        // scope. 
        // This does not matter in this case as below we convert units
        // which should decouple cow_pointer but very scary operation in
        // general.
        working_ws->setX(0, bins);
        working_ws->setX(1, bins);
        MantidVec &Signal1 = working_ws->dataY(0);
        MantidVec &Error1  = working_ws->dataE(0);
        MantidVec &Signal2 = working_ws->dataY(1);
        MantidVec &Error2  = working_ws->dataE(1);
        for(size_t i=0;i<YLength;i++){
          Signal1[i] = inputWS->dataY(wsIndex0)[i];
          Error1[i]  = inputWS->dataE(wsIndex0)[i];
          Signal2[i] = inputWS->dataY(wsIndex1)[i];
          Error2[i]  = inputWS->dataE(wsIndex1)[i];
        }
        // copy detector mapping
       API::ISpectrum *spectrum = working_ws->getSpectrum(0);
       spectrum->setSpectrumNo(specNum1);
       spectrum->clearDetectorIDs();
       spectrum->addDetectorIDs(pSpectr1->getDetectorIDs());
       spectrum = working_ws->getSpectrum(1);
       spectrum->setSpectrumNo(specNum2);
       spectrum->clearDetectorIDs();
       spectrum->addDetectorIDs(pSpectr2->getDetectorIDs());


        if(inputWS->getAxis(0)->unit()->caption() != "Energy"){
          API::IAlgorithm_sptr conv = createChildAlgorithm("ConvertUnits");
          conv->initialize();
          conv->setProperty("InputWorkspace",working_ws);
          conv->setProperty("OutputWorkspace",working_ws);
          conv->setPropertyValue("Target","Energy");
          conv->setPropertyValue("EMode","Elastic");
          //conv->setProperty("AlignBins",true); --> throws due to bug in ConvertUnits
          conv->execute();

        }

        return working_ws;
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
        // its chopper phase provided
        if (units=="deg" || units.c_str()[0] == -80){
          chop_delay*=1.e+6/(360.*chop_speed); // convert in uSec
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
      if (!inputWS->isHistogramData()){
        result["Workspace"] = "Only histogram workspaces are currently supported. Rebin input workspace first.";
      }

      specid_t specNum1 = getProperty("Monitor1SpecID");
      try{
        size_t wsIndex = inputWS->getIndexFromSpectrumNumber(specNum1);
      }catch(std::runtime_error &){
        result["Monitor1SpecID"] = "Input workspace does not contain spectra with ID: "+boost::lexical_cast<std::string>(specNum1);
      }
      specid_t specNum2 = getProperty("Monitor2SpecID");
      try{
        size_t wsIndex = inputWS->getIndexFromSpectrumNumber(specNum2);
      }catch(std::runtime_error &){
        result["Monitor2SpecID"] = "Input workspace does not contain spectra with ID: "+boost::lexical_cast<std::string>(specNum2);
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
