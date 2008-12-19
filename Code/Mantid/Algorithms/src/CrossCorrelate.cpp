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
      declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input));
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      // Reference spectra against which cross correlation is performed
      declareProperty("ReferenceSpectra",0, mustBePositive);
      // Spectra in the range [min to max] will be cross correlated to reference.
      declareProperty("Spectra_min",0, mustBePositive->clone());
      declareProperty("Spectra_max",0, mustBePositive->clone());
     }

    /** Executes the algorithm
     *
     *  @throw runtime_error Thrown if algorithm cannot execute
     */
void CrossCorrelate::exec()
{
  	Workspace_const_sptr inputWS=getProperty("InputWorkspace");

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
   		indexes.push_back(index_map_it->second); // If spectrum found then add its index.
   		++nspecs;
   	}
   	std::ostringstream mess;
   	if (nspecs==0) // Throw if no spectra in range
	{
		mess<< "No spectra in range between" << specmin << " and " << specmax;
		throw std::runtime_error(mess.str());
	}

   	// Output message information
   	mess << "There are " << nspecs << " spectra in the range" << std::endl;
   	g_log.information(mess.str());

    const 	int nY=inputWS->readY(index_ref).size(); // Number of Yvalues for spectra reference

    // Now start the real stuff
    // Create a 2DWorkspace that will hold the result
    const int npoints=2*nY-3;
	Workspace_sptr out= WorkspaceFactory::Instance().create("Workspace2D",nspecs,npoints,npoints);

	// Take a copy of  the reference spectrum
   	std::vector<double> refX=inputWS->dataX(index_ref);
   	std::vector<double> refY=inputWS->dataY(index_ref);
   	std::vector<double> refE=inputWS->dataE(index_ref);

   	// Calculate the mean value of the reference spectrum
	double refMean=std::accumulate(refY.begin(),refY.end(),0);
	refMean/=static_cast<double>(nY);

	std::vector<double>::iterator it;
	double refVar=0.0;
	for (it=refY.begin();it!=refY.end();++it)
	{
		(*it)-=refMean; // Now the vector is (y[i]-refMean)
		refVar+=(*it)*(*it);
	}
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
   		double tempMean=std::accumulate(tempY.begin(),tempY.end(),0);
   		tempMean/=static_cast<double>(nY);
   		//
   		double tempVar=0.0;
		for (it=tempY.begin();it!=tempY.end();++it)
		{
			(*it)-=tempMean; // Now the vector is (y[i]-refMean)
			tempVar+=(*it)*(*it);
		}
		double normalisation=1.0/sqrt(refVar*tempVar);
		std::vector<double>& outY=out->dataY(i);
		for (int k=-nY+2;k<=nY-2;++k)
		{
			int kp=abs(k);
			double val=0;
			for (int j=nY-1-kp;j>=0;--j)
			{
				(k>=0) ? val+=(refY[j]*tempY[j+kp]) : val+=(tempY[j]*refY[j+kp]);
			}
			outY[k+nY-2]=(val*normalisation);
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
