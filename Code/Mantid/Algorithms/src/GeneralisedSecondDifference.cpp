//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <sstream>
#include <numeric>
#include "MantidKernel/VectorHelper.h"


#include "MantidAlgorithms/GeneralisedSecondDifference.h"



namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(GeneralisedSecondDifference)

    using namespace Kernel;
    using namespace API;

    // Get a reference to the logger
    Logger& GeneralisedSecondDifference::g_log = Logger::get("GeneralisedSecondDifference");

    /// Constructor
    GeneralisedSecondDifference::GeneralisedSecondDifference(): Algorithm(),Cij(0),Cij2(0),z(0),m(0)
    {
    }
    /// Destructor
    GeneralisedSecondDifference::~GeneralisedSecondDifference()
    {
    }
    /// Initialisation method.
    void GeneralisedSecondDifference::init()
    {

      //Input and output workspaces
      declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input));
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("z",0,mustBePositive);
      declareProperty("m",0,mustBePositive->clone());
      declareProperty("spectra_min",0,mustBePositive->clone());
      declareProperty("spectra_max",0,mustBePositive->clone());
     }

    /** Executes the algorithm
     *
     *  @throw runtime_error Thrown if algorithm cannot execute
     */
void GeneralisedSecondDifference::exec()
{
	// Message stream used for logger
	std::ostringstream message;

	// Get some properties
  	Workspace_const_sptr inputWS=getProperty("InputWorkspace");
  	int spec_min=getProperty("spectra_min");
  	int spec_max=getProperty("spectra_max");
	if (spec_min>spec_max)
		std::swap(spec_min,spec_max);

	if (spec_max>inputWS->getNumberHistograms())
	{
		message << "The value of spectra_max "<< spec_max << "is not valid";
		throw std::runtime_error(message.str());
		message.str("");
	}

	// Get some more input fields
  	z=getProperty("z");
  	m=getProperty("m");
  	const int n_av=z*m+1;

  	// Calculate thre Cij and Cij^2 coefficients
  	computePrefactors();

	const int n_specs=spec_max-spec_min+1;
	const int n_points=inputWS->dataY(0).size()-2*n_av;
  	//Create OuputWorkspace
  	Workspace_sptr out= WorkspaceFactory::Instance().create(inputWS,n_specs,n_points+1,n_points);

	const int nsteps=2*n_av+1;

  	for (int i=spec_min;i<=spec_max;i++)
  	{
  		int out_index=i-spec_min;
  		out->getAxis(1)->spectraNo(out_index)=inputWS->getAxis(1)->spectraNo(i);
  		const std::vector<double>& refX=inputWS->dataX(i);
  		const std::vector<double>& refY=inputWS->dataY(i);
  		const std::vector<double>& refE=inputWS->dataE(i);
  		std::vector<double>& outX=out->dataX(out_index);
  		std::vector<double>& outY=out->dataY(out_index);
  		std::vector<double>& outE=out->dataE(out_index);

  		std::copy(refX.begin()+n_av,refX.end()-n_av,outX.begin());
  		std::vector<double>::const_iterator itInY=refY.begin();
  		std::vector<double>::iterator itOutY=outY.begin();
  		std::vector<double>::const_iterator itInE=refE.begin();
  		std::vector<double>::iterator itOutE=outE.begin();
  		double err2;
  		for (;itOutY!=outY.end();itOutY++,itInY++,itOutE++,itInE++)
  		{
  			//Calculate \sum_{j}Cij.Y(j)
  			(*itOutY)=std::inner_product(itInY,itInY+nsteps,Cij.begin(),0.0);
  			//Calculate the error bars sqrt(\sum_{j}Cij^2.E^2)
  			err2=std::inner_product(itInE,itInE+nsteps,Cij2.begin(),0.0);
  			(*itOutE)=sqrt(err2);
  		}
  	}
  	setProperty("OutputWorkspace",out);
  	return;
}
/** Compute the Cij
 *
 */
void GeneralisedSecondDifference::computePrefactors()
{
	int zz=0;
	int max_index_prev=1;
	int n_el_prev=3;
	std::vector<double> previous(n_el_prev);
	previous[0]=1;
	previous[1]=-2;
	previous[2]=1;

	if (z==0) //
	{
		Cij.resize(3);
		std::copy(previous.begin(),previous.end(),Cij.begin());
		Cij2.resize(3);
		std::transform(Cij.begin(),Cij.end(),Cij2.begin(),Squares<double>());
		return;
	}
	std::vector<double> next;
	// Calculate the Cij iteratively.
	do
	{
	zz++;
	int max_index=zz*m+1;
	int n_el=2*max_index+1;
	next.resize(n_el);
	std::fill(next.begin(),next.end(),0.0);
	for (int i=0;i<n_el;++i)
	{
		int delta=-max_index+i;
		for (int l=delta-m;l<=delta+m;l++)
		{
			int index=l+max_index_prev;
			if (index>=0 && index<n_el_prev)
				next[i]+=previous[index];
		}
	}
	previous.resize(n_el);
	std::copy(next.begin(),next.end(),previous.begin());
	max_index_prev=max_index;
	n_el_prev=n_el;
	}while(zz!=z);


	Cij.resize(2*z*m+3);
	std::copy(previous.begin(),previous.end(),Cij.begin());
	Cij2.resize(2*z*m+3);
	std::transform(Cij.begin(),Cij.end(),Cij2.begin(),Squares<double>());
	return;
}



  } // namespace Algorithm
} // namespace Mantid
