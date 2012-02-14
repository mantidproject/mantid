#include "MantidMDEvents/MDEventWSWrapper.h"

namespace Mantid
{
namespace MDEvents
{
// logger for loading workspaces  
 //  Kernel::Logger& MDEventWSWrapper::g_log =Kernel::Logger::get("MD-Algorithms");

     
/**function returns the number of dimensions in current MDEvent workspace or throws if the workspace has not been defined */
size_t 
MDEventWSWrapper::nDimensions()const
{
    if(n_dimensions==0){
      //  g_log.error()<<" attempt to obtain number of dimensions from not-initated undefined workspace wrapper\n";
        throw(std::invalid_argument("The workspace has not been initated yet"));
    }
    return size_t(n_dimensions);
}
/** function creates empty MD event workspace with given parameters (workspace factory) and stores internal pointer to this workspace for further usage */
API::IMDEventWorkspace_sptr
MDEventWSWrapper::createEmptyMDWS(const MDWSDescription &WSD)
{
    if(WSD.n_dims<1||WSD.n_dims>MAX_N_DIM){
   //     g_log.error()<< " Number of requested dimensions: "<<WSD.n_dims<<" exceeds the maxumal allowed numed or dimensions: "<<MAX_N_DIM<<std::endl;
        throw(std::invalid_argument(" Numer of requested dimensions exceeds maximal allowed number of dimensions"));
    }
    this->n_dimensions = (int)WSD.n_dims;
 

    (this->*(wsCreator[n_dimensions]))(WSD.dim_names,WSD.dim_units,WSD.dim_IDs,WSD.dim_min,WSD.dim_max,10);

    return workspace;
}

/// method adds the data to the workspace which was initiated before;
void  
MDEventWSWrapper::addMDData(std::vector<float> &sig_err,std::vector<uint16_t> &run_index,std::vector<uint32_t> &det_id,std::vector<coord_t> &Coord,size_t data_size)const
{
    if(data_size==0)return;

   // run the actual addition 
   (this->*(mdEvSummator[n_dimensions]))(&sig_err[0],&run_index[0],&det_id[0],&Coord[0],data_size);

}
  

/** method should be called at the end of the algorithm, to let the workspace manager know that it has whole responsibility for the workspace
   (As the algorithm is static, it will hold the pointer to the workspace otherwise, not allowing the WS manager to delete WS on request or when it find this usefull)*/
void 
MDEventWSWrapper::releaseWorkspace()
{
    // decrease the sp count by one
     workspace.reset();
     // mark the number dimensions invalid;
     n_dimensions=0;
}


//
template<size_t i>
class LOOP{
  public:
    static inline void EXEC(MDEventWSWrapper *pH){
            LOOP< i-1 >::EXEC(pH);
            pH->wsCreator[i]    = &MDEventWSWrapper::createEmptyEventWS<i>;
            pH->mdEvSummator[i] = &MDEventWSWrapper::add_MDData<i>;
            pH->mdCalCentroid[i]= &MDEventWSWrapper::calc_Centroid<i>;

    }
};
template<>
class LOOP<0>{
  public:
    static inline void EXEC(MDEventWSWrapper *pH){           
            pH->wsCreator[0]    =&MDEventWSWrapper::createEmptyEventWS_wrong;
            pH->mdEvSummator[0] =&MDEventWSWrapper::add_MDData_wrong;
            pH->mdCalCentroid[0]=&MDEventWSWrapper::calc_Centroid_wrong;
      }
};

MDEventWSWrapper::MDEventWSWrapper():n_dimensions(0)
{
    wsCreator.resize(MAX_N_DIM+1);
    mdEvSummator.resize(MAX_N_DIM+1);
    mdCalCentroid.resize(MAX_N_DIM+1);
    LOOP<MAX_N_DIM>::EXEC(this);
}



} // endnamespace MDEvents
} // endnamespace Mantid