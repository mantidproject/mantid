//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FindSXPeaks.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/Progress.h"


namespace Mantid
{
namespace Algorithms
{


double SXPeak::mN=1.67492729e-27;
double SXPeak::hbar=1.054571628e-34;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FindSXPeaks)

using namespace Kernel;
using namespace API;

/// Set the documentation strings
void FindSXPeaks::initDocs()
{
  this->setWikiSummary("Takes a 2D workspace as input and find the FindSXPeaksimum in each 1D spectrum. The algorithm creates a new 1D workspace containing all FindSXPeaksima as well as their X boundaries and error. This is used in particular for single crystal as a quick way to find strong peaks.");
  this->setOptionalMessage("Takes a 2D workspace as input and find the FindSXPeaksimum in each 1D spectrum. The algorithm creates a new 1D workspace containing all FindSXPeaksima as well as their X boundaries and error. This is used in particular for single crystal as a quick way to find strong peaks.");
}

/** Initialisation method.
 *
 */
void FindSXPeaks::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new HistogramValidator<>),
      "The name of the Workspace2D to take as input");
   declareProperty("RangeLower",EMPTY_DBL(),
      "The X value to search from (default 0)");
  declareProperty("RangeUpper",EMPTY_DBL(),
      "The X value to search to (default FindSXPeaks)");
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex",0, mustBePositive,
      "Start spectrum number (default 0)");
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndWorkspaceIndex",EMPTY_INT(), mustBePositive->clone(),
      "End spectrum number  (default FindSXPeaks)");
  declareProperty("SignalBackground",10.0);
  declareProperty("Resolution",0.01);
  declareProperty(new WorkspaceProperty<API::ITableWorkspace>("PeaksList","",Direction::Output),
      "The name of the TableWorkspace in which to store the list of peaks found" );

    // Set up the columns for the TableWorkspace holding the peak information
   m_peaks = WorkspaceFactory::Instance().createTable("TableWorkspace");
   m_peaks->addColumn("double","Qx");
   m_peaks->addColumn("double","Qy");
   m_peaks->addColumn("double","Qz");
   m_peaks->addColumn("double","Intensity");
   m_peaks->addColumn("int","NPixels");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void FindSXPeaks::exec()
{
// Try and retrieve the optional properties
  m_MinRange = getProperty("RangeLower");
  m_MaxRange = getProperty("RangeUpper");
  m_MinSpec = getProperty("StartWorkspaceIndex");
  m_MaxSpec = getProperty("EndWorkspaceIndex");
  double SB=getProperty("SignalBackground");

  // Get the input workspace
  MatrixWorkspace_const_sptr localworkspace = getProperty("InputWorkspace");

  const int numberOfSpectra = localworkspace->getNumberHistograms();

  // Check 'StartSpectrum' is in range 0-numberOfSpectra
  if ( m_MinSpec > numberOfSpectra )
  {
	g_log.warning("StartSpectrum out of range! Set to 0.");
	m_MinSpec = 0;
  }
  if ( isEmpty(m_MaxSpec) ) m_MaxSpec = numberOfSpectra-1;
  if ( m_MaxSpec > numberOfSpectra-1 || m_MaxSpec < m_MinSpec )
  {
	g_log.warning("EndSpectrum out of range! Set to max detector number");
	m_MaxSpec = numberOfSpectra;
  }
  if ( m_MinRange > m_MaxRange )
  {
	g_log.warning("Range_upper is less than Range_lower. Will integrate up to frame maximum.");
	m_MaxRange = 0.0;
  }

  Progress progress(this,0,1,(m_MaxSpec-m_MinSpec+1));

  // Calculate the primary flight path.
  //Mantid::Geometry::
  //Mantid::Kernel::
  Kernel::V3D sample=localworkspace->getInstrument()->getSample()->getPos();
// Mantid::Geometry::V3D sample=localworkspace->getInstrument()->getSample()->getPos();
  Kernel::V3D L1=sample-localworkspace->getInstrument()->getSource()->getPos();
//  Mantid::Geometry::V3D L1=sample-localworkspace->getInstrument()->getSource()->getPos();

  double l1=L1.norm();
  //

  peakvector entries;
  //PARALLEL_FOR1(localworkspace)
  // Loop over spectra
  for (int i = m_MinSpec; i <= m_MaxSpec; ++i)
  {
	//PARALLEL_START_INTERUPT_REGION
	// Retrieve the spectrum into a vector
	const MantidVec& X = localworkspace->readX(i);
	const MantidVec& Y = localworkspace->readY(i);

	// Find the range [min,max]
	MantidVec::const_iterator lowit, highit;
	if (m_MinRange == EMPTY_DBL()) lowit=X.begin();
	else lowit=std::lower_bound(X.begin(),X.end(),m_MinRange);

	if (m_MaxRange == EMPTY_DBL()) highit=X.end();
	else highit=std::find_if(lowit,X.end(),std::bind2nd(std::greater<double>(),m_MaxRange));

	// If range specified doesn't overlap with this spectrum then bail out
	if ( lowit == X.end() || highit == X.begin() ) continue;

	highit--; // Upper limit is the bin before, i.e. the last value smaller than MaxRange

	MantidVec::difference_type distmin=std::distance(X.begin(),lowit);
	MantidVec::difference_type distmax=std::distance(X.begin(),highit);

	// Find the max element
	MantidVec::const_iterator maxY=std::max_element(Y.begin()+distmin,Y.begin()+distmax);
	double intensity=(*maxY);
	double background=0.5*(1.0+Y.front()+Y.back());
	if (intensity<SB*background) // This is not a peak.
		continue;
	MantidVec::difference_type d=std::distance(Y.begin(),maxY);
	// t.o.f. of the peak
	double tof=0.5*(*(X.begin()+d)+*(X.begin()+d+1));

//	Geometry::IDetector_sptr det=localworkspace->getDetector(i);
	Geometry::IDetector_const_sptr det=localworkspace->getDetector(static_cast<size_t>(i));
	//Geometry::DetectorGroup_const_sptr det=localworkspace->getDetector(static_cast<size_t>(i))
//IDetector_sptr det=localworkspace->getDetector(static_cast<size_t>(i));

	double phi=det->getPhi();
	//
	if (phi<0)
		phi+=2.0*M_PI;
	//
	// double th2=det->getTwoTheta(Mantid::Geometry::V3D(0,0,0),Mantid::Geometry::V3D(0,0,1));
	double th2=det->getTwoTheta(Mantid::Kernel::V3D(0,0,0),Mantid::Kernel::V3D(0,0,1));

	std::vector<int> specs;
	specs.push_back(i);
	// Mantid::Geometry::V3D L2=det->getPos();
	Mantid::Kernel::V3D L2=det->getPos();
	L2-=sample;
	std::cout << "r,th,phi,t: " << L2.norm() << "," << th2*180/M_PI << "," << phi*180/M_PI << "," << tof << "\n";
	entries.push_back(SXPeak(tof,th2,phi,*maxY,specs,l1+L2.norm()));
	progress.report();
	//PARALLEL_END_INTERUPT_REGION
  }
  //PARALLEL_CHECK_INTERUPT_REGION
  // Now reduce the list with duplicate entries
  reducePeakList(entries);
  setProperty("PeaksList",m_peaks);
  progress.report();
  return;
}

void FindSXPeaks::reducePeakList(const peakvector& pcv)
{
	double resol=getProperty("Resolution");
	peakvector finalv;
	bool found=false;
	for (std::size_t i=0;i<pcv.size();i++)
	{
		for (std::size_t j=0;j<finalv.size();j++)
		{
			if(pcv[i].compare(finalv[j],resol))
			{
				finalv[j]+=pcv[i];
				found=true;
				break;
			}
		}
		if (!found)
			finalv.push_back(pcv[i]);
		found=false;
	}
	for (std::size_t i=0;i<finalv.size();i++)
	{
		finalv[i].reduce();
		Mantid::API::TableRow trow = m_peaks->appendRow();
		Mantid::Kernel::V3D Q=finalv[i].getQ();
		//Mantid::Geometry::V3D Q=finalv[i].getQ();
		        trow << Q[0] << Q[1] << Q[2] << finalv[i]._intensity << static_cast<int>(finalv[i]._spectral.size()) ;
	}


}

} // namespace Algorithms
} // namespace Mantid
