//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindPeaks1D.h"
#include "MantidDataObjects/TableRow.h"
#include <numeric>
namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindPeaks1D)

using namespace Kernel;
using namespace API;

// Get a reference to the logger. It is used to print out information, warning and error messages
Logger& FindPeaks1D::g_log = Logger::get("FindPeaks1D");

/// Constructor
FindPeaks1D::FindPeaks1D() : API::Algorithm(), peaks(new DataObjects::TableWorkspace) {}

void FindPeaks1D::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  BoundedValidator<int> *min = new BoundedValidator<int>();
  min->setLower(0);
  declareProperty("spectrum",0,min);
  BoundedValidator<int> *min2 = new BoundedValidator<int>();
  min2->setLower(1);
  declareProperty("smoothNpts",1,min->clone());
  declareProperty("smoothIter",1,min->clone());
  BoundedValidator<double> *mind = new BoundedValidator<double>();
  mind->setLower(1.0);
  declareProperty("Threashold",3.0,mind);

  // The found peaks in a table
  declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PeaksList","",Direction::Output));
  // Set up the columns for the TableWorkspace holding the peak information
  peaks->createColumn("double","centre");
  peaks->createColumn("double","width");
  peaks->createColumn("double","height");
  return;
}

void FindPeaks1D::retrieveProperties()
{
	input = getProperty("InputWorkspace");
	spec_number= getProperty("spectrum");
	if (spec_number>input->getNumberHistograms())
		throw std::runtime_error("FindPeaks1D, spectrum not valid");
	smooth_npts=getProperty("smoothNpts");
	smooth_iter=getProperty("smoothIter");
	threashold=getProperty("Threashold");
	return;
}

void FindPeaks1D::exec()
{
  // Retrieve all properties
  retrieveProperties();
  // Now run generalisedsecondDifference sub-algo
  generalisedSecondDifference();
  analyseVector();
  setProperty("PeaksList",peaks);
  return;
}

void FindPeaks1D::generalisedSecondDifference()
{
	Algorithm_sptr second_diff;
	  try
	  {
	    second_diff = createSubAlgorithm("GeneralisedSecondDifference");
	  }
	  catch (Exception::NotFoundError)
	  {
	    g_log.error("Can't locate GeneralisedSecondDifference");
	    throw;
	  }
	  second_diff->setProperty("InputWorkspace",input);
	  second_diff->setProperty("z",smooth_iter);
	  second_diff->setProperty("m",smooth_npts);
	  second_diff->setProperty("spectra_min",spec_number);
	  second_diff->setProperty("spectra_max",spec_number);

	    try {
	      second_diff->execute();
	    } catch (std::runtime_error) {
	      g_log.error("Unable to successfully run GeneralizedSecondDifference sub-algorithm");
	      throw;
	    }

	    if ( ! second_diff->isExecuted() )
	    {
	      g_log.error("Unable to successfully run GeneralizedSecondDifference sub-algorithm");
	      throw std::runtime_error("Unable to successfully run GeneralizedSecondDifference sub-algorithm");
	    }
	    second_diff_spec=second_diff->getProperty("OutputWorkspace");
  return;
}

void FindPeaks1D::analyseVector()
{

	return;

}
} // namespace Algorithms
} // namespace Mantid
