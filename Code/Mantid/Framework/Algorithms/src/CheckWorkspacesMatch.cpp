//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CheckWorkspacesMatch.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <sstream>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CheckWorkspacesMatch)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

void CheckWorkspacesMatch::init()
{
  this->setWikiSummary("Compares two workspaces for equality. This algorithm is mainly intended for use by the Mantid development team as part of the testing process.");
  this->setOptionalMessage("Compares two workspaces for equality. This algorithm is mainly intended for use by the Mantid development team as part of the testing process.");

  declareProperty(new WorkspaceProperty<>("Workspace1","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("Workspace2","",Direction::Input));

  declareProperty("Tolerance",0.0);
  
  declareProperty("CheckAxes",true);
  declareProperty("CheckSpectraMap",true);
  declareProperty("CheckInstrument",true);
  declareProperty("CheckMasking",true);
  declareProperty("CheckSample",false);    // Have this one false by default - the logs are brittle
  
  declareProperty("Result","",Direction::Output);
}

void CheckWorkspacesMatch::exec()
{
  result.clear();
  this->doComparison();
  
  if ( result != "") 
  {
    g_log.notice() << "The workspaces did not match: " << result << std::endl;
  }
  else
  {
    result = successString();
  }
  setProperty("Result",result);
  
  return;
}

///Perform the comparison
void CheckWorkspacesMatch::doComparison()
{
  MatrixWorkspace_const_sptr ws1 = getProperty("Workspace1");
  MatrixWorkspace_const_sptr ws2 = getProperty("Workspace2");

  // Check that both workspaces are the same type
  EventWorkspace_const_sptr ews1 = boost::dynamic_pointer_cast<const EventWorkspace>(ws1);
  EventWorkspace_const_sptr ews2 = boost::dynamic_pointer_cast<const EventWorkspace>(ws2);
  if ((ews1 && !ews2) ||(!ews1 && ews2))
  {
    result = "One workspace is an EventWorkspace and the other is not.";
    return;
  }

  int numhist = ws1->getNumberHistograms();

  // Check some event-based stuff
  if (ews1 && ews2)
  {
    prog = new Progress(this, 0.0, 1.0, numhist*5);

    // Both will end up sorted anyway
    ews1->sortAll(TOF_SORT, prog);
    ews2->sortAll(TOF_SORT, prog);

    if (ews1->getNumberHistograms() != ews2->getNumberHistograms())
    {
      result = "Mismatched number of histograms.";
      return;
    }
    bool mismatchedEvent = false;
    int mismatchedEventWI = 0;
    //PARALLEL_FOR2(ews1, ews2)
    for (int i=0; i<ews1->getNumberHistograms(); i++)
    {
      PARALLEL_START_INTERUPT_REGION
      prog->reportIncrement(1, "EventLists");
      if (!mismatchedEvent) // This guard will avoid checking unnecessarily
      {
        const EventList &el1 = ews1->getEventList(i);
        const EventList &el2 = ews2->getEventList(i);
        if (el1 != el2)
        {
          mismatchedEvent = true;
          mismatchedEventWI = i;
        }
      }
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    if ( mismatchedEvent)
    {
      std::ostringstream mess;
      mess << "Mismatched event list at workspace index " << mismatchedEventWI;
      result = mess.str();
      return;
    }
  }
  else
  {
    // Fewer steps if not events
    prog = new Progress(this, 0.0, 1.0, numhist*2);
  }


  // First check the data - always do this
  if ( ! checkData(ws1,ws2) ) return;
  
  // Now do the other ones if requested. Bail out as soon as we see a failure.
  prog->reportIncrement(numhist/5, "Axes");
  if ( static_cast<bool>(getProperty("CheckAxes")) && ! checkAxes(ws1,ws2) ) return;
  prog->reportIncrement(numhist/5, "SpectraMap");
  if ( static_cast<bool>(getProperty("CheckSpectraMap")) && ! checkSpectraMap(ws1->spectraMap(),ws2->spectraMap()) ) return;
  prog->reportIncrement(numhist/5, "Instrument");
  if ( static_cast<bool>(getProperty("CheckInstrument")) && ! checkInstrument(ws1,ws2) ) return;
  prog->reportIncrement(numhist/5, "Masking");
  if ( static_cast<bool>(getProperty("CheckMasking")) && ! checkMasking(ws1,ws2) ) return;
  prog->reportIncrement(numhist/5, "Sample");
  if ( static_cast<bool>(getProperty("CheckSample")) )
  {
    if( !checkSample(ws1->sample(), ws2->sample()) ) return;
    if( !checkRunProperties(ws1->run(), ws2->run()) ) return;
  } 
      
  return;
}

/// Checks that the data matches
/// @param ws1 :: the first workspace
/// @param ws2 :: the second workspace
/// @retval true The data matches
/// @retval false The data does not matches
bool CheckWorkspacesMatch::checkData(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2)
{
  // Cache a few things for later use
  const int numHists = ws1->getNumberHistograms();
  const int numBins = ws1->blocksize();
  const bool histogram = ws1->isHistogramData();
  
  // First check that the workspace are the same size
  if ( numHists != ws2->getNumberHistograms() || numBins != ws2->blocksize() )
  {
    result = "Size mismatch";
    return false;
  }
  
  // Check that both are either histograms or point-like data
  if ( histogram != ws2->isHistogramData() )
  {
    result = "Histogram/point-like mismatch";
    return false;
  }
  
  const double tolerance = getProperty("Tolerance");
  bool resultBool = true;
  
  // Now check the data itself
  //PARALLEL_FOR2(ws1, ws2)
  for ( int i = 0; i < numHists; ++i )
  {
    PARALLEL_START_INTERUPT_REGION
    prog->reportIncrement(1, "Histograms");
    if (resultBool) // Avoid checking unnecessarily
    {

      // Get references to the current spectrum
      const MantidVec& X1 = ws1->readX(i);
      const MantidVec& Y1 = ws1->readY(i);
      const MantidVec& E1 = ws1->readE(i);
      const MantidVec& X2 = ws2->readX(i);
      const MantidVec& Y2 = ws2->readY(i);
      const MantidVec& E2 = ws2->readE(i);

      for ( int j = 0; j < numBins; ++j )
      {
        if ( std::abs(X1[j]-X2[j]) > tolerance || std::abs(Y1[j]-Y2[j]) > tolerance || std::abs(E1[j]-E2[j]) > tolerance )
        {
          g_log.debug() << "Data mismatch at cell (hist#,bin#): (" << i << "," << j << ")\n";
          g_log.debug() << " Dataset #1 (X,Y,E) = (" << X1[j] << "," << Y1[j] << "," << E1[j] << ")\n";
          g_log.debug() << " Dataset #2 (X,Y,E) = (" << X2[j] << "," << Y2[j] << "," << E2[j] << ")\n";
          g_log.debug() << " Difference (X,Y,E) = (" << std::abs(X1[j]-X2[j]) << "," << std::abs(Y1[j]-Y2[j]) << "," << std::abs(E1[j]-E2[j]) << ")\n";
          result = "Data mismatch";
          resultBool = false;
        }
      }

      // Extra one for histogram data
      if ( histogram && std::abs(X1.back()-X2.back()) > tolerance )
      {
        result = "Data mismatch";
        resultBool = false;
      }
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  
  // If all is well, return true
  return resultBool;
}

/// Checks that the axes matches
/// @param ws1 :: the first workspace
/// @param ws2 :: the second workspace
/// @retval true The axes match
/// @retval false The axes do not match
bool CheckWorkspacesMatch::checkAxes(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2)
{
  const int numAxes = ws1->axes();
  
  if ( numAxes != ws2->axes() )
  {
    result = "Different numbers of axes";
    return false;
  }
  
  for ( int i = 0; i < numAxes; ++i )
  {
    std::ostringstream axis_name_ss;
    axis_name_ss << "Axis " << i;
    std::string axis_name = axis_name_ss.str();

    const Axis * const ax1 = ws1->getAxis(i);
    const Axis * const ax2 = ws2->getAxis(i);
    
    if ( ax1->isSpectra() != ax2->isSpectra() )
    {
      result = axis_name + " type mismatch";
      return false;
    }

    if ( ax1->title() != ax2->title() )
    {
      result = axis_name + " title mismatch";
      return false;
    }
    
    Unit_const_sptr ax1_unit = ax1->unit();
    Unit_const_sptr ax2_unit = ax2->unit();
    
    if ( (ax1_unit == NULL && ax2_unit != NULL) || (ax1_unit != NULL && ax2_unit == NULL) 
         || ( ax1_unit && ax1_unit->unitID() != ax2_unit->unitID() ) )
    {
      result = axis_name + " unit mismatch";
      return false;
    }
    
    // Use Axis's equality operator to check length and values
    if ( ! ax1->operator==(*ax2) )
    {
      result = axis_name + " values mismatch";
      return false;
    }
  }
  
  
  if ( ws1->YUnit() != ws2->YUnit() )
  {
    g_log.debug() << "YUnit strings : WS1 = " << ws1->YUnit() <<
                                    " WS2 = " << ws2->YUnit() << "\n";
    result = "YUnit mismatch";
    return false;
  }
  
  // Check both have the same distribution flag
  if ( ws1->isDistribution() != ws2->isDistribution() )
  {
    g_log.debug() << "Distribution flags: WS1 = " << ws1->isDistribution() <<
                                        " WS2 = " << ws2->isDistribution() << "\n";
    result = "Distribution flag mismatch";
    return false;
  }
  
  // Everything's OK with the axes
  return true;
}

/// Checks that the spectra maps match
/// @param map1 :: the first sp det map
/// @param map2 :: the second sp det map
/// @retval true The maps match
/// @retval false The maps do not match
bool CheckWorkspacesMatch::checkSpectraMap(const API::SpectraDetectorMap& map1, const API::SpectraDetectorMap& map2)
{
  // Use the SpectraDetectorMap::operator!= to check the maps
  if ( map1 != map2 )
  {
    result = "SpectraDetectorMap mismatch";
    return false;
  }
  
  // Everything's OK if we get to here
  return true;
}

/// Checks that the instruments match
/// @param ws1 :: the first workspace
/// @param ws2 :: the second workspace
/// @retval true The instruments match
/// @retval false The instruments do not match
bool CheckWorkspacesMatch::checkInstrument(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2)
{
  // First check the name matches
  if ( ws1->getInstrument()->getName() != ws2->getInstrument()->getName() )
  {
    g_log.debug() << "Instrument names: WS1 = " << ws1->getInstrument()->getName() <<
                                      " WS2 = " << ws2->getInstrument()->getName() << "\n";
    result = "Instrument name mismatch";
    return false;
  }
  
  const Geometry::ParameterMap& ws1_parmap = ws1->instrumentParameters();
  const Geometry::ParameterMap& ws2_parmap = ws2->instrumentParameters();
    
  if ( ws1_parmap.asString() != ws2_parmap.asString() )
  {
    g_log.debug() << "Parameter maps...\n";
    g_log.debug() << "WS1: " << ws1_parmap.asString() << "\n";
    g_log.debug() << "WS2: " << ws2_parmap.asString() << "\n";
    result = "Instrument ParameterMap mismatch";
    return false;
  }
  
  // All OK if we're here
  return true;
}

/// Checks that the masking matches
/// @param ws1 :: the first workspace
/// @param ws2 :: the second workspace
/// @retval true The masking matches
/// @retval false The masking does not match
bool CheckWorkspacesMatch::checkMasking(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2)
{
  const int numHists = ws1->getNumberHistograms();
  
  for ( int i = 0; i < numHists; ++i)
  {
    const bool ws1_masks = ws1->hasMaskedBins(i);
    if ( ws1_masks != ws2->hasMaskedBins(i) )
    {
      g_log.debug() << "Only one workspace has masked bins for spectrum " << i << "\n";
      result = "Masking mismatch";
      return false;
    }
    
    // If there are masked bins, check that they match
    if ( ws1_masks && ws1->maskedBins(i) != ws2->maskedBins(i) )
    {
      g_log.debug() << "Mask lists for spectrum " << i << " do not match\n";
      result = "Masking mismatch";
      return false;     
    }
  }
  
  // All OK if here
  return true;
}

/// Checks that the sample matches
/// @param sample1 :: the first sample
/// @param sample2 :: the second sample
/// @retval true The sample matches
/// @retval false The samples does not match
bool CheckWorkspacesMatch::checkSample(const API::Sample& sample1, const API::Sample& sample2)
{
  if ( sample1.getName() != sample2.getName() )
  {
    g_log.debug() << "WS1 sample name: " << sample1.getName() << "\n";
    g_log.debug() << "WS2 sample name: " << sample2.getName() << "\n";
    result = "Sample name mismatch";
    return false;
  }
  // N.B. Sample shape properties are not currently written out to nexus processed files, so omit here
  
  // All OK if here
  return true;
}

/// Checks that the Run matches
/// @param run1 :: the first run object
/// @param run2 :: the second run object
/// @retval true The sample matches
/// @retval false The samples does not match

bool CheckWorkspacesMatch::checkRunProperties(const API::Run& run1, const API::Run& run2)
{
  double run1Charge(-1.0);
  try
  {
    run1Charge = run1.getProtonCharge();
  }
  catch(Exception::NotFoundError&)
  {
  }
  double run2Charge(-1.0);
  try
  {
    run2Charge = run2.getProtonCharge();
  }
  catch(Exception::NotFoundError&)
  {
  }

  if ( run1Charge != run2Charge )
  {
    g_log.debug() << "WS1 proton charge: " << run1Charge << "\n";
    g_log.debug() << "WS2 proton charge: " << run2Charge << "\n";
    result = "Proton charge mismatch";
    return false;
  }
  
  const std::vector<Kernel::Property*>& ws1logs = run1.getLogData();
  const std::vector<Kernel::Property*>& ws2logs = run2.getLogData();
  // Check that the number of separate logs is the same
  if ( ws1logs.size() != ws2logs.size() )
  {
    g_log.debug() << "WS1 number of logs: " << ws1logs.size() << "\n";
    g_log.debug() << "WS2 number of logs: " << ws2logs.size() << "\n";
    result = "Different numbers of logs";
    return false;
  }
  
  // Now loop over the individual logs
  for ( size_t i = 0; i < ws1logs.size(); ++i )
  {
    // Check the log name
    if ( ws1logs[i]->name() != ws2logs[i]->name() )
    {
      g_log.debug() << "WS1 log " << i << " name: " << ws1logs[i]->name() << "\n";
      g_log.debug() << "WS2 log " << i << " name: " << ws2logs[i]->name() << "\n";
      result = "Log name mismatch";
      return false;
    }
    
    // Now check the log entry itself, using the method that gives it back as a string
    if ( ws1logs[i]->value() != ws2logs[i]->value() )
    {
      g_log.debug() << "WS1 log " << ws1logs[i]->name() << ": " << ws1logs[i]->value() << "\n";
      g_log.debug() << "WS2 log " << ws2logs[i]->name() << ": " << ws2logs[i]->value() << "\n";
      result = "Log value mismatch";
      return false;
    }
  }
  return true;
}


} // namespace Algorithms
} // namespace Mantid

