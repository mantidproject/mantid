/**WIKI*
== Description ==

SliceMDHisto extracts a hyperslab of data from a MDHistoWorkspace. Beyond 
the usual input and output workspace parameters, the start and end of the
hyperslabs dimensions are required. Both  as comma separated lists with an 
entry for each dimension of the MDHistoWorkspace. 

Example: consider an input MDHistoWorkspace with dimensions 100,100,100. 
Running SliceMDHisto with parameters Start= 20,20,20 and End= 50,50,100 
will copy all the data between x: 20-50, y: 20-50, z:20-100 into the 
result MDHistoWorkspace with dimensions 30,30,80.
*WIKI*/

/**
 * This algorithm takes a MDHistoWorkspace and allows to select a slab out of
 * it which is storeed into the result workspace.
 *
 * copyright: do not bother me or use mantid copyright
 *
 * Mark Koennecke, November 2012
 */
#include "MantidSINQ/SliceMDHisto.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/ArrayProperty.h"

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SliceMDHisto)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;
using namespace Mantid;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void SliceMDHisto::init()
{
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace","",Direction::Input));
    declareProperty(new ArrayProperty<int>("Start"),"A comma separated list of min,for each dimension");
    declareProperty(new ArrayProperty<int>("End"),"A comma separated list of max for each dimension");
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("OutputWorkspace","",Direction::Output));

}

void SliceMDHisto::exec()
{
	IMDHistoWorkspace_sptr inWS = IMDHistoWorkspace_sptr(getProperty("InputWorkspace"));
	rank = inWS->getNumDims();
	for(int i = 0; i < rank; i++){
		boost::shared_ptr<const IMDDimension> arDim = inWS->getDimension(i);
		dim.push_back(arDim->getNBins());
	}

    std::vector<int> start = getProperty("Start");
    std::vector<int> end = getProperty("End");

    // some checks
    if(start.size() < rank || end.size() < rank){
    	throw std::runtime_error("Start and end need to be given for each dimension of the dataset");
    }
    for(int i = 0; i < rank; i++){
    	if(start[i] < 0) {
    		start[i] = 0;
    	}
    	if(start[i] > dim[i]){
    		start[i] = dim[i];
    	}
       	if(end[i] < 0) {
        	end[i] = 0;
        }
        if(end[i] >= dim[i]){
        	end[i] = dim[i];
        }
        if(end[i] < start[i]){
        	throw std::runtime_error("End must be larger then start for each dimension");
        }
    }

    // create the new dadaset
    std::vector<MDHistoDimension_sptr> dimensions;
    for(int k = 0; k < rank; ++k)
    {
  	  boost::shared_ptr<const IMDDimension> arDim = inWS->getDimension(k);
      dimensions.push_back(MDHistoDimension_sptr(
    		  new MDHistoDimension(arDim->getName(),arDim->getName(), arDim->getUnits(), arDim->getX(start[k]), arDim->getX(end[k]), end[k] -start[k])));
    }
    MDHistoWorkspace_sptr outWS (new MDHistoWorkspace(dimensions));

    coord_t *sourceDim = (coord_t *)malloc(rank*sizeof(coord_t));
    coord_t *targetDim = (coord_t *)malloc(rank*sizeof(coord_t));
    cutData(inWS, outWS, sourceDim, targetDim, start, end, 0);
    free(sourceDim);
    free(targetDim);


    // assign the workspace
    copyMetaData(inWS,outWS);
	setProperty("OutputWorkspace",outWS);
}

void SliceMDHisto::cutData(Mantid::API::IMDHistoWorkspace_sptr inWS,
		  Mantid::API::IMDHistoWorkspace_sptr outWS,
		  Mantid::coord_t *sourceDim, Mantid::coord_t *targetDim,
		  std::vector<int> start, std::vector<int> end, int dim)
{
	int length;
	signal_t val;
	size_t idx;

	boost::shared_ptr<const IMDDimension> inDim = inWS->getDimension(dim);
	boost::shared_ptr<const IMDDimension> outDim = outWS->getDimension(dim);
	length = end[dim] - start[dim];
	if(dim == rank -1){
		MDHistoWorkspace_sptr outWSS = boost::dynamic_pointer_cast<MDHistoWorkspace>(outWS);
		for(int i = 0; i < length; i++){
			sourceDim[dim] = inDim->getX(start[dim]+i);
			val = inWS->getSignalAtCoord(sourceDim, static_cast<Mantid::API::MDNormalization>(0));
			targetDim[dim] = outDim->getX(i);
			idx = outWSS->getLinearIndexAtCoord(targetDim);
			outWS->setSignalAt(idx,val);
			outWS->setErrorSquaredAt(idx,val);
		}
	} else {
		for(int i = 0; i < length; i++){
			sourceDim[dim] = inDim->getX(start[dim]+i);
			targetDim[dim] = outDim->getX(i);
			cutData(inWS,outWS,sourceDim,targetDim,start,end,dim+1);
		}
	}
}

void SliceMDHisto::copyMetaData( Mantid::API::IMDHistoWorkspace_sptr inws,  Mantid::API::IMDHistoWorkspace_sptr outws)
{
	outws->setTitle(inws->getTitle());
	ExperimentInfo_sptr info;

	if(inws->getNumExperimentInfo() > 0){
		info = inws->getExperimentInfo(0);
		outws->addExperimentInfo(info);
	}
}

void SliceMDHisto::initDocs()
{
    this->setWikiSummary("Extracts a hyperslab of data from a MDHistoWorkspace");
}

