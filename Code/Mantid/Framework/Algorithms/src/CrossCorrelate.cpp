//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <fstream>
#include <sstream>
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAlgorithms/CrossCorrelate.h"
#include <numeric>

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(CrossCorrelate)
    
    /// Sets documentation strings for this algorithm
    void CrossCorrelate::initDocs()
    {
      this->setWikiSummary("Cross-correlates a range of spectra against one reference spectra in the same workspace. ");
      this->setOptionalMessage("Cross-correlates a range of spectra against one reference spectra in the same workspace.");
    }
    

    using namespace Kernel;
    using namespace API;

    /// Initialisation method.
    void CrossCorrelate::init()
    {
      API::CompositeValidator<MatrixWorkspace> *wsValidator = new API::CompositeValidator<MatrixWorkspace>;
      wsValidator->add(new API::WorkspaceUnitValidator<MatrixWorkspace>("dSpacing"));
      wsValidator->add(new API::RawCountValidator<MatrixWorkspace>);

    	//Input and output workspaces
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input, wsValidator),
          "A 2D workspace with X values of d-spacing");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
          "The name of the output workspace");

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      // Reference spectra against which cross correlation is performed
      declareProperty("ReferenceSpectra",0, mustBePositive);
      // Spectra in the range [min to max] will be cross correlated to reference.
      declareProperty("WorkspaceIndexMin",0, mustBePositive->clone());
      declareProperty("WorkspaceIndexMax",0, mustBePositive->clone());
      // Only the data in the range X_min, X_max will be used
      declareProperty("XMin",0.0);
      declareProperty("XMax",0.0);
     }

    /** Executes the algorithm
     *
     *  @throw runtime_error Thrown if algorithm cannot execute
     */
void CrossCorrelate::exec()
{
  	MatrixWorkspace_const_sptr inputWS=getProperty("InputWorkspace");

  	//Get the map between spectra number and index
  	try{
      const SpectraAxis* axis = dynamic_cast<const SpectraAxis*>(inputWS->getAxis(1));
      if (!axis)
      {
        throw std::runtime_error("Input workspace has non-spectra axis.");
      }
      axis->getSpectraIndexMap(index_map);
  	}catch(std::runtime_error& error)
  	{
  		g_log.error(error.what());
  		throw;
  	}
  	// Check whether the reference spectra is valid
   	int reference=getProperty("ReferenceSpectra");
   	index_map_it=index_map.find(reference);
   	if (index_map_it==index_map.end()) // Not in the map
   		throw std::runtime_error("Can't find reference spectra");
   	const int index_ref=index_map_it->second;

   	// check that the data range specified makes sense
    double xmin=getProperty("XMin");
    double xmax=getProperty("XMax");
    if (xmin >= xmax)
      throw std::runtime_error("Must specify xmin < xmax");

   	// Now check if the range between x_min and x_max is valid
   	const MantidVec& referenceX=inputWS->dataX(index_ref);
    MantidVec::const_iterator minIt=std::find_if(referenceX.begin(),referenceX.end(),std::bind2nd(std::greater<double>(),xmin));
   	if (minIt==referenceX.end())
   		throw std::runtime_error("No data above XMin");
   	MantidVec::const_iterator maxIt=std::find_if(minIt,referenceX.end(),std::bind2nd(std::greater<double>(),xmax));
   	if (minIt==maxIt)
   		throw std::runtime_error("Range is not valid");

    MantidVec::difference_type difminIt=std::distance(referenceX.begin(),minIt);
   	MantidVec::difference_type difmaxIt=std::distance(referenceX.begin(),maxIt);

   	// Now loop on the spectra in the range spectra_min and spectra_max and get valid spectra

   	int specmin=getProperty("WorkspaceIndexMin");
   	int specmax=getProperty("WorkspaceIndexMax");
   	if (specmin>=specmax)
   	  throw std::runtime_error("Must specify WorkspaceIndexMin<WorkspaceIndexMax");
   	// Get the number of spectra in range specmin to specmax
   	int nspecs=0;
   	std::vector<int> indexes; // Indexes of all spectra in range
    indexes.reserve(specmax-specmin); //reserve at leat enough space
   	for (int i=specmin;i<=specmax;++i)
   	{
   		index_map_it=index_map.find(i);
   		if (index_map_it==index_map.end()) // Not in the map
   			continue; // Continue
   		indexes.push_back(index_map_it->second); // If spectrum found then add its index to a vector.
   		++nspecs;
   	}

    std::ostringstream mess;
	if (nspecs==0) // Throw if no spectra in range
	{
		mess<< "No Workspaces in range between" << specmin << " and " << specmax;
		throw std::runtime_error(mess.str());
	}

	// Output message information
	mess << "There are " << nspecs << " Workspaces in the range" << std::endl;
	g_log.information(mess.str());
	mess.str("");

	// Take a copy of  the reference spectrum
	const MantidVec& referenceY=inputWS->dataY(index_ref);
	const MantidVec& referenceE=inputWS->dataE(index_ref);

	std::vector<double> refX(maxIt-minIt);
	std::vector<double> refY(maxIt-minIt-1);
	std::vector<double> refE(maxIt-minIt-1);

	std::copy(minIt,maxIt,refX.begin());
	mess << "min max" << refX.front() << " " << refX.back();
	g_log.information(mess.str());
	mess.str("");
	std::copy(referenceY.begin()+difminIt,referenceY.begin()+(difmaxIt-1),refY.begin());
	std::copy(referenceE.begin()+difminIt,referenceE.begin()+(difmaxIt-1),refE.begin());

  // Now start the real stuff
	// Create a 2DWorkspace that will hold the result
 	const int nY=refY.size();
	const int npoints=2*nY-3;
	MatrixWorkspace_sptr out= WorkspaceFactory::Instance().create(inputWS,nspecs,npoints,npoints);

	// Calculate the mean value of the reference spectrum and associated error squared
	double refMean=std::accumulate(refY.begin(),refY.end(),0.0);
	double refMeanE2=std::accumulate(refE.begin(),refE.end(),0.0,VectorHelper::SumSquares<double>());
	refMean/=static_cast<double>(nY);
	refMeanE2/=static_cast<double>(nY*nY);
  std::vector<double>::iterator itY=refY.begin();
	std::vector<double>::iterator itE=refE.begin();

	double refVar=0.0, refVarE=0.0;
	for (;itY!=refY.end();++itY,++itE)
	{
		(*itY)-=refMean; // Now the vector is (y[i]-refMean)
		(*itE)=(*itE)*(*itE)+refMeanE2; // New error squared
		double t=(*itY)*(*itY); //(y[i]-refMean)^2
		refVar+=t;              // Sum previous term
		refVarE+=4.0*t*(*itE);  // Error squared
	}

	double refNorm=1.0/sqrt(refVar);
	double refNormE=0.5*pow(refNorm,3)*sqrt(refVarE);

	// Now copy the other spectra
	bool is_distrib=inputWS->isDistribution();

	std::vector<double> tempY(nY);
	std::vector<double> tempE(nY);
	std::vector<double> XX(npoints);
  for (int i=0;i<npoints;++i)
   		XX[i]=static_cast<double>(i-nY+2);
   // Initialise the progress reporting object
  m_progress = new Progress(this,0.0,1.0,nspecs);
  for (int i=0;i<nspecs;++i) // Now loop on all spectra
	{
		int spec_index=indexes[i]; // Get the spectrum index from the table
		//Copy spectra info from input Workspace
		out->getAxis(1)->spectraNo(i)=inputWS->getAxis(1)->spectraNo(spec_index);
		out->dataX(i)=XX;
		// Get temp references
		const MantidVec&  iX=inputWS->dataX(spec_index);
		const MantidVec&  iY=inputWS->dataY(spec_index);
		const MantidVec&  iE=inputWS->dataE(spec_index);
		// Copy Y,E data of spec(i) to temp vector
		// Now rebin on the grid of reference spectrum
		VectorHelper::rebin(iX,iY,iE,refX,tempY,tempE,is_distrib);
		// Calculate the mean value of tempY
		double tempMean=std::accumulate(tempY.begin(),tempY.end(),0.0);
		tempMean/=static_cast<double>(nY);
		double tempMeanE2=std::accumulate(tempE.begin(),tempE.end(),0.0,VectorHelper::SumSquares<double>());
		tempMeanE2/=static_cast<double>(nY*nY);
		//
		itY=tempY.begin();
		itE=tempE.begin();
		double tempVar=0.0, tempVarE=0.0;
		for (;itY!=tempY.end();++itY,++itE)
		{
			(*itY)-=tempMean; // Now the vector is (y[i]-refMean)
			(*itE)=(*itE)*(*itE)+tempMeanE2; // New error squared
			double t=(*itY)*(*itY);
			tempVar+=t;
			tempVarE+=4.0*t*(*itE);
		}

		// Calculate the normalisation constant
		double tempNorm=1.0/sqrt(tempVar);
		double tempNormE=0.5*pow(tempNorm,3)*sqrt(tempVarE);
		double normalisation=refNorm*tempNorm;
		double normalisationE2=pow((refNorm*tempNormE),2)+pow((tempNorm*refNormE),2);
		// Get reference to the ouput spectrum
		MantidVec& outY=out->dataY(i);
		MantidVec& outE=out->dataE(i);

		for (int k=-nY+2;k<=nY-2;++k)
		{
			int kp=abs(k);
			double val=0, err2=0, x, y, xE, yE;
			for (int j=nY-1-kp;j>=0;--j)
			{
				if (k>=0)
				{
					x=refY[j];
					y=tempY[j+kp];
					xE=refE[j];
					yE=tempE[j+kp];
				}
				else
				{
					x=tempY[j];
					y=refY[j+kp];
					xE=tempE[j];
					yE=refE[j+kp];
				}
				val+=(x*y);
				err2+=x*x*yE+y*y*xE;
			}
			outY[k+nY-2]=(val*normalisation);
			outE[k+nY-2]=sqrt(val*val*normalisationE2+normalisation*normalisation*err2);
		}
		// Update progress information
		//double prog=static_cast<double>(i)/nspecs;
		//progress(prog);
		m_progress->report();
		//interruption_point();
	}
   	setProperty("OutputWorkspace",out);
   	return;
}


  } // namespace Algorithm
} // namespace Mantid
