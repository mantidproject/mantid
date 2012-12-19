#include "MantidAlgorithms/SumEventsByLogValue.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Column.h"

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

    // TODO: Check for binning parameters if required & perhaps for string TSP
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
      //  TODO: Implement.
      //}
      else
      {
        throw std::runtime_error("This algorithm only supports number-series logs");
      }
    }
  }

  void SumEventsByLogValue::createTableOutput(const Kernel::TimeSeriesProperty<int> * log)
  {
    // This is the version for integer logs when not binning parameters have been given and has a data point per log value
    const int minVal = log->minValue();
    const int maxVal = log->maxValue();
    const int xLength = maxVal - minVal + 1;

    // Accumulate things in a local vector before transferring to the table
    std::vector<int> Y(xLength);
    // TODO: Parallelize
    for ( std::size_t spec = 0; spec < m_inputWorkspace->getNumberHistograms(); ++spec )
    {
      const IEventList & eventList = m_inputWorkspace->getEventList(spec);
      // TODO: Handle weighted events and avoid the vector copy below
      const auto pulseTimes = eventList.getPulseTimes();
      for ( std::size_t eventIndex = 0; eventIndex < pulseTimes.size(); ++eventIndex )
      {
        // Find the value of the log at the time of this event
        // TODO: If the pulse time is before the first log entry, we get the first value. Check that's what we want.
        const int logValue = log->getSingleValue( pulseTimes[eventIndex] );

        if ( logValue >= minVal && logValue <= maxVal )
        {
          // In this scenario it's easy to know what bin to increment
          ++Y[logValue-minVal];
        }
      }

    }
    // For now, no errors. Do we need them?

    // Create a table workspace to hold the sum.
    ITableWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().createTable();
    outputWorkspace->addColumn("int",m_logName);
    outputWorkspace->addColumn("int","Counts");
    // TODO: Columns for average value of other time-varying logs
    // TODO: Columns to enable normalisation
    outputWorkspace->setRowCount(xLength); // One row per log value across the full range

    // Get hold of the columns
    auto logValues = outputWorkspace->getColumn(m_logName);
    auto counts = outputWorkspace->getColumn("Counts");
    // Set type for benefit of MantidPlot
    logValues->setPlotType(1); // X
    counts->setPlotType(2);    // Y

    // Transfer the results to the table
    for ( int i = 0; i < xLength; ++i )
    {
      logValues->cell<int>(i) = minVal+i;
      counts->cell<int>(i) = Y[i];
    }

    setProperty("OutputWorkspace",outputWorkspace);
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
    // TODO: Parallelize & add progress reporting
    for ( std::size_t spec = 0; spec < m_inputWorkspace->getNumberHistograms(); ++spec )
    {
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
          ++Y[VectorHelper::getBinIndex(XValues, logValue)];
        }
      }

    }

    // For now, the errors are the sqrt of the counts. TODO: change as part of weighted event handling
    std::transform( Y.begin(),Y.end(),outputWorkspace->dataE(0).begin(), (double(*)(double)) std::sqrt );

    setProperty("OutputWorkspace",outputWorkspace);
  }


} // namespace Algorithms
} // namespace Mantid
