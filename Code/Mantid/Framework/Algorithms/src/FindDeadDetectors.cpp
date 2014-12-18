//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindDeadDetectors.h"
#include "MantidKernel/System.h"
#include <fstream>
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FindDeadDetectors)

using namespace Kernel;
using namespace API;

/// Initialisation method.
void FindDeadDetectors::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "Name of the input workspace");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Each histogram from the input workspace maps to a histogram in this\n"
      "workspace with one value that indicates if there was a dead detector");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);
  declareProperty(
      "DeadThreshold", 0.0, mustBePositive,
      "The threshold against which to judge if a spectrum belongs to a dead\n"
      "detector");
  // As the property takes ownership of the validator pointer, have to take care
  // to pass in a unique
  // pointer to each property.
  declareProperty(
      "LiveValue", 0.0, mustBePositive,
      "The value to assign to an integrated spectrum flagged as 'live'\n"
      "(default 0.0)");
  declareProperty(
      "DeadValue", 100.0, mustBePositive,
      "The value to assign to an integrated spectrum flagged as 'dead'\n"
      "(default 100.0)");
  // EMPTY_DBL() is a tag that tells us that no value has been set and we want
  // to use the default
  declareProperty(
      "RangeLower", EMPTY_DBL(),
      "No bin with a boundary at an x value less than this will be used\n"
      "in the summation that decides if a detector is 'dead' (default: the\n"
      "start of each histogram)");
  declareProperty(
      "RangeUpper", EMPTY_DBL(),
      "No bin with a boundary at an x value higher than this value will\n"
      "be used in the summation that decides if a detector is 'dead'\n"
      "(default: the end of each histogram)");
  declareProperty(
      "OutputFile", "",
      "A filename to which to write the list of dead detector UDETs");
  // This output property will contain the list of UDETs for the dead detectors
  declareProperty("FoundDead", std::vector<detid_t>(), Direction::Output);
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void FindDeadDetectors::exec() {
  double deadThreshold = getProperty("DeadThreshold");
  double liveValue = getProperty("LiveValue");
  double deadValue = getProperty("DeadValue");

  // Try and open the output file, if specified, and write a header
  std::ofstream file(getPropertyValue("OutputFile").c_str());
  file << "Index Spectrum UDET(S)" << std::endl;

  // Get the integrated input workspace
  MatrixWorkspace_sptr integratedWorkspace = integrateWorkspace();

  std::vector<detid_t> deadDets;
  int countSpec = 0, countDets = 0;

  // iterate over the data values setting the live and dead values
  g_log.information() << "Marking dead detectors" << std::endl;
  const int64_t numSpec = integratedWorkspace->getNumberHistograms();
  const double numSpec_d = static_cast<double>(numSpec);
  int64_t iprogress_step = numSpec / 100;
  if (iprogress_step == 0)
    iprogress_step = 1;
  for (int64_t i = 0; i < int64_t(numSpec); ++i) {
    // Spectrum in the integratedWorkspace
    ISpectrum *spec = integratedWorkspace->getSpectrum(i);
    double &y = spec->dataY()[0];
    if (y > deadThreshold) {
      y = liveValue;
    } else {
      ++countSpec;
      y = deadValue;
      const specid_t specNo = spec->getSpectrumNo();
      // Write the spectrum number to file
      file << i << " " << specNo;
      // Get the list of detectors for this spectrum and iterate over
      const std::set<detid_t> &dets = spec->getDetectorIDs();
      std::set<detid_t>::const_iterator it;
      for (it = dets.begin(); it != dets.end(); ++it) {
        // Write the detector ID to file, log & the FoundDead output property
        file << " " << *it;
        // we could write dead detectors to the log but if they are viewing the
        // log in the MantidPlot viewer it will crash MantidPlot
        deadDets.push_back(*it);
        ++countDets;
      }
      file << std::endl;
    }
    if (i % iprogress_step == 0) {
      progress(static_cast<double>(i) / numSpec_d);
      interruption_point();
    }
  }

  g_log.notice() << "Found a total of " << countDets
                 << " 'dead' detectors within " << countSpec
                 << " 'dead' spectra." << std::endl;

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", integratedWorkspace);
  setProperty("FoundDead", deadDets);

  // Close the output file
  file.close();
  return;
}

/// Run Integration as a Child Algorithm
MatrixWorkspace_sptr FindDeadDetectors::integrateWorkspace() {
  g_log.information() << "Integrating input workspace" << std::endl;

  API::IAlgorithm_sptr childAlg = createChildAlgorithm("Integration");
  // Now execute integration.
  // pass inputed values straight to Integration, checking must be done there
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace",
                                              getProperty("InputWorkspace"));
  childAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace",
                                              getProperty("OutputWorkspace"));
  childAlg->setProperty<double>("RangeLower", getProperty("RangeLower"));
  childAlg->setProperty<double>("RangeUpper", getProperty("RangeUpper"));
  childAlg->executeAsChildAlg();

  return childAlg->getProperty("OutputWorkspace");
}

} // namespace Algorithm
} // namespace Mantid
