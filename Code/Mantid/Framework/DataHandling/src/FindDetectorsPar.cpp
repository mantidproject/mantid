#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"

namespace Mantid
{
  namespace DataHandling
  {
// Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(FindDetectorsPar)

    //----------------------------------------------------------------
    Kernel::Logger& FindDetectorsPar::g_log=Kernel::Logger::get("DataHandling");

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
      const int progStep = static_cast<int>((nHist/100));
        
      // Loop over the spectra
      for (size_t i = 0; i < nHist; i++)
      {
        Geometry::IDetector_sptr spDet = inputWS->getDetector(i);
        // Check that we aren't writing a monitor...
        if (!spDet->isMonitor()){             

          Geometry::det_topology group_shape= spDet->getTopology();
          if(group_shape == Geometry::cyl){  // we have a ring;
            calc_cylDetPar(spDet,sample,azimuthal[i], polar[i], 
                           azimuthal_width[i], polar_width[i],secondary_flightpath[i]);
          }else{  // we have a detector or a rectangular shape
            calc_rectDetPar(inputWS,spDet,sample,azimuthal[i],polar[i],
                            azimuthal_width[i],polar_width[i],secondary_flightpath[i]);
          }
        }
        // make regular progress reports and check for canceling the algorithm
        if ( i % progStep == 0 ){
          progress.report();
        }
      }
      if(!this->isChild()){
        Kernel::Property *pPAsim    = this->getPointerToProperty("azimuthal");        fill_property(pPAsim,azimuthal);
        Kernel::Property *pPPol     = this->getPointerToProperty("polar");                fill_property(pPPol,polar);
        Kernel::Property *pPAsWidth = this->getPointerToProperty("azimuthal_width");  fill_property(pPAsWidth,azimuthal_width);
        Kernel::Property *pPPolWidth= this->getPointerToProperty("polar_width");       fill_property(pPPolWidth,polar_width);
        Kernel::Property *pPFlp     = this->getPointerToProperty("secondary_flightpath"); fill_property(pPFlp,secondary_flightpath);
      }

    }
// Constant for converting Radians to Degrees
    const double rad2deg = 180.0 / M_PI;

    void 
    FindDetectorsPar::calc_cylDetPar(const Geometry::IDetector_sptr spDet,const Geometry::IObjComponent_const_sptr sample,
                                     double &azim, double &polar, double &azim_width, double &polar_width,double &dist)
    {
      // polar values are constants for ring;
      polar_width= 2*M_PI;
      polar      = 0;

      // accumulators;
      double d1_min(FLT_MAX);
      double d1_max(-FLT_MAX);
      double azim_sum(0);
      double dist_sum(0);

      std::vector<Geometry::V3D> coord(3);

      // get vector leading from the sample to the ring centre 
      Geometry::V3D GroupCenter   = spDet->getPos();
      Geometry::V3D Observer      = sample->getPos();
      coord[1]  = (GroupCenter-Observer);
      double d0 = coord[1].norm();
      coord[1] /= d0;
      // access contribured detectors;
      Geometry::DetectorGroup * pDetGroup = dynamic_cast<Geometry::DetectorGroup *>(spDet.get());
      if(!pDetGroup){
        g_log.error()<<"calc_cylDetPar: can not downcast IDetector_sptr to detector group for det->ID: "<<spDet->getID()<<std::endl;
        throw(std::bad_cast());
      }
      std::vector<Geometry::IDetector_sptr> pDets = pDetGroup->getDetectors();
      Geometry::BoundingBox bbox;

      // loop through all detectors in the group 
      for(size_t i=0;i<pDets.size();i++){
        Geometry::V3D center= pDets[i]->getPos();
        coord[0]  = center-GroupCenter;
        double d1 = coord[0].norm();
        coord[0] /= d1;
        coord[2]  = coord[0].cross_prod(coord[1]);

        // obtain the bounding box, aligned accordingly to the coordinates;
        bbox.setBoxAlignment(center,coord);
        pDets[i]->getBoundingBox(bbox);

        double d_min = d1+bbox.xMin();  if(d_min<d1_min)d1_min = d_min;
        double d_max = d1+bbox.xMax();  if(d_max>d1_max)d1_max = d_max;

        azim_sum+=atan2(d1,d0);
        dist_sum+=d1*d1+d0*d0;
      }

      azim_width = atan2(d1_max,d0)-atan2(d1_min,d0);
      azim       = azim_sum/double(pDets.size());
      dist       = sqrt(dist_sum/double(pDets.size()));
        
    }

    void 
    FindDetectorsPar::calc_rectDetPar(const API::MatrixWorkspace_sptr inputWS, 
                                      const Geometry::IDetector_sptr spDet,const Geometry::IObjComponent_const_sptr sample,
                                      double &azim, double &polar, double &azim_width, double &polar_width,double &dist)
    {
      // Get Sample->Detector distance
      dist     =  spDet->getDistance(*sample);
      polar    =  inputWS->detectorTwoTheta(spDet)*rad2deg;
      azim     =  spDet->getPhi()*rad2deg;    
      // Now let's work out the detector widths
      // TODO: This is the historically wrong method...update it!
      // Get the bounding box
      Geometry::BoundingBox bbox;
      spDet->getBoundingBox(bbox);
      double xsize = bbox.xMax() - bbox.xMin();
      double ysize = bbox.yMax() - bbox.yMin();

      polar_width  = atan2((ysize/2.0), dist)*rad2deg;
      azim_width   = atan2((xsize/2.0), dist)*rad2deg;
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
