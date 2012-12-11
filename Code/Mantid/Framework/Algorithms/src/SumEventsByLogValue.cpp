#include "MantidAlgorithms/SumEventsByLogValue.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid
{
namespace Algorithms
{
  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(SumEventsByLogValue)

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
    declareProperty( new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
                              "The name of the workspace to be created as the output of the algorithm" );

    declareProperty( "LogName","",boost::make_shared<MandatoryValidator<std::string>>(),
                              "The name of the number series log against which the data should be summed" );
    declareProperty( new ArrayProperty<double>("OutputBinning","",boost::make_shared<RebinParamsValidator>(true)),
                              "Binning parameters for the output workspace (optional for integer typed logs)" );
  }

  void SumEventsByLogValue::exec()
  {
    DataObjects::EventWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");

    const std::string logName = getPropertyValue("LogName");
    // Get hold of the requested log. Will throw if it doesn't exist (which is what we want).
    const Property * const log = inputWorkspace->run().getLogData(logName);

    // Now we need to know what type of property it is
    const TimeSeriesProperty<int> * intLog;
    const TimeSeriesProperty<double> * dblLog;
    if ( ( intLog = dynamic_cast<const TimeSeriesProperty<int>*>(log) ) != NULL )
    {
      // This is the int log version that ignores binning parameters and has a data point per log value
      // TODO: add the ability to specify binning for integer logs
      
      const int minVal = intLog->minValue();
      const int maxVal = intLog->maxValue();
      const int xLength = maxVal - minVal + 1;
      // Create a point-like workspace to hold the sum. The factory will give back a Workspace2Ds
      MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace,1,xLength,xLength);
      // The X axis contains a point for each possible log value, in order.
      MantidVec & X = outputWorkspace->dataX(0);
      int val = minVal;
      for ( auto it = X.begin(); it != X.end(); ++it, ++val )  *it = val;
      // TODO: Add a label unit to the x axis, copy over y axis unit
      outputWorkspace->getAxis(0)->title() = logName;
  
      MantidVec & Y = outputWorkspace->dataY(0);
      // TODO: Parallelize
      for ( std::size_t spec = 0; spec < inputWorkspace->getNumberHistograms(); ++spec )
      {
        const IEventList & eventList = inputWorkspace->getEventList(spec);
        // TODO: Handle weighted events and avoid the vector copy below                                                              
        const auto pulseTimes = eventList.getPulseTimes();
        for ( std::size_t eventIndex = 0; eventIndex < pulseTimes.size(); ++eventIndex )
        {
          // Find the value of the log at the time of this event
          // TODO: If the pulse time is before the first log entry, we get the first value. Check that's what we want.
          const int logValue = intLog->getSingleValue( pulseTimes[eventIndex] );

          if ( logValue >= minVal && logValue <= maxVal )
          {
            // In this scenario it's easy to know what bin to increment
            ++Y[logValue-minVal];
            assert( X[logValue-minVal] == logValue );
          }
        }

      }
      // For now, the errors are the sqrt of the counts. TODO: change as part of weighted event handling 
      std::transform( Y.begin(),Y.end(),outputWorkspace->dataE(0).begin(), (double(*)(double)) std::sqrt );

      setProperty("OutputWorkspace",outputWorkspace);

    }
    else if ( ( dblLog = dynamic_cast<const TimeSeriesProperty<double>*>(log) ) != NULL )
    {
      std::vector<double> binningParams = getProperty("OutputBinning");
      // Binning parameters must be provided for floating point logs
      if ( binningParams.empty() )
      {
        throw std::invalid_argument("OutputBinning must be provided for floating-point number logs");
      }
      // If only the number of bins was given, add the min & max values of the log
      if ( binningParams.size() == 1 )
      {
        // TODO: What if min & max are the same (for example if there's only one entry in the property)
        binningParams.insert( binningParams.begin(), dblLog->minValue() );
        binningParams.push_back(dblLog->maxValue()*1.000001); // Make it a tiny bit larger to cover full range
      }
      MantidVec XValues;
      const int XLength = VectorHelper::createAxisFromRebinParams(binningParams, XValues);
      assert ( (int)XValues.size() == XLength );

      // Create the output workspace - the factory will give back a Workspace2D
      MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace,1,XLength,XLength-1);
      // Copy the bin boundaries into the output workspace
      outputWorkspace->dataX(0) = XValues;
      // TODO: Add a label unit to the x axis, copy over y axis unit
      outputWorkspace->getAxis(0)->title() = logName;

      MantidVec & Y = outputWorkspace->dataY(0);
      // TODO: Parallelize & add progress reporting
      for ( std::size_t spec = 0; spec < inputWorkspace->getNumberHistograms(); ++spec )
      {
        const IEventList & eventList = inputWorkspace->getEventList(spec);
        // TODO: Handle weighted events and avoid the vector copy below
        const auto pulseTimes = eventList.getPulseTimes();
        for ( std::size_t eventIndex = 0; eventIndex < pulseTimes.size(); ++eventIndex )
	{
          // Find the value of the log at the time of this event
          const double logValue = dblLog->getSingleValue( pulseTimes[eventIndex] );
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
    //else if ( dynamic_cast<const TimeSeriesProperty<std::string>*>(log) != NULL )
    //{
    //  TODO: Implement.
    //}
    else
    {
      throw std::runtime_error("This algorithm only supports number-series logs");
    }

  }


} // namespace Algorithms
} // namespace Mantid
