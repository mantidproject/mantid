//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindPeaks1D.h"
#include "MantidAPI/TableRow.h"
#include <numeric>
namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindPeaks1D)

using namespace Kernel;
using namespace API;

/// Constructor
FindPeaks1D::FindPeaks1D() : API::Algorithm() {}

void FindPeaks1D::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  // The found peaks in a table
  declareProperty(new WorkspaceProperty<API::ITableWorkspace>("PeaksList","",Direction::Output));
  // Set up the columns for the TableWorkspace holding the peak information
  peaks = WorkspaceFactory::Instance().createTable("TableWorkspace");
  peaks->addColumn("double","centre");
  peaks->addColumn("double","width");
  peaks->addColumn("double","height");
  BoundedValidator<int> *min = new BoundedValidator<int>();
  min->setLower(0);
  declareProperty("Spectrum",0,min);
  BoundedValidator<int> *min2 = new BoundedValidator<int>();
  min2->setLower(1);
  declareProperty("SmoothNPts",1,min->clone());
  declareProperty("SmoothIter",1,min->clone());
  BoundedValidator<double> *mind = new BoundedValidator<double>();
  mind->setLower(1.0);
  declareProperty("Threashold",3.0,mind);


  return;
}

void FindPeaks1D::retrieveProperties()
{
	input = getProperty("InputWorkspace");
	spec_number= getProperty("Spectrum");
	if (spec_number>input->getNumberHistograms())
		throw std::runtime_error("FindPeaks1D, spectrum not valid");
	smooth_npts=getProperty("SmoothNPts");
	smooth_iter=getProperty("SmoothIter");
	threashold=getProperty("Threashold");
	return;
}

void FindPeaks1D::exec()
{
  // Retrieve all properties
  retrieveProperties();
  // Now run generalisedsecondDifference sub-algo
  generalisedSecondDifference();
  // Now find peaks in the vector
  analyseVector();
  // Assign output
  setProperty("PeaksList",peaks);
  return;
}

void FindPeaks1D::generalisedSecondDifference()
{
	IAlgorithm_sptr second_diff_alg;
	  try
	  {
	    second_diff_alg = createSubAlgorithm("GeneralisedSecondDifference");
	  }
	  catch (Exception::NotFoundError)
	  {
	    g_log.error("Can't locate GeneralisedSecondDifference");
	    throw;
	  }
	  second_diff_alg->setProperty("InputWorkspace",input);
	  second_diff_alg->setProperty("z",smooth_iter);
	  second_diff_alg->setProperty("m",smooth_npts);
	  second_diff_alg->setProperty("spectra_min",spec_number);
	  second_diff_alg->setProperty("spectra_max",spec_number);

	    try {
	      second_diff_alg->execute();
	    } catch (std::runtime_error) {
	      g_log.error("Unable to successfully run GeneralizedSecondDifference sub-algorithm");
	      throw;
	    }

	    if ( ! second_diff_alg->isExecuted() )
	    {
	      g_log.error("Unable to successfully run GeneralizedSecondDifference sub-algorithm");
	      throw std::runtime_error("Unable to successfully run GeneralizedSecondDifference sub-algorithm");
	    }
	    // Everything is fine, get the second_difference
	    second_diff_spec=second_diff_alg->getProperty("OutputWorkspace");
	    return;
}

void FindPeaks1D::analyseVector()
{
	typedef std::vector<double> Vector;

	Vector& X=second_diff_spec->dataX(0);
	Vector& Y=second_diff_spec->dataY(0);
	Vector& E=second_diff_spec->dataE(0);
	Vector& obs=input->dataY(spec_number);

	// Number of points chopped at the beginning and end of the second-difference spectrum with
	// respect to the initial data(obs).
	int n_chop=smooth_iter*smooth_npts+1;

	// Calculate Y divided by E.
	Vector YoE(Y.size());
	std::transform(Y.begin(),Y.end(),E.begin(),YoE.begin(),std::divides<double>());

	Vector::iterator left=YoE.begin(),right=YoE.begin(),center;
	typedef Vector::reverse_iterator rit;
	rit rleft;
	Vector::difference_type d;
	double peak_center, peak_width, peak_amplitude;
	int counter=0;
	std::ostringstream mess;

	for(;;) // Find the peaks
	{
		left=std::find_if(left,YoE.end(),std::bind2nd(std::less_equal<double>(),-threashold));
		if (left==YoE.end())
			break; // No more peaks
		rleft=std::find_if(rit(left),rit(right),std::bind2nd(std::greater<double>(),1.0));
		if (rleft==rit(right))
			continue; // Not a peak
		d=std::distance(rleft,YoE.rend());
		peak_width=-*(X.begin()+d+1); //Left FWHM
		right=std::find_if(left,YoE.end(),std::bind2nd(std::greater<double>(),1.0));
		if (right==YoE.end())
			break; //Not a peak
		d=std::distance(YoE.begin(),right);
		peak_width+=*(X.begin()+d); //Right FWHM
		center=std::min_element(left,right);
		d=std::distance(YoE.begin(),center);
		peak_center=*(X.begin()+d);
		peak_amplitude=*(obs.begin()+d+n_chop);
		mess << "Find one peak at Pos:" << peak_center << ",A=" <<  peak_amplitude << "W=" << peak_width;
		g_log.information(mess.str());
		mess.str("");
		API::TableRow newrow = peaks->appendRow();
		newrow << peak_center << peak_amplitude << peak_width;
		left=right;
		counter++;
	}
	mess.str("");
	mess << " Total number of peaks " << counter;
	g_log.information(mess.str());
return;

}
} // namespace Algorithms
} // namespace Mantid
