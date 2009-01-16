//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <fstream>
#include <sstream>
#include "MantidKernel/VectorHelper.h"
#include "MantidAlgorithms/CrossCorrelate.h"
#include <numeric>

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(CrossCorrelate)

    using namespace Kernel;
    using namespace API;

    // Get a reference to the logger
    Logger& CrossCorrelate::g_log = Logger::get("CrossCorrelate");

    /// Initialisation method.
    void CrossCorrelate::init()
    {
    	//Input and output workspaces
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input));
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output));

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      // Reference spectra against which cross correlation is performed
      declareProperty("ReferenceSpectra",0, mustBePositive);
      // Spectra in the range [min to max] will be cross correlated to reference.
      declareProperty("Spectra_min",0, mustBePositive->clone());
      declareProperty("Spectra_max",0, mustBePositive->clone());
      // Only the data in the range X_min, X_max will be used
      declareProperty("X_min",0.0);
      declareProperty("X_max",0.0);
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
   	inputWS->getAxis(1)->getSpectraIndexMap(index_map);
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

	// Now check if the range between x_min and x_max is valid
   	const std::vector<double>& referenceX=inputWS->dataX(index_ref);
   	double xmin=getProperty("X_min");
   	std::vector<double>::const_iterator minIt=std::find_if(referenceX.begin(),referenceX.end(),std::bind2nd(std::greater<double>(),xmin));
   	if (minIt==referenceX.end())
   		throw std::runtime_error("No data above X_min");

   	double xmax=getProperty("X_max");
   	std::vector<double>::const_iterator maxIt=std::find_if(minIt,referenceX.end(),std::bind2nd(std::greater<double>(),xmax));
   	if (minIt==maxIt)
   		throw std::runtime_error("Range is not valid");
   	//
   	std::vector<double>::difference_type difminIt=std::distance(referenceX.begin(),minIt);
   	std::vector<double>::difference_type difmaxIt=std::distance(referenceX.begin(),maxIt);

   	// Now loop on the spectra in the range spectra_min and spectra_max and get valid spectra

   	int specmin=getProperty("Spectra_min");
   	int specmax=getProperty("Spectra_max");
   	// Get the number of spectra in range specmin to specmax
   	int nspecs=0;
   	std::vector<int> indexes; // Indexes of all spectra in range
   	for (int i=specmin;i<=specmax;++i)
   	{
   		index_map_it=index_map.find(i);
   		if (index_map_it==index_map.end()) // Not in the map
   			continue; // Continue
   		indexes.push_back(index_map_it->second); // If spectrum found then add its index to a vector.
   		++nspecs;
   	}

   	std::ostringstream mess; // Use for log message

   	if (nspecs==0) // Throw if no spectra in range
	{
		mess<< "No spectra in range between" << specmin << " and " << specmax;
		throw std::runtime_error(mess.str());
	}

   	// Output message information
   	mess << "There are " << nspecs << " spectra in the range" << std::endl;
   	g_log.information(mess.str());
   	mess.str("");

	// Take a copy of  the reference spectrum
	const std::vector<double>& referenceY=inputWS->dataY(index_ref);
	const std::vector<double>& referenceE=inputWS->dataE(index_ref);
	int test=maxIt-minIt+1;
	mess<< "There are " << test << " bins";
	g_log.information(mess.str());
	mess.str("");

   	std::vector<double> refX(maxIt-minIt+1);
   	std::vector<double> refY(maxIt-minIt+1);
   	std::vector<double> refE(maxIt-minIt+1);

   	std::copy(minIt,maxIt,refX.begin());
   	std::copy(referenceY.begin()+difminIt,referenceY.begin()+difmaxIt,refY.begin());
   	std::copy(referenceE.begin()+difminIt,referenceE.begin()+difmaxIt,refE.begin());

   	// Now start the real stuff
	// Create a 2DWorkspace that will hold the result
   	const int nY=refY.size();
   	mess << "and here there are " << nY << " bins";
   	g_log.information(mess.str());
   	mess.str("");
	const int npoints=2*nY-3;
	MatrixWorkspace_sptr out= WorkspaceFactory::Instance().create(inputWS,nspecs,npoints,npoints);

   	// Calculate the mean value of the reference spectrum and associated error squared
	double refMean=std::accumulate(refY.begin(),refY.end(),0.0);
	double refMeanE2=std::accumulate(refE.begin(),refE.end(),0.0,SumSquares<double>());
	refMean/=static_cast<double>(nY);
	refMeanE2/=static_cast<double>(nY*nY);
    mess.str("");
    mess << "refMeanE2" << refMeanE2;
    g_log.information(mess.str());
    std::vector<double>::iterator itY=refY.begin();
	std::vector<double>::iterator itE=refE.begin();

	double refVar=0.0, refVarE=0.0;
	for (;itY!=refY.end();++itY,++itE)
	{
		(*itY)-=refMean; // Now the vector is (y[i]-refMean)
		(*itE)=(*itE)*(*itE)+refMeanE2; // New error squared
		double t=(*itY)*(*itY);
		refVar+=t;
		refVarE+=4.0*t*(*itE)*(*itE);
	}

	double refNorm=1.0/sqrt(refVar);
	double refNormE=0.5*pow(refNorm,3)*sqrt(refVarE);
	mess.str("");
	mess << "refNorm" << refNorm << "error" << refNormE;
	g_log.information(mess.str());
	mess << "Reference spectrum mean value: " << refMean << ", Variance: " << refVar;
	g_log.information(mess.str());

   	// Now copy the other spectra
   	bool is_distrib=inputWS->isDistribution();

   	std::vector<double> tempY(nY);
   	std::vector<double> tempE(nY);
   	std::vector<double> XX(npoints);
   	for (int i=0;i<npoints;++i)
   	{
   		XX[i]=static_cast<double>(i-nY+2);
   	}

   	for (int i=0;i<nspecs;++i) // Now loop on all spectra
   	{
   		int spec_index=indexes[i]; // Get the spectrum index from the table
   		//Copy spectra info from input Workspace
   		out->getAxis(1)->spectraNo(i)=inputWS->getAxis(1)->spectraNo(spec_index);
   		out->dataX(i)=XX;
   		// Get temp references
   		const std::vector<double>&  iX=inputWS->dataX(spec_index);
   		const std::vector<double>&  iY=inputWS->dataY(spec_index);
   		const std::vector<double>&  iE=inputWS->dataE(spec_index);
   		// Copy Y,E data of spec(i) to temp vector
   		// Now rebin on the grid of reference spectrum
   		rebin(iX,iY,iE,refX,tempY,tempE,is_distrib);
   		// Calculate the mean value of tempY
   		double tempMean=std::accumulate(tempY.begin(),tempY.end(),0.0);
   		tempMean/=static_cast<double>(nY);
   		double tempMeanE2=std::accumulate(tempE.begin(),tempE.end(),0.0,SumSquares<double>());
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
			tempVarE+=4.0*t*(*itE)*(*itE);
		}

		// Calculate the normalisation constant
		double tempNorm=1.0/sqrt(tempVar);
		double tempNormE=0.5*pow(tempNorm,3)*sqrt(tempVarE);
		double normalisation=refNorm*tempNorm;
		double normalisationE2=pow((refNorm*tempNormE),2)+pow((tempNorm*refNormE),2);
		// Get reference to the ouput spectrum
		std::vector<double>& outY=out->dataY(i);
		std::vector<double>& outE=out->dataE(i);

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
				err2+=pow((x*yE),2)+pow((y*xE),2);
			}
			outY[k+nY-2]=(val*normalisation);
			outE[k+nY-2]=sqrt(val*val*normalisationE2+normalisation*normalisation*err2);
		}
		// Update progress information
		double prog=static_cast<double>(i)/nspecs;
		progress(prog);
		interruption_point();
   	}

   	setProperty("OutputWorkspace",out);

   	return;
}


  } // namespace Algorithm
} // namespace Mantid
