//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CrossCorrelate.h"
#include "MantidDataObjects/Workspace2D.h"
#include <fstream>
#include <sstream>

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
   	int reference=getProperty("ReferenceSpectra");
   	index_map_it=index_map.find(reference);
   	if (index_map_it==index_map.end()) // Not in the map
   		throw std::runtime_error("Can't find reference spectra");
   	const int index_ref=index_map_it->second;
   	// Now loop on the spectra
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
		mess.clear();
		mess<< "No spectra in range between" << specmin << " and " << specmax;
		throw std::runtime_error(mess.str());
	}

   	// Output message information
   	mess << "There are " << nspecs << " spectra in the range";
   	g_log.information(mess.str());

   	// Now start the real stuff
   	int nX=inputWS->readX(index_ref).size(); // Number of bins for spectra reference
   	//Create new workspace
   	bool add_reference=(reference<specmin || reference>specmax);
   	if  (add_reference)  // Reference should be included in list
   		nspecs++;
   	Workspace_sptr out= WorkspaceFactory::Instance().create(inputWS,nspecs,nX,nX-1);
   	setProperty("OutputWorkspace",out);
   	int offset=0;
   	// Copy reference if not in the list of spectra
   	if (add_reference)
   	{
   		out->getAxis(1)->spectraNo(0)=inputWS->getAxis(1)->spectraNo(index_ref);
   		out->dataX(0)=inputWS->dataX(index_ref);
   		out->dataY(0)=inputWS->dataY(index_ref);
   		out->dataE(0)=inputWS->dataE(index_ref);
   		offset=1;
   	} // Now copy the other spectra
   	for (int i=0;i<(nspecs-offset);++i) // Copy spectra in range from inputWS, including SpectraNo
   	{
   		int t_index=indexes[i];
   		int o_index=i+offset;
   		out->getAxis(1)->spectraNo(o_index)=inputWS->getAxis(1)->spectraNo(t_index);
   		out->dataX(o_index)=inputWS->dataX(t_index);
   		out->dataY(o_index)=inputWS->dataY(t_index);
   		out->dataE(o_index)=inputWS->dataE(t_index);
   	}







   	return;
}

  } // namespace Algorithm
} // namespace Mantid
