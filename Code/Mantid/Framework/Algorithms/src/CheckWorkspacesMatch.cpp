/*WIKI*


Compares two workspaces for equality. This algorithm is mainly intended for use by Mantid developers as part of the testing process.

The data values (X,Y and error) are always checked. The algorithm can also optionally check the axes (this includes the units), the spectra-detector map, the instrument (the name and parameter map) and any bin masking.

In the case of [[EventWorkspace]]s, they are checked to hold identical event lists. Comparisons between an EventList and a Workspace2D always fail.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CheckWorkspacesMatch.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include <sstream>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CheckWorkspacesMatch)

/// Sets documentation strings for this algorithm
void CheckWorkspacesMatch::initDocs()
{
  this->setWikiSummary("Compares two workspaces for equality. This algorithm is mainly intended for use by the Mantid development team as part of the testing process. ");
  this->setOptionalMessage("Compares two workspaces for equality. This algorithm is mainly intended for use by the Mantid development team as part of the testing process.");
}

/// Constructor
CheckWorkspacesMatch::CheckWorkspacesMatch() : API::Algorithm(), result(), prog(NULL)
{}

/// Virtual destructor
CheckWorkspacesMatch::~CheckWorkspacesMatch()
{
  delete prog;
}

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace Geometry;

/**
 * Process two groups and ensure the Result string is set properly on the final algorithm
 *
 * returns True if everything executed correctly
 */
bool CheckWorkspacesMatch::processGroups()
{
  Workspace_const_sptr w1 = getProperty("Workspace1");
  Workspace_const_sptr w2 = getProperty("Workspace2");

  WorkspaceGroup_const_sptr ws1 = boost::dynamic_pointer_cast<const WorkspaceGroup>(w1);
  WorkspaceGroup_const_sptr ws2 = boost::dynamic_pointer_cast<const WorkspaceGroup>(w2);
  this->result.clear();

  if( ws1 && ws2 ) // Both groups
  {
    processGroups(ws1, ws2);
  }
  else if(!ws1 && !ws2) // Neither groups (shouldn't happen)
  {
    throw std::runtime_error("CheckWorkspacesMatch::processGroups - Neither input is a WorkspaceGroup. This is a logical error in the code.");
  }
  else if( !ws1 || !ws2 )
  {
    this->result = "Type mismatch. One workspace is a group, the other is not.";
  }

  if( this->result.empty() )
  {
    this->result = this->successString();
  }
  else
  {
    g_log.notice() << this->result << "\n";
  }
  setProperty("Result", this->result);
  setExecuted(true);
  m_notificationCenter.postNotification(new FinishedNotification(this,this->isExecuted()));
  return true;
}

/**
 * Process the two groups together and set the result accordingly
 * @param groupOne :: Input group 1
 * @param groupTwo :: Input group 2
 */
void CheckWorkspacesMatch::processGroups(WorkspaceGroup_const_sptr groupOne, WorkspaceGroup_const_sptr groupTwo)
{
  // Check their sizes
  const size_t totalNum = static_cast<size_t>(groupOne->getNumberOfEntries());
  if( groupOne->getNumberOfEntries() != groupTwo->getNumberOfEntries() )
  {
    this->result = "GroupWorkspaces size mismatch.";
    return;
  }

  // See if there are any other properties that require setting
  const std::vector<Property*> & allProps = this->getProperties();
  std::vector<Property*> nonDefaultProps;
  nonDefaultProps.reserve(allProps.size());
  for(size_t i = 0; i < allProps.size(); ++i)
  {
    Property *p = allProps[i];
    const std::string & propName = p->name();
    // Skip those not set and the input workspaces
    if(p->isDefault() || propName == "Workspace1" || propName == "Workspace2" ) continue;
    nonDefaultProps.push_back(p);
  }
  const size_t numNonDefault = nonDefaultProps.size();

  const double progressFraction = 1.0/static_cast<double>(totalNum);
  std::vector<std::string> namesOne = groupOne->getNames();
  std::vector<std::string> namesTwo = groupTwo->getNames();
  for( size_t i = 0; i < totalNum; ++i )
  {
    // We should use an algorithm for each so that the output properties are reset properly
    Algorithm_sptr checker =  this->createChildAlgorithm(this->name(), progressFraction*(double)i, progressFraction*(double)(i+1), false, this->version());
    checker->setPropertyValue("Workspace1", namesOne[i]);
    checker->setPropertyValue("Workspace2", namesTwo[i]);
    for( size_t j = 0; j < numNonDefault; ++j )
    {
      Property *p = nonDefaultProps[j];
      checker->setPropertyValue(p->name(), p->value());
    }
    checker->execute();
    std::string success = checker->getProperty("Result");
    if(success != this->successString())
    {
      if(!this->result.empty()) this->result += "\n";
      this->result += success + ". Inputs=[" + namesOne[i] + "," + namesTwo[i] + "]";
    }
  }
}

void CheckWorkspacesMatch::init()
{
  declareProperty(new WorkspaceProperty<Workspace>("Workspace1","",Direction::Input), "The name of the first input workspace.");
  declareProperty(new WorkspaceProperty<Workspace>("Workspace2","",Direction::Input), "The name of the second input workspace.");

  declareProperty("Tolerance",0.0, "The maximum amount by which values may differ between the workspaces.");
  
  declareProperty("CheckType",true, "Whether to check that the data types (Workspace2D vs EventWorkspace) match.");
  declareProperty("CheckAxes",true, "Whether to check that the axes match.");
  declareProperty("CheckSpectraMap",true, "Whether to check that the spectra-detector maps match. ");
  declareProperty("CheckInstrument",true, "Whether to check that the instruments match. ");
  declareProperty("CheckMasking",true, "Whether to check that the bin masking matches. ");
  declareProperty("CheckSample",false, "Whether to check that the sample (e.g. logs).");    // Have this one false by default - the logs are brittle
  
  declareProperty("Result","",Direction::Output);

  declareProperty("ToleranceRelErr",false, "Treat tolerance as relative error rather then the absolute error");   
  declareProperty("CheckAllData",false, "Usually checking data ends when first mismatch occurs. This forces algorithm to check all data and print mismatch to the debug log.\n"\
                                        "Very often such logs are huge so making it true should be the last option.");    // Have this one false by default - it can be a lot of printing. 

  

}

void CheckWorkspacesMatch::exec()
{
  result.clear();
  m_ErrorIsRelative = getProperty("ToleranceRelerr");
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
  Workspace_sptr w1 = getProperty("Workspace1");
  Workspace_sptr w2 = getProperty("Workspace2");

  // Not implemented yet for table workspaces
  if ( w1->id() == "TableWorkspace" || w2->id() == "TableWorkspace" )
  {
    throw Kernel::Exception::NotImplementedError("This algorithm does not (yet) work for table workspaces");
  }

  // Check that both workspaces are the same type
  bool checkType = getProperty("CheckType");
  PeaksWorkspace_sptr tws1, tws2;
  tws1 = boost::dynamic_pointer_cast<PeaksWorkspace>(w1);
  tws2 = boost::dynamic_pointer_cast<PeaksWorkspace>(w2);
  if (checkType)
  {
    if ((tws1 && !tws2) ||(!tws1 && tws2))
    {
      result = "One workspace is an PeaksWorkspace and the other is not.";
      return;
    }
  }
  // Check some peak-based stuff
  if (tws1 && tws2)
  {
    // Check some table-based stuff
    if (tws1->getNumberPeaks() != tws2->getNumberPeaks())
    {
      result = "Mismatched number of rows.";
      return;
    }
    if (tws1->columnCount() != tws2->columnCount())
    {
      result = "Mismatched number of columns.";
      return;
    }

    // sort the workspaces before comparing
    {
      auto sortPeaks = createChildAlgorithm("SortPeaksWorkspace");
      sortPeaks->setProperty("InputWorkspace", tws1);
      sortPeaks->setProperty("ColumnNameToSortBy", "DSpacing");
      sortPeaks->setProperty("SortAscending", true);
      sortPeaks->executeAsChildAlg();
      IPeaksWorkspace_sptr temp = sortPeaks->getProperty("OutputWorkspace");
      tws1 = boost::dynamic_pointer_cast<PeaksWorkspace>(temp);

      sortPeaks = createChildAlgorithm("SortPeaksWorkspace");
      sortPeaks->setProperty("InputWorkspace", tws2);
      sortPeaks->setProperty("ColumnNameToSortBy", "DSpacing");
      sortPeaks->setProperty("SortAscending", true);
      sortPeaks->executeAsChildAlg();
      temp = sortPeaks->getProperty("OutputWorkspace");
      tws2 = boost::dynamic_pointer_cast<PeaksWorkspace>(temp);
    }

    const double tolerance = getProperty("Tolerance");
    for (int i =0; i<tws1->getNumberPeaks(); i++)
    {
      Peak & peak1 = tws1->getPeaks()[i];
      Peak & peak2 = tws2->getPeaks()[i];
      for (size_t j =0; j<tws1->columnCount(); j++)
      {
        boost::shared_ptr<const API::Column> col = tws1->getColumn(j);
        std::string name = col->name();
        double s1 = 0.0;
        double s2 = 0.0;
        if (name == "runnumber")
        {
          s1 = double(peak1.getRunNumber());
          s2 = double(peak2.getRunNumber());
        }
        else if (name == "detid")
        {
          s1 = double(peak1.getDetectorID());
          s2 = double(peak2.getDetectorID());
        }
        else if (name == "h")
        {
          s1 = peak1.getH();
          s2 = peak2.getH();
        }
        else if (name == "k")
        {
          s1 = peak1.getK();
          s2 = peak2.getK();
        }
        else if (name == "l")
        {
          s1 = peak1.getL();
          s2 = peak2.getL();
        }
        else if (name == "wavelength")
        {
          s1 = peak1.getWavelength();
          s2 = peak2.getWavelength();
        }
        else if (name == "energy")
        {
          s1 = peak1.getInitialEnergy();
          s2 = peak2.getInitialEnergy();
        }
        else if (name == "tof")
        {
          s1 = peak1.getTOF();
          s2 = peak2.getTOF();
        }
        else if (name == "dspacing")
        {
          s1 = peak1.getDSpacing();
          s2 = peak2.getDSpacing();
        }
        else if (name == "intens")
        {
          s1 = peak1.getIntensity();
          s2 = peak2.getIntensity();
        }
        else if (name == "sigint")
        {
          s1 = peak1.getSigmaIntensity();
          s2 = peak2.getSigmaIntensity();
        }
        else if (name == "bincount")
        {
          s1 = peak1.getBinCount();
          s2 = peak2.getBinCount();
        }
        else if (name == "row")
        {
          s1 = peak1.getRow();
          s2 = peak2.getRow();
        }
        else if (name == "col")
        {
          s1 = peak1.getCol();
          s2 = peak2.getCol();
        }
        if (std::fdim(s1, s2) > tolerance)
        {
          g_log.debug() << "Data mismatch at cell (row#,col#): (" << i << "," << j << ")\n";
          result = "Data mismatch";
          return;
        }
      }
    }
    return;
  }

  // Check things for IMDEventWorkspaces
  IMDEventWorkspace_const_sptr mdews1, mdews2;
  mdews1 = boost::dynamic_pointer_cast<const IMDEventWorkspace>(w1);
  mdews2 = boost::dynamic_pointer_cast<const IMDEventWorkspace>(w2);
  if (checkType)
  {
    if ((mdews1 && !mdews2) ||(!mdews1 && mdews2))
    {
      result = "One workspace is an IMDEventWorkspace and the other is not.";
      return;
    }
  }
  if (mdews1 && mdews2)
  {
    this->doMDComparison(w1, w2);
    return;
  }

  // Check things for IMDHistoWorkspaces
  IMDHistoWorkspace_const_sptr mdhws1, mdhws2;
  mdhws1 = boost::dynamic_pointer_cast<const IMDHistoWorkspace>(w1);
  mdhws2 = boost::dynamic_pointer_cast<const IMDHistoWorkspace>(w2);
  if (checkType)
  {
    if ((mdhws1 && !mdhws2) ||(!mdhws1 && mdhws2))
    {
      result = "One workspace is an IMDHistoWorkspace and the other is not.";
      return;
    }
  }
  if (mdhws1 && mdhws2)
  {
    this->doMDComparison(w1, w2);
    return;
  }

  MatrixWorkspace_const_sptr ws1, ws2;
  ws1 = boost::dynamic_pointer_cast<const MatrixWorkspace>(w1);
  ws2 = boost::dynamic_pointer_cast<const MatrixWorkspace>(w2);

  EventWorkspace_const_sptr ews1, ews2;
  ews1 = boost::dynamic_pointer_cast<const EventWorkspace>(ws1);
  ews2 = boost::dynamic_pointer_cast<const EventWorkspace>(ws2);
  if (checkType)
  {
    if ((ews1 && !ews2) ||(!ews1 && ews2))
    {
      result = "One workspace is an EventWorkspace and the other is not.";
      return;
    }
  }

  size_t numhist = ws1->getNumberHistograms();

  if (ews1 && ews2)
  {
    prog = new Progress(this, 0.0, 1.0, numhist*5);

    // Both will end up sorted anyway
    ews1->sortAll(PULSETIMETOF_SORT, prog);
    ews2->sortAll(PULSETIMETOF_SORT, prog);

    if (ews1->getNumberHistograms() != ews2->getNumberHistograms())
    {
      result = "Mismatched number of histograms.";
      return;
    }
    double ToleranceTOF = 0.05; //TODO should be smarter
    bool mismatchedEvent = false;
    int mismatchedEventWI = 0;
    //PARALLEL_FOR2(ews1, ews2)
    for (int i=0; i<static_cast<int>(ews1->getNumberHistograms()); i++)
    {
      PARALLEL_START_INTERUPT_REGION
          prog->report("EventLists");
      if (!mismatchedEvent) // This guard will avoid checking unnecessarily
      {
        const EventList &el1 = ews1->getEventList(i);
        const EventList &el2 = ews2->getEventList(i);
        if (!el1.equals(el2, ToleranceTOF, Tolerance, 1))
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
  if ( static_cast<bool>(getProperty("CheckSpectraMap")) && ! checkSpectraMap(ws1,ws2) ) return;
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
  const size_t numHists = ws1->getNumberHistograms();
  const size_t numBins = ws1->blocksize();
  const bool histogram = ws1->isHistogramData();
  bool checkAllData=getProperty("CheckAllData");


  
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
  for ( int i = 0; i < static_cast<int>(numHists); ++i )
  {
    PARALLEL_START_INTERUPT_REGION
        prog->report("Histograms");
    if (resultBool) // Avoid checking unnecessarily
    {

      // Get references to the current spectrum
      const MantidVec& X1 = ws1->readX(i);
      const MantidVec& Y1 = ws1->readY(i);
      const MantidVec& E1 = ws1->readE(i);
      const MantidVec& X2 = ws2->readX(i);
      const MantidVec& Y2 = ws2->readY(i);
      const MantidVec& E2 = ws2->readE(i);

      bool RelErr = m_ErrorIsRelative;
      for ( int j = 0; j < static_cast<int>(numBins); ++j )
      {
        bool err;
        if (RelErr)
        {
            double s1=0.5*(X1[j]+X2[j]);
            if (s1>tolerance)
                err = (std::fdim(X1[j],X2[j]) > tolerance*s1);
            else
                err = (std::fdim(X1[j],X2[j]) > tolerance);

            double s2=0.5*(Y1[j]+Y2[j]);
            if (s2>tolerance)
               err = ((std::fdim(Y1[j],Y2[j]) > tolerance*s2)||err);
            else
               err = ((std::fdim(Y1[j],Y2[j]) > tolerance)||err);


            double s3=0.5*(E1[j]+E2[j]);
            if (s3>tolerance)
               err = ((std::fdim(E1[j],E2[j]) > tolerance*s3)||err);
            else
               err = ((std::fdim(E1[j],E2[j]) > tolerance)||err);
        }
        else
            err = (std::fdim(X1[j],X2[j]) > tolerance || std::fdim(Y1[j],Y2[j]) > tolerance
                   || std::fdim(E1[j],E2[j]) > tolerance);

        if (err)
        {
          g_log.debug() << "Data mismatch at cell (hist#,bin#): (" << i << "," << j << ")\n";
          g_log.debug() << " Dataset #1 (X,Y,E) = (" << X1[j] << "," << Y1[j] << "," << E1[j] << ")\n";
          g_log.debug() << " Dataset #2 (X,Y,E) = (" << X2[j] << "," << Y2[j] << "," << E2[j] << ")\n";
          g_log.debug() << " Difference (X,Y,E) = (" << std::fdim(X1[j],X2[j]) << ","
                        << std::fdim(Y1[j],Y2[j]) << "," << std::fdim(E1[j],E2[j]) << ")\n";
          result = "Data mismatch";
          resultBool = checkAllData;
        }
      }

      // Extra one for histogram data
      if ( histogram && std::fdim(X1.back(), X2.back()) > tolerance )
      {
        result = "Data mismatch";
        resultBool = checkAllData;
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
    // Don't check spectra axis as that just takes it values from the ISpectrum (see checkSpectraMap)
    if ( ! ax1->isSpectra() && ! ax1->operator==(*ax2) )
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
/// @param ws1 :: the first sp det map
/// @param ws2 :: the second sp det map
/// @retval true The maps match
/// @retval false The maps do not match
bool CheckWorkspacesMatch::checkSpectraMap(MatrixWorkspace_const_sptr ws1, MatrixWorkspace_const_sptr ws2)
{
  if (ws1->getNumberHistograms() != ws2->getNumberHistograms())
  {
    result = "Number of spectra mismatch";
    return false;
  }

  for (size_t i=0; i<ws1->getNumberHistograms(); i++)
  {
    const ISpectrum * spec1 = ws1->getSpectrum(i);
    const ISpectrum * spec2 = ws2->getSpectrum(i);
    if (spec1->getSpectrumNo() != spec2->getSpectrumNo())
    {
      result = "Spectrum number mismatch";
      return false;
    }
    if (spec1->getDetectorIDs().size() != spec2->getDetectorIDs().size())
    {
      std::ostringstream out;
      out << "Number of detector IDs mismatch: " << spec1->getDetectorIDs().size() << " vs " << spec2->getDetectorIDs().size() << " at workspace index " << i;
      result = out.str();
      return false;
    }
    std::set<detid_t>::const_iterator it1 = spec1->getDetectorIDs().begin();
    std::set<detid_t>::const_iterator it2 = spec2->getDetectorIDs().begin();
    for (; it1 != spec1->getDetectorIDs().end(); ++it1, ++it2)
    {
      if (*it1 != *it2)
      {
        result = "Detector IDs mismatch";
        return false;
      }
    }
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

  if ( ws1_parmap != ws2_parmap )
  {
    g_log.debug() << "Parameter maps...\n";
    g_log.debug() << "WS1: " << ws1_parmap.asString() << "\n";
    g_log.debug() << "WS2: " << ws2_parmap.asString() << "\n";
    result = "Instrument ParameterMap mismatch (differences in ordering ignored)";
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
  const int numHists = static_cast<int>(ws1->getNumberHistograms());
  
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

void CheckWorkspacesMatch::doMDComparison(Workspace_sptr w1, Workspace_sptr w2)
{
  IMDWorkspace_sptr mdws1, mdws2;
  mdws1 = boost::dynamic_pointer_cast<IMDWorkspace>(w1);
  mdws2 = boost::dynamic_pointer_cast<IMDWorkspace>(w2);

  IAlgorithm_sptr alg = this->createChildAlgorithm("CompareMDWorkspaces");
  alg->setProperty<IMDWorkspace_sptr>("Workspace1", mdws1);
  alg->setProperty<IMDWorkspace_sptr>("Workspace2", mdws2);
  const double tolerance = getProperty("Tolerance");
  alg->setProperty("Tolerance", tolerance);
  alg->executeAsChildAlg();
  bool doesMatch = alg->getProperty("Equals");
  std::string algResult = alg->getProperty("Result");
  if (!doesMatch)
  {
    result = algResult;
  }
}

} // namespace Algorithms
} // namespace Mantid

