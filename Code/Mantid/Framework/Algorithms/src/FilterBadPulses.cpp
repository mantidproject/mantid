//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FilterBadPulses.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/V3D.h"

#include <fstream>

namespace Mantid
{
namespace Algorithms
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FilterBadPulses)

using namespace Kernel;
using namespace API;
using Geometry::IInstrument_const_sptr;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;

const double CONSTANT = (PhysicalConstants::h * 1e10) / (2.0 * PhysicalConstants::NeutronMass * 1e6);

//========================================================================
//========================================================================
/// (Empty) Constructor
FilterBadPulses::FilterBadPulses()
{
}

/// Destructor
FilterBadPulses::~FilterBadPulses()
{
}
/// Algorithm's name for identification overriding a virtual method
const std::string FilterBadPulses::name() const
{
  return "FilterBadPulses";
}

/// Algorithm's version for identification overriding a virtual method
int FilterBadPulses::version() const
{
  return 1;
}

/// Algorithm's category for identification overriding a virtual method
const std::string FilterBadPulses::category() const
{
  return "General";
}

//-----------------------------------------------------------------------
void FilterBadPulses::init()
{
  //this->setWikiSummary("Filters out events associated with pulses that happen when proton charge is outside of the supplied range.");
  //this->setOptionalMessage("Filters out events associated with pulses that happen when proton charge is outside of the supplied range.");

  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input, new API::EventWorkspaceValidator<MatrixWorkspace>),
    "An event workspace" );
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "The name to use for the output workspace" );
  BoundedValidator<double> *range = new BoundedValidator<double>();
  range->setBounds(0., 100.);
  declareProperty("LowerCutoff", 95., range,
                  "The percentage of the average to use as the lower bound");
}


//-----------------------------------------------------------------------
/** Executes the algorithm */
void FilterBadPulses::exec()
{
  // the input workspace into the event workspace we already know it is
  MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");

  // get the proton charge exists in the run object
  const API::Run& runlogs = inputWS->run();
  if (!runlogs.hasProperty("proton_charge"))
  {
    throw std::runtime_error("Failed to find \"proton_charge\" in sample logs");
  }
  Kernel::TimeSeriesProperty<double> * pcharge_log
      = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( runlogs.getLogData("proton_charge") );
  Kernel::TimeSeriesPropertyStatistics stats = pcharge_log->getStatistics();

  // set the range
  double min_percent = this->getProperty("LowerCutoff");
  min_percent *= .01; // convert it to a percentage (0<x<1)
  double min_pcharge = stats.mean * min_percent;
  double max_pcharge = stats.maximum * 1.1; // make sure everything high is in
  if (min_pcharge >= max_pcharge) {
    throw std::runtime_error("proton_charge window filters out all of the data");
  }
  this->g_log.information() << "Filtering pcharge outside of " << min_pcharge
                            << " to " << max_pcharge << std::endl;

  // sub-algorithme does all of the actual work - do not set the output workspace
  IAlgorithm_sptr filterAlgo = createSubAlgorithm("FilterByLogValue", 0., 1.);
  filterAlgo->setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS));
  filterAlgo->setProperty("LogName", "proton_charge");
  filterAlgo->setProperty("MinimumValue", min_pcharge);
  filterAlgo->setProperty("MaximumValue", max_pcharge);
  filterAlgo->execute();

  // just grab the child's output workspace
  MatrixWorkspace_sptr outputWS = filterAlgo->getProperty("OutputWorkspace");
  this->setProperty("OutputWorkspace", outputWS);
}


} // namespace Algorithms
} // namespace Mantid
