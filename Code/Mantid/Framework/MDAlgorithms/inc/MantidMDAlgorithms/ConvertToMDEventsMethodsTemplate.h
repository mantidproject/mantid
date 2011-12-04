#include "MantidMDAlgorithms/ConvertToMDEvents.h"
namespace Mantid
{
namespace MDAlgorithms
{

//-----------------------------------------------
template<size_t nd,Q_state Q, AnalMode MODE, CnvrtUnits CONV>
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
        convert_log.error()<<"ConvertToMDEvents: can not cast input worspace pointer into pointer to proper target workspace\n"; 
        throw(std::bad_cast());
    }
    coord_transformer<Q,MODE,CONV> trn(this); 
    // one of the dimensions has to be X-ws dimension -> need to add check for that;

    // copy experiment info into target workspace
    API::ExperimentInfo_sptr ExperimentInfo(inWS2D->cloneExperimentInfo());
    uint16_t runIndex = pWs->addExperimentInfo(ExperimentInfo);
    
    const size_t specSize = inWS2D->blocksize();    
    std::vector<coord_t> Coord(nd);


    if(!trn.calc_generic_variables(Coord,nd))return; // if any property dimension is outside of the data range requested
    //External loop over the spectra:
    for (int64_t i = 0; i < int64_t(numSpec); ++i)
    {
 
        const MantidVec& X        = inWS2D->readX(i);
        const MantidVec& Signal   = inWS2D->readY(i);
        const MantidVec& Error    = inWS2D->readE(i);
        int32_t det_id            = det_loc.det_id[i];

        if(!trn.calculate_y_coordinate(Coord,i))continue;   // skip y outsize of the range;

        //=> START INTERNAL LOOP OVER THE "TIME"
        for (size_t j = 0; j < specSize; ++j)
        {
            // drop emtpy events
           if(Signal[j]<FLT_EPSILON)continue;

           if(!trn.calculate_ND_coordinates(X,i,j,Coord))continue; // skip ND outside the range
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
     
///
template<Q_state Q,AnalMode MODE,CnvrtUnits CONV>
struct coord_transformer
      {
      coord_transformer(ConvertToMDEvents *){}
    /**Template defines common interface to common part of the algorithm, where all variables
     * needed within the loop calculated outside of the loop. 
     * In addition it caluclates the property-dependant coordinates 
     *
     * @param n_ws_variabes -- subalgorithm specific number of variables, calculated from the workspace data
     *
     * @return Coord        -- subalgorithm specific number of variables, calculated from properties and placed into specific place of the Coord vector;
     * @return true         -- if all Coord are within the range requested by algorithm. false otherwise
     *
     * has to be specialized
    */
    inline bool calc_generic_variables(std::vector<coord_t> &Coord, size_t n_ws_variabes){
        UNUSED_ARG(Coord); UNUSED_ARG(n_ws_variabes);throw(Kernel::Exception::NotImplementedError(""));
        return false;}

   
    /** template generalizes the code to calculate Y-variables within the external loop of processQND workspace
     * @param X    -- vector of X workspace values
     * @param i    -- index of external loop, identifying current y-coordinate
     * 
     * @return Coord -- current Y coordinate, placed in the position of the Coordinate vector, specific for particular subalgorithm.    
     * @return true         -- if all Coord are within the range requested by algorithm. false otherwise   
     * 
     *  some default implementations possible (e.g mode Q3D,ragged  Any_Mode( Direct, indirect,elastic), 
     */
    inline bool calculate_y_coordinatee(std::vector<coord_t> &Coord,size_t i){
        UNUSED_ARG(Coord); UNUSED_ARG(i);  return true;}

    /** template generalizes the code to calculate all remaining coordinates, defined within the inner loop
     * @param X    -- vector of X workspace values
     * @param i    -- index of external loop, identifying generic y-coordinate
     * @param j    -- index of internal loop, identifying generic x-coordinate
     * 
     * @return Coord --Subalgorithm specific number of coordinates, placed in the proper position of the Coordinate vector   
     * @return true  -- if all Coord are within the range requested by algorithm. false otherwise   
     *
     * has to be specialized
     */
    inline bool calculate_ND_coordinatese(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){
        UNUSED_ARG(X); UNUSED_ARG(i); UNUSED_ARG(j); UNUSED_ARG(Coord);throw(Kernel::Exception::NotImplementedError(""));
        return false;}
}; // end coordTransformer structure:


//----------------------------------------------------------------------------------------------------------------------
// SPECIALIZATIONS:
//----------------------------------------------------------------------------------------------------------------------
// NoQ,ANY_Mode 
template<AnalMode MODE,CnvrtUnits CONV> 
struct coord_transformer<NoQ,MODE,CONV>
{
    inline bool calc_generic_variables(std::vector<coord_t> &Coord, size_t nd)    
    {
    // workspace defines 2 properties
         pHost->fillAddProperties(Coord,nd,2);
         for(size_t i=2;i<nd;i++){
            if(Coord[i]<pHost->dim_min[i]||Coord[i]>=pHost->dim_max[i])return false;
         }
      // get the Y axis; 
         pYAxis = dynamic_cast<API::NumericAxis *>(pHost->inWS2D->getAxis(1));
         if(!pYAxis ){ // the cast should be verified earlier; just in case here:
            throw(std::invalid_argument("Input workspace has to have Y-axis"));
          }
         return true;
    }

    inline bool calculate_y_coordinate(std::vector<coord_t> &Coord,size_t i)
    {
        Coord[1]                  = pYAxis->operator()(i);
        if(Coord[1]<pHost->dim_min[1]||Coord[1]>=pHost->dim_max[1])return false;
        return true;
    }

    inline bool calculate_ND_coordinates(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){
         Coord[0]    = 0.5*( X[j]+ X[j+1]);
        if(Coord[0]<pHost->dim_min[0]||Coord[0]>=pHost->dim_max[0])return false;
        return true;
    }
    coord_transformer(ConvertToMDEvents *pConv):pHost(pConv)
    {}
private:
   // the variables used for exchange data between different specific parts of the generic ND algorithm:
    // pointer to Y axis of MD workspace
     API::NumericAxis *pYAxis;
     ConvertToMDEvents *pHost;

};
//
////----------------------------------------------------------------------------------------------------------------------
//
// modQ,ANY_Mode 
template<AnalMode MODE,CnvrtUnits CONV> 
struct coord_transformer<modQ,MODE,CONV>
{
    inline bool calc_generic_variables(std::vector<coord_t> &Coord, size_t nd){
        UNUSED_ARG(Coord);
        UNUSED_ARG(nd);
        throw(Kernel::Exception::NotImplementedError("not yet implemented"));
    }
    inline bool calculate_y_coordinate(std::vector<coord_t> &Coord,size_t i)
     {UNUSED_ARG(Coord); UNUSED_ARG(i); return false;}

      inline bool calculate_ND_coordinates(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){
          UNUSED_ARG(i);UNUSED_ARG(j);UNUSED_ARG(X);UNUSED_ARG(Coord);
          return false;
     }

    coord_transformer(ConvertToMDEvents *pConv) {}
};
//------------------------------------------------------------------------------------------------------------------------------
// Q3D any mode 
template<AnalMode MODE>
inline double k_trans(double Ei, double E_tr){
    throw(Kernel::Exception::NotImplementedError("Generic K_tr not implemented"));
}
template<>
inline double k_trans<Direct>(double Ei, double E_tr){
    return sqrt((Ei-E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
}
template<>
inline double k_trans<Indir>(double Ei, double E_tr){
    return sqrt((Ei+E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
}

    // DO CONVERSION
// procedure which converts/ non-converts units
 template<CnvrtUnits CONV>
 inline coord_t get_x_converted(double X1, double X2, const double factor,const double power)
 {
     UNUSED_ARG(factor);UNUSED_ARG(power);
     return (coord_t)(0.5*(X1+X2));
 }
 template<>
 inline coord_t get_x_converted<ConvertYes>(double X1,double X2,const double factor,const double power)
 {
     double Xm=0.5*(X1+X2);
     return Xm=(coord_t)(factor*std::pow(Xm,power));
 }

 template<AnalMode MODE>
 inline void prepare_conversion(Kernel::Unit *const pThisUnit,double &factor,double &power) 
{
    if(!pThisUnit->quickConversion("DeltaE",factor,power)){
          throw(std::logic_error(" should be able to convert units and catch case of non-conversions much earlier"));
    }

}
template<>
inline void prepare_conversion<Elastic>(Kernel::Unit *const pThisUnit,double &factor,double &power)
{
   if(!pThisUnit->quickConversion("Wavelength",factor,power)){
        throw(std::logic_error(" should be able to convert units and catch case of non-conversions much earlier"));
   }

}

template<AnalMode MODE,CnvrtUnits CONV> 
struct coord_transformer<Q3D,MODE,CONV>
{
    inline bool calc_generic_variables(std::vector<coord_t> &Coord, size_t nd)
    {
        // four inital properties came from workspace and all are interconnnected all additional defined by  properties:
        pHost->fillAddProperties(Coord,nd,4);
        for(size_t i=4;i<nd;i++){
            if(Coord[i]<pHost->dim_min[i]||Coord[i]>=pHost->dim_max[i])return false;
         }
        // energy 
         Ei  =  boost::lexical_cast<double>(pHost->inWS2D->run().getProperty("Ei")->value());
         // the wave vector of incident neutrons;
         ki=sqrt(Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq); 
         // 
        rotMat = pHost->get_transf_matrix();
        if(CONV==ConvertYes){
              const Kernel::Unit_sptr pThisUnit=pHost->inWS2D->getAxis(0)->unit();
               prepare_conversion<MODE>(pThisUnit.get(),factor,power);
        }
        return true;
    }
    //
    inline bool calculate_y_coordinate(std::vector<coord_t> &Coord,size_t i){
              UNUSED_ARG(Coord); UNUSED_ARG(i);  return true;}

    inline bool calculate_ND_coordinates(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){

          coord_t E_tr = get_x_converted<CONV>(X[j],X[j+1],factor,power);
          Coord[3]    = E_tr;
          if(Coord[3]<pHost->dim_min[3]||Coord[3]>=pHost->dim_max[3])return false;


          double k_tr = k_trans<MODE>(Ei,E_tr);
   
          double  ex = pHost->det_loc.det_dir[i].X();
          double  ey = pHost->det_loc.det_dir[i].Y();
          double  ez = pHost->det_loc.det_dir[i].Z();
          double  qx  =  -ex*k_tr;                
          double  qy  =  -ey*k_tr;
          double  qz  = ki - ez*k_tr;

         Coord[0]  = (coord_t)(rotMat[0]*qx+rotMat[3]*qy+rotMat[6]*qz);  if(Coord[0]<pHost->dim_min[0]||Coord[0]>=pHost->dim_max[0])return false;
         Coord[1]  = (coord_t)(rotMat[1]*qx+rotMat[4]*qy+rotMat[7]*qz);  if(Coord[1]<pHost->dim_min[1]||Coord[1]>=pHost->dim_max[1])return false;
         Coord[2]  = (coord_t)(rotMat[2]*qx+rotMat[5]*qy+rotMat[8]*qz);  if(Coord[2]<pHost->dim_min[2]||Coord[2]>=pHost->dim_max[2])return false;

         return true;
    }

    coord_transformer(ConvertToMDEvents *pConv):pHost(pConv){}
private:
    ConvertToMDEvents *pHost;
    // the energy of the incident neutrons
    double Ei;
    // the wavevector of incident neutrons
    double ki;
    // the matrix which transforms the neutron momentums from lablratory to crystall coordinate system. 
    std::vector<double> rotMat;
    // variables for units conversion:
    double factor, power;


};


} // endNamespace MDAlgorithms
} // endNamespace Mantid