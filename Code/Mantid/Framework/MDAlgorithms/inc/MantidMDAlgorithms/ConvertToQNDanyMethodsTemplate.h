#include "MantidMDAlgorithms/ConvertToQNDany.h"
namespace Mantid
{
namespace MDAlgorithms
{

template<size_t nd,Q_state Q>
void ConvertToQNDany::process_QND(API::IMDEventWorkspace *const piWS)
{

    MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> *const pWs = dynamic_cast<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> *>(piWS);
    if(!pWs){
        g_log.error()<<"ConvertToQNDany: can not cast input worspace pointer into pointer to proper target workspace\n"; 
        throw(std::bad_cast());
    }

    // one of the dimensions has to be X-ws dimension -> need to add check for that;

    // copy experiment info into target workspace
    API::ExperimentInfo_sptr ExperimentInfo(inWS2D->cloneExperimentInfo());
    uint16_t runIndex = pWs->addExperimentInfo(ExperimentInfo);

    


    const size_t numSpec  = inWS2D->getNumberHistograms();
    const size_t specSize = inWS2D->blocksize();    
    std::vector<coord_t> Coord(nd);

    size_t n_x0(1);   
#ifdef Q3D_
    n_x0 = 3;
    std::vector<double> rotMat = this->get_transf_matrix();
#endif
#ifdef MODQ
  throw(Kernel::Exception::NotImplementedError(""));
#endif
#ifdef INELASTIC
    n_x0+=1;
#endif
    for(size_t i=n_x0;i<nd;i++){
        //HACK: A METHOD, Which converts TSP into value, correspondent to time scale of matrix workspace has to be developed and deployed!
        Kernel::TimeSeriesProperty<double> *run_property = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(inWS2D->run().getProperty(this->other_dim_names[i]));  
        if(!run_property){
            g_log.error()<<" property: "<<this->other_dim_names[i]<<" is not a time series (run) property\n";
        }
        Coord[i]=run_property->firstValue();
    }


    size_t n_added_events(0);
    size_t SPLIT_LEVEL(1024);
    for (int64_t i = 0; i < int64_t(numSpec); ++i)
    {

    //    const MantidVec& X        = inWS2D->readX(i);
        const MantidVec& Signal   = inWS2D->readY(i);
        const MantidVec& Error    = inWS2D->readE(i);
        int32_t det_id            = det_loc.det_id[i];
    
   
        for (size_t j = 0; j < specSize; ++j)
        {
            // drop emtpy events
           if(Signal[j]<FLT_EPSILON)continue;

          // double E_tr = 0.5*( X[j]+ X[j+1]);
#ifdef NOQ
           Coord[0]=(coord_t)E_tr;
#endif
#ifdef Q3D_
          double k_tr = sqrt((Ei-E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
   
          double  ex = det_loc.det_dir[i].X();
          double  ey = det_loc.det_dir[i].Y();
          double  ez = det_loc.det_dir[i].Z();
          double  qx  =  -ex*k_tr;                
          double  qy  =  -ey*k_tr;
          double  qz  = ki - ez*k_tr;

          Coord[0]  = (coord_t)(rotMat[0]*qx+rotMat[3]*qy+rotMat[6]*qz);  if(QE[0]<QEmin[0]||QE[0]>=QEmax[0])continue;
          Coord[1]  = (coord_t)(rotMat[1]*qx+rotMat[4]*qy+rotMat[7]*qz);  if(QE[1]<QEmin[1]||QE[1]>=QEmax[1])continue;
          Coord[2]  = (coord_t)(rotMat[2]*qx+rotMat[5]*qy+rotMat[8]*qz);  if(QE[2]<QEmin[2]||QE[2]>=QEmax[2])continue;
#endif
#ifdef  INELASTIC
          Coord[n_x0-1]=(coord_t)E_tr;
#endif
 

            float ErrSq = float(Error[j]*Error[j]);
            pWs->addEvent(MDEvents::MDEvent<nd>(float(Signal[j]),ErrSq,runIndex,det_id,&Coord[0]));
            n_added_events++;
        }
  
      // This splits up all the boxes according to split thresholds and sizes.
        //Kernel::ThreadScheduler * ts = new ThreadSchedulerFIFO();
        //ThreadPool tp(NULL);
      if(n_added_events>SPLIT_LEVEL){
            pWs->splitAllIfNeeded(NULL);
            n_added_events=0;
       }
        //tp.joinAll();
        prog->report(i);  
     }
     if(n_added_events>0){
         pWs->splitAllIfNeeded(NULL);
         n_added_events=0;
     }
      pWs->refreshCache();
      prog->report();      

}

/// helper function to create empty MDEventWorkspace with nd dimensions 
template<size_t nd>
boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd> > create_emptyEventWS(const std::vector<std::string> &dimensionNames,const std::vector<std::string> dimensionUnits,
                                                                              const std::vector<double> &dimMin,const std::vector<double> &dimMax)
{

       boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> > ws = 
       boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd> >(new MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd>());
    
      // Give all the dimensions
      for (size_t d=0; d<nd; d++)
      {
        Geometry::MDHistoDimension * dim = new Geometry::MDHistoDimension(dimensionNames[d], dimensionNames[d], dimensionUnits[d], dimMin[d], dimMax[d], 10);
        ws->addDimension(Geometry::MDHistoDimension_sptr(dim));
      }
      ws->initialize();

      // Build up the box controller
      Mantid::API::BoxController_sptr bc = ws->getBoxController();
      bc->setSplitInto(5);
//      bc->setSplitThreshold(1500);
      bc->setSplitThreshold(10);
      bc->setMaxDepth(20);
      // We always want the box to be split (it will reject bad ones)
      ws->splitBox();
      return ws;
}

} // endNamespace MDAlgorithms
} // endNamespace Mantid