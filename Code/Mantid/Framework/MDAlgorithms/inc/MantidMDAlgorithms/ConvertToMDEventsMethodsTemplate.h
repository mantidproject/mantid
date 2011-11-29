#include "MantidMDAlgorithms/ConvertToMDEvents.h"
namespace Mantid
{
namespace MDAlgorithms
{


//-----------------------------------------------
template<size_t nd,Q_state Q>
void 
ConvertToMDEvents::processQND(API::IMDEventWorkspace *const piWS)
{
    // service variable used for efficient filling of the MD event WS  -> should be moved to configuration;
    size_t SPLIT_LEVEL(1024);
    // counder for the number of events
    size_t n_added_events(0);
    // amount of work
    const size_t numSpec  = inWS2D->getNumberHistograms();
    // progress reporter
    pProg = std::auto_ptr<API::Progress>(new API::Progress(this,0.0,1.0,numSpec));


    MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> *const pWs = dynamic_cast<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> *>(piWS);
    if(!pWs){
        g_log.error()<<"ConvertToMDEvents: can not cast input worspace pointer into pointer to proper target workspace\n"; 
        throw(std::bad_cast());
    }

    // one of the dimensions has to be X-ws dimension -> need to add check for that;

    // copy experiment info into target workspace
    API::ExperimentInfo_sptr ExperimentInfo(inWS2D->cloneExperimentInfo());
    uint16_t runIndex = pWs->addExperimentInfo(ExperimentInfo);
    
    const size_t specSize = inWS2D->blocksize();    
    std::vector<coord_t> Coord(nd);

    calc_generic_variables<Q>(Coord,nd);
    //External loop over the spectra:
    for (int64_t i = 0; i < int64_t(numSpec); ++i)
    {
 
        const MantidVec& X        = inWS2D->readX(i);
        const MantidVec& Signal   = inWS2D->readY(i);
        const MantidVec& Error    = inWS2D->readE(i);
        int32_t det_id            = det_loc.det_id[i];

        calculate_y_coordinate<Q>(Coord,i);
        //=> START INTERNAL LOOP OVER THE "TIME"
        for (size_t j = 0; j < specSize; ++j)
        {
            // drop emtpy events
           if(Signal[j]<FLT_EPSILON)continue;

           if(!calculate_ND_coordinates<Q>(X,i,j,Coord))continue;
            //  ADD RESULTING EVENTS TO THE WORKSPACE
            float ErrSq = float(Error[j]*Error[j]);
            pWs->addEvent(MDEvents::MDEvent<nd>(float(Signal[j]),ErrSq,runIndex,det_id,&Coord[0]));
            n_added_events++;
        } // end spectra loop

         // This splits up all the boxes according to split thresholds and sizes.
         //Kernel::ThreadScheduler * ts = new ThreadSchedulerFIFO();
         //ThreadPool tp(NULL);
          if(n_added_events>SPLIT_LEVEL){
                pWs->splitAllIfNeeded(NULL);
                n_added_events=0;
                pProg->report(i);
          }
          //tp.joinAll();        
       } // end detectors loop;

       // FINALIZE:
       if(n_added_events>0){
         pWs->splitAllIfNeeded(NULL);
         n_added_events=0;
        }
        pWs->refreshCache();
        pProg->report();          

}

/// helper function to create empty MDEventWorkspace with nd dimensions 
template<size_t nd>
API::IMDEventWorkspace_sptr
ConvertToMDEvents::createEmptyEventWS(size_t split_into,size_t split_threshold,size_t split_maxDepth)
{

       boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> > ws = 
       boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd> >(new MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd>());
    
      // Give all the dimensions
      for (size_t d=0; d<nd; d++)
      {
        Geometry::MDHistoDimension * dim = new Geometry::MDHistoDimension(this->dim_names[d], this->dim_names[d], this->dim_units[d], dim_min[d], dim_min[d], 10);
        ws->addDimension(Geometry::MDHistoDimension_sptr(dim));
      }
      ws->initialize();

      // Build up the box controller
      Mantid::API::BoxController_sptr bc = ws->getBoxController();
      bc->setSplitInto(split_into);
//      bc->setSplitThreshold(1500);
      bc->setSplitThreshold(split_threshold);
      bc->setMaxDepth(split_maxDepth);
      // We always want the box to be split (it will reject bad ones)
      ws->splitBox();
      return ws;
}

// PROCESS GENERIG VARIABLES AS FUNCTION OF ALGORITHM
template<> 
inline void ConvertToMDEvents::calc_generic_variables<NoQ>(std::vector<coord_t> &Coord, size_t nd){
    // workspace defines 2 properties
         fillAddProperties(Coord,nd,2);
      // get the Y axis; 
         pYAxis = dynamic_cast<API::NumericAxis *>(inWS2D->getAxis(1));
         if(!pYAxis ){ // the cast should be verified earlier; just in case here:
            throw(std::invalid_argument("Input workspace has to have Y-axis"));
          }
}
//
template<> 
void ConvertToMDEvents::calc_generic_variables<modQ>(std::vector<coord_t> &Coord, size_t nd){
    UNUSED_ARG(Coord);
    UNUSED_ARG(nd);
    throw(Kernel::Exception::NotImplementedError("not yet implemented"));
}
//
template<> 
inline void ConvertToMDEvents::calc_generic_variables<Q3D>(std::vector<coord_t> &Coord, size_t nd){
   // INELASTIC:
    // four inital properties came from workspace and all are interconnnected all additional defined by  properties:
    fillAddProperties(Coord,nd,4);
    Ei  =  boost::lexical_cast<double>(inWS2D->run().getProperty("Ei")->value());
    // the wave vector of input neutrons;
    ki=sqrt(Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq); 
    // 
    rotMat = this->get_transf_matrix();
}

// PROCESS Y VARIABLE AS FUNCTION OF SUBALGORITHM
template<>
void ConvertToMDEvents::calculate_y_coordinate<NoQ>(std::vector<coord_t> &Coord,size_t i)
{
        UNUSED_ARG(i);
        Coord[1]                  = pYAxis->operator()(i);
}
// PROCESS X - VARIABLE AS FUNCTION OF SUBALGORITHM
template<>
bool ConvertToMDEvents::calculate_ND_coordinates<NoQ>(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){
        UNUSED_ARG(i);
        Coord[0]    = 0.5*( X[j]+ X[j+1]);
        return true;
}
//
template<>
bool ConvertToMDEvents::calculate_ND_coordinates<Q3D>(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){

          coord_t E_tr = 0.5*( X[j]+ X[j+1]);
          Coord[3]    = E_tr;
          if(Coord[3]<dim_min[3]||Coord[3]>=dim_max[3])return false;


          double k_tr = sqrt((Ei-E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
   
          double  ex = det_loc.det_dir[i].X();
          double  ey = det_loc.det_dir[i].Y();
          double  ez = det_loc.det_dir[i].Z();
          double  qx  =  -ex*k_tr;                
          double  qy  =  -ey*k_tr;
          double  qz  = ki - ez*k_tr;

         Coord[0]  = (coord_t)(rotMat[0]*qx+rotMat[3]*qy+rotMat[6]*qz);  if(Coord[0]<dim_min[0]||Coord[0]>=dim_max[0])return false;
         Coord[1]  = (coord_t)(rotMat[1]*qx+rotMat[4]*qy+rotMat[7]*qz);  if(Coord[1]<dim_min[1]||Coord[1]>=dim_max[1])return false;
         Coord[2]  = (coord_t)(rotMat[2]*qx+rotMat[5]*qy+rotMat[8]*qz);  if(Coord[2]<dim_min[2]||Coord[2]>=dim_max[2])return false;

         return true;
}


} // endNamespace MDAlgorithms
} // endNamespace Mantid