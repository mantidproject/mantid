#include "MantidAlgorithms/SumEventsByLogValue.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Column.h"
#include <numeric>

namespace Mantid
{
namespace Algorithms
{
  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(SumEventsByLogValue);

  using namespace Kernel;
  using namespace API;

  /** Constructor
   */
  SumEventsByLogValue::SumEventsByLogValue()
  {
  }
    
  /** Destructor
   */
  SumEventsByLogValue::~SumEventsByLogValue()
  {
  }
  
  /// Sets documentation strings for this algorithm
  void SumEventsByLogValue::initDocs()
  {
    this->setWikiSummary("Produces a single spectrum containing the total summed events in the workspace as a function of a specified log.");
    this->setOptionalMessage("Produces a single spectrum containing the total summed events in the workspace as a function of a specified log.");
  }

  void SumEventsByLogValue::init()
  {
    declareProperty( new WorkspaceProperty<DataObjects::EventWorkspace>("InputWorkspace","",Direction::Input),
                              "The input EventWorkspace" );
    declareProperty( new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output),
                              "The name of the workspace to be created as the output of the algorithm" );

    declareProperty( "LogName","",boost::make_shared<MandatoryValidator<std::string>>(),
                              "The name of the number series log against which the data should be summed" );
    declareProperty( new ArrayProperty<double>("OutputBinning","",boost::make_shared<RebinParamsValidator>(true)),
                              "Binning parameters for the output workspace (optional for integer typed logs)" );
  }

  std::map<std::string, std::string> SumEventsByLogValue::validateInputs()
  {
    std::map<std::string, std::string> errors;

    // Check that the log exists for the given input workspace
    m_inputWorkspace = getProperty("InputWorkspace");
    m_logName = getPropertyValue("LogName");
    try {
      ITimeSeriesProperty * log =
          dynamic_cast<ITimeSeriesProperty*>( m_inputWorkspace->run().getLogData(m_logName) );
      if ( log == NULL )
      {
        errors["LogName"] = "'" + m_logName + "' is not a time-series log.";
        return errors;
      }
      if ( log->realSize() == 0 )
      {
        errors["LogName"] = "'" + m_logName + "' is empty.";
      }
    } catch ( Exception::NotFoundError& ) {
      errors["LogName"] = "The log '" + m_logName + "' does not exist in the workspace '" + m_inputWorkspace->name() + "'.";
      return errors;
    }

    return errors;
  }

  void SumEventsByLogValue::exec()
  {
    // Get hold of the requested log. Will throw if it doesn't exist (which is what we want).
    const Property * const log = m_inputWorkspace->run().getLogData(m_logName);

    // Now we need to know what type of property it is
    const TimeSeriesProperty<int> * intLog = dynamic_cast<const TimeSeriesProperty<int>*>(log);
    const TimeSeriesProperty<double> * dblLog = dynamic_cast<const TimeSeriesProperty<double>*>(log);

    m_binningParams = getProperty("OutputBinning");
    // Binning parameters must be provided for floating point logs
    if ( m_binningParams.empty() )
    {
      if ( intLog != NULL )
      {
        createTableOutput(intLog);
      }
      else
      {
        throw std::invalid_argument("OutputBinning must be provided for floating-point number logs");
      }
    }
    else // Binning parameters have been given
    {
      if ( intLog != NULL )
      {
        createBinnedOutput(intLog);
      }
      else if ( dblLog != NULL )
      {
        createBinnedOutput(dblLog);
      }
      //else if ( dynamic_cast<const TimeSeriesProperty<std::string>*>(log) != NULL )
      //{
      //  TODO: Implement (if anyone ever asks for it).
      //}
      else
      {
        throw std::runtime_error("This algorithm only supports number-series logs");
      }
    }
  }

  void SumEventsByLogValue::createTableOutput(const Kernel::TimeSeriesProperty<int> * log)
  {
    // This is the version for integer logs when no binning parameters have been given and has a data point per log value
    const int minVal = log->minValue();
    const int maxVal = log->maxValue();
    const int xLength = maxVal - minVal + 1;

    // Accumulate things in a local vector before transferring to the table
    std::vector<int> Y(xLength);
    const int numSpec = static_cast<int>(m_inputWorkspace->getNumberHistograms());
    Progress prog(this,0.0,1.0,numSpec);
    PARALLEL_FOR1(m_inputWorkspace)
    for ( int spec = 0; spec < numSpec; ++spec )
    {
      PARALLEL_START_INTERUPT_REGION
      const IEventList & eventList = m_inputWorkspace->getEventList(spec);
      filterEventList(eventList, minVal, maxVal, log, Y);
      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
    // For now, no errors. Do we need them?

    // Create a table workspace to hold the sum.
    ITableWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().createTable();
    auto logValues = outputWorkspace->addColumn("int",m_logName);
    auto counts = outputWorkspace->addColumn("int","Counts");
    outputWorkspace->setRowCount(xLength); // One row per log value across the full range
    // Set type for benefit of MantidPlot
    logValues->setPlotType(1); // X
    counts->setPlotType(2);    // Y

    // Transfer the results to the table
    for ( int i = 0; i < xLength; ++i )
    {
      logValues->cell<int>(i) = minVal+i;
      counts->cell<int>(i) = Y[i];
    }

    // Columns for normalisation: monitors (if available), time & proton charge
    addMonitorCounts(outputWorkspace, log, minVal, maxVal);
    // Add a column to hold the time duration (in seconds) for which the log had a certain value
    auto timeCol = outputWorkspace->addColumn("double","time");
    // Add a column to hold the proton charge for which the log had a certain value
    auto protonChgCol = outputWorkspace->addColumn("double","proton_charge");
    // Get hold of the proton charge log for later
    const TimeSeriesProperty<double> * protonChargeLog = 0;
    try {
      protonChargeLog = m_inputWorkspace->run().getTimeSeriesProperty<double>("proton_charge");
    } catch (std::exception&) {
      // Log and carry on if not found. Column will be left empty.
      g_log.warning("proton_charge log not found in workspace.");
    }

    // Get a list of the other time-series logs in the input workspace
    auto otherLogs = getNumberSeriesLogs();
    // Add a column for each of these 'other' logs
    for ( auto it = otherLogs.begin(); it != otherLogs.end(); ++it )
    {
      auto newColumn = outputWorkspace->addColumn("double",it->first);
      // For the benefit of MantidPlot, set these columns to be containing X values
      newColumn->setPlotType(1);
    }

    // Now to get the average value of other time-varying logs
    // Loop through the values of the 'main' log
    for ( int value = minVal; value <= maxVal; ++value )
    {
      const int row = value-minVal;
      // Create a filter giving the times when this log has the current value
      TimeSplitterType filter;
      log->makeFilterByValue(filter,value,value); // min & max are the same of course
      // Calculate the time covered by this log value and add it to the table
      double duration = 0.0;
      for ( auto it = filter.begin(); it != filter.end(); ++it )
      {
        duration += it->duration();
      }
      timeCol->cell<double>(row) = duration;

      // Sum up the proton charge for this log value
      if ( protonChargeLog ) protonChgCol->cell<double>(row) = sumProtonCharge(protonChargeLog, filter);

      for ( auto log = otherLogs.begin(); log != otherLogs.end(); ++log )
      {
        // Calculate the average value of each 'other' log for the current value of the main log
        // Have to (maybe inefficiently) fetch back column by name - move outside loop if too slow
        outputWorkspace->getColumn(log->first)->cell<double>(row) = log->second->averageValueInFilter(filter);
      }
    }

    setProperty("OutputWorkspace",outputWorkspace);
  }

  void SumEventsByLogValue::filterEventList(const API::IEventList& eventList, const int minVal,
      const int maxVal, const Kernel::TimeSeriesProperty<int> * log, std::vector<int>& Y)
  {
    // TODO: Handle weighted events and avoid the vector copy below
    const auto pulseTimes = eventList.getPulseTimes();
    for ( std::size_t eventIndex = 0; eventIndex < pulseTimes.size(); ++eventIndex )
    {
      // Find the value of the log at the time of this event
      // This algorithm is really concerned with 'slow' logs so we don't care about
      // the time of the event within the pulse.
      // NB: If the pulse time is before the first log entry, we get the first value.
      const int logValue = log->getSingleValue( pulseTimes[eventIndex] );

      if ( logValue >= minVal && logValue <= maxVal )
      {
        // In this scenario it's easy to know what bin to increment
        PARALLEL_ATOMIC
        ++Y[logValue-minVal];
      }
    }
  }

  void SumEventsByLogValue::addMonitorCounts(ITableWorkspace_sptr outputWorkspace,
      const TimeSeriesProperty<int> * log, const int minVal, const int maxVal)
  {
    // See if there's a monitor workspace alongside the input one
    const std::string monitorWorkspaceName = m_inputWorkspace->name() + "_monitors";
    DataObjects::EventWorkspace_const_sptr monitorWorkspace;
    try {
      monitorWorkspace = AnalysisDataService::Instance().retrieveWS<DataObjects::EventWorkspace>(monitorWorkspaceName);
      // Check that we have an EventWorkspace for the monitors. If not, just return.
      if ( !monitorWorkspace )
      {
        g_log.warning() << "A monitor workspace (" << monitorWorkspaceName << ") was found, but "
            << "it is not an EventWorkspace so cannot be used in this algorithm.\n";
        return;
      }
    } catch (Exception::NotFoundError&) {
      // The monitors workspace isn't there - just return
      g_log.information() << "No monitor workspace (" << monitorWorkspaceName << ") found.\n";
      return;
    }

    const int xLength = maxVal - minVal + 1;
    // Loop over the spectra - there will be one per monitor
    for ( std::size_t spec = 0; spec < monitorWorkspace->getNumberHistograms(); ++spec )
    {
      // Create a column for this monitor
      const std::string monitorName = monitorWorkspace->getDetector(spec)->getName();
      auto monitorCounts = outputWorkspace->addColumn("int",monitorName);
      const IEventList & eventList = monitorWorkspace->getEventList(spec);
      // Accumulate things in a local vector before transferring to the table workspace
      std::vector<int> Y(xLength);
      filterEventList(eventList, minVal, maxVal, log, Y);
      // Transfer the results to the table
      for ( int i = 0; i < xLength; ++i )
      {
        monitorCounts->cell<int>(i) = Y[i];
      }
    }
  }

  std::vector<std::pair<std::string,const Kernel::ITimeSeriesProperty * >>
  SumEventsByLogValue::getNumberSeriesLogs()
  {
    std::vector<std::pair<std::string,const Kernel::ITimeSeriesProperty * >> numberSeriesProps;
    const auto & logs = m_inputWorkspace->run().getLogData();
    for ( auto log = logs.begin(); log != logs.end(); ++log )
    {
      const std::string logName = (*log)->name();
      // Don't add the log that's the one being summed against
      if ( logName == m_logName ) continue;
      // Exclude the proton charge log as we have a separate column for the sum of that
      if ( logName == "proton_charge") continue;
      // Try to cast to an ITimeSeriesProperty
      auto tsp = dynamic_cast<const ITimeSeriesProperty*>(*log);
      // Move on to the next one if this is not a TSP
      if ( tsp == NULL ) continue;
      // Don't keep ones with only one entry
      if ( tsp->realSize() < 2 ) continue;
      // Now make sure it's either an int or double tsp, and if so add log to the list
      if ( dynamic_cast<TimeSeriesProperty<double>* >(*log) || dynamic_cast<TimeSeriesProperty<int>* >(*log))
      {
        numberSeriesProps.push_back(std::make_pair(logName,tsp));
      }
    }

    return numberSeriesProps;
  }

  double SumEventsByLogValue::sumProtonCharge(const Kernel::TimeSeriesProperty<double> * protonChargeLog,
      const Kernel::TimeSplitterType& filter)
  {
    // Clone the proton charge log and filter the clone on this log value
    auto protonChargeLogClone = protonChargeLog->clone();
    protonChargeLogClone->filterByTimes(filter);
    // Seems like the only way to sum this is to yank out the values
    const std::vector<double> pcValues = protonChargeLogClone->valuesAsVector();
    // Delete the clone
    delete protonChargeLogClone;

    return std::accumulate(pcValues.begin(), pcValues.end(), 0.0);
  }

  template <typename T>
  void SumEventsByLogValue::createBinnedOutput(const Kernel::TimeSeriesProperty<T> * log)
  {
    // If only the number of bins was given, add the min & max values of the log
    if ( m_binningParams.size() == 1 )
    {
      m_binningParams.insert( m_binningParams.begin(), log->minValue() );
      m_binningParams.push_back(log->maxValue()*1.000001); // Make it a tiny bit larger to cover full range
    }
    MantidVec XValues;
    const int XLength = VectorHelper::createAxisFromRebinParams(m_binningParams, XValues);
    assert ( (int)XValues.size() == XLength );

    // Create the output workspace - the factory will give back a Workspace2D
    MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create("Workspace2D",1,XLength,XLength-1);
    // Copy the bin boundaries into the output workspace
    outputWorkspace->dataX(0) = XValues;
    // TODO: Add a label unit to the x axis, copy over y axis unit
    outputWorkspace->getAxis(0)->title() = m_logName;

    MantidVec & Y = outputWorkspace->dataY(0);
    const int numSpec = static_cast<int>(m_inputWorkspace->getNumberHistograms());
    Progress prog(this,0.0,1.0,numSpec);
    PARALLEL_FOR1(m_inputWorkspace)
    for ( int spec = 0; spec < numSpec; ++spec )
    {
      PARALLEL_START_INTERUPT_REGION
      const IEventList & eventList = m_inputWorkspace->getEventList(spec);
      // TODO: Handle weighted events and avoid the vector copy below
      const auto pulseTimes = eventList.getPulseTimes();
      for ( std::size_t eventIndex = 0; eventIndex < pulseTimes.size(); ++eventIndex )
      {
        // Find the value of the log at the time of this event
        const double logValue = log->getSingleValue( pulseTimes[eventIndex] );
        // TODO: Refactor getBinIndex to use a binary search and allow out-of-range values
        if ( logValue >= XValues.front() && logValue < XValues.back() )
        {
          PARALLEL_ATOMIC
          ++Y[VectorHelper::getBinIndex(XValues, logValue)];
        }
      }

      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    // For now, the errors are the sqrt of the counts. TODO: change as part of weighted event handling
    std::transform( Y.begin(),Y.end(),outputWorkspace->dataE(0).begin(), (double(*)(double)) std::sqrt );

    setProperty("OutputWorkspace",outputWorkspace);
  }


} // namespace Algorithms
} // namespace Mantid
