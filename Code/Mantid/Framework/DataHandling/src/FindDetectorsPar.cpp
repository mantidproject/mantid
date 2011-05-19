#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Objects/BoundingBox.h"

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FindDetectorsPar)

/// Sets documentation strings for this algorithm
void FindDetectorsPar::initDocs()
{
  this->setWikiSummary("The algorithm returns the angular parameters and second flight path for a workspace detectors (data, usually availble in par or phx file)");
  //this->setOptionalMessage("Sums spectra bin-by-bin, equivalent to grouping the data from a set of detectors.  Individual groups can be specified by passing the algorithm a list of spectrum numbers, detector IDs or workspace indices. Many spectra groups can be created in one execution via an input file.");
}


using namespace Kernel;
using namespace API;
// nothing here according to mantid
FindDetectorsPar::FindDetectorsPar(){};
FindDetectorsPar::~FindDetectorsPar(){};

void FindDetectorsPar::init()
{
  declareProperty(
    new WorkspaceProperty<>("InputWorkspace","", Direction::Input),
    "The name of the workspace that will be used as input for the algorithm" );
  //
  // Outputs:
  declareProperty(new ArrayProperty<double>("azimuthal", new NullValidator<std::vector<double> >, Direction::Output),
    "A comma separated list or array containing a list detector's azimuthal angular positions" );
  declareProperty(new ArrayProperty<double>("polar", new NullValidator<std::vector<double> >, Direction::Output),
    "A comma separated list or array containing a list detector's polar angular positions" );
  declareProperty(new ArrayProperty<double>("azimuthal_width", new NullValidator<std::vector<double> >, Direction::Output),
    "A comma separated list or array containing a list detector's azimutal angular widths" );
  declareProperty(new ArrayProperty<double>("polar_width", new NullValidator<std::vector<double> >, Direction::Output),
    "A comma separated list or array containing a list detector's polar angular widths" );
  declareProperty(new ArrayProperty<double>("secondary_flightpath", new NullValidator<std::vector<double> >, Direction::Output),
    "A comma separated list or array containing a list detector's secondary flight pathes (distances from detectors to the centre of sample)");
}

void 
FindDetectorsPar::exec()
{

 // Get the input workspace
	const MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  if(inputWS.get()==NULL){
	  throw(Kernel::Exception::NotFoundError("can not obtain InoputWorkspace for the algorithm to work",""));
  }
  

  // Constant for converting Radians to Degrees
  const double rad2deg = 180.0 / M_PI;

  // Number of spectra
  const size_t nHist = inputWS->getNumberHistograms();
  // Get a pointer to the sample
   Geometry::IObjComponent_const_sptr sample =inputWS->getInstrument()->getSample();


  azimuthal.resize(nHist);
  polar.resize(nHist);
  azimuthal_width.resize(nHist);
  polar_width.resize(nHist);
  secondary_flightpath.resize(nHist);

   Progress progress(this,0,1,100);
   const int progStep = static_cast<int>(ceil(nHist/100.0));
      
      // Loop over spectra
      for (int i = 0; i < nHist; i++) {
          // Check that we aren't writing a monitor...
          if (!inputWS->getDetector(i)->isMonitor())
            {
              Geometry::IDetector_sptr det = inputWS->getDetector(i);
              polar[i]     =  inputWS->detectorTwoTheta(det) * rad2deg;
              azimuthal[i] =  det->getPhi() * rad2deg;

              // Get Sample->Detector distance
			  double distance        = det->getDistance(*sample);
              secondary_flightpath[i]= distance;

              // Now let's work out the detector widths
              // TODO: This is the historically wrong method...update it!
	      // Get the bounding box
	      Geometry::BoundingBox bbox;
	      det->getBoundingBox(bbox);
	      double xsize = bbox.xMax() - bbox.xMin();
	      double ysize = bbox.yMax() - bbox.yMin();

              double delta_polar = atan2((ysize / 2.0), distance) * rad2deg;
              double delta_azimuthal = atan2((xsize / 2.0), distance) * rad2deg;

              // Now store the widths...
              polar_width[i]    = delta_polar;
			  azimuthal_width[i]= delta_azimuthal;

		  }
	  }
	  if(!this->isChild()){
		  Kernel::Property *pPAsim = this->getPointerToProperty("azimuthal");           fill_property(pPAsim,azimuthal);
		  Kernel::Property *pPPol = this->getPointerToProperty("polar");	            fill_property(pPPol,polar);
		  Kernel::Property *pPAsWidth = this->getPointerToProperty("azimuthal_width");  fill_property(pPAsWidth,azimuthal_width);
		  Kernel::Property *pPPolWidth = this->getPointerToProperty("polar_width");     fill_property(pPPolWidth,polar_width);
		  Kernel::Property *pPFlp = this->getPointerToProperty("secondary_flightpath"); fill_property(pPFlp,secondary_flightpath);
	  }

}
void 
FindDetectorsPar::fill_property(Kernel::Property *const pProperty,std::vector<double> const&data)
{
    std::stringstream Buf;
	if(!data.empty()){
		Buf<<data[0];
	}else{
	    pProperty->setValue("");
		return;
	}
	for(size_t i=1;i<data.size();i++){
		Buf<<","<<data[i];
	}
	Buf<<std::endl;
	pProperty->setValue( Buf.str());
}
}
}
