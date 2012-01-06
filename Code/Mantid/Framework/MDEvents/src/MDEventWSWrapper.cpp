#include "MantidMDEvents/MDEventWSWrapper.h"

namespace Mantid
{
namespace MDEvents
{
// logger for loading workspaces  
   Kernel::Logger& MDEventWSWrapper::g_log =Kernel::Logger::get("MD-Algorithms");


/**function returns the number of dimensions in current MDEvent workspace or throws if the workspace has not been defined */
size_t 
MDEventWSWrapper::nDimensions()const
{
    if(n_dimensions==0){
        g_log.error()<<" attempt to obtain number of dimensions from not-initated undefined workspace wrapper\n";
        throw(std::invalid_argument("The workspace has not been initated yet"));
    }
    return size_t(n_dimensions);
}
/** function creates empty MD event workspace with given parameters (workspace factory) and stores internal pointer to this workspace for further usage */
API::IMDEventWorkspace_sptr
MDEventWSWrapper::createEmptyMDWS(size_t n_dim, const Strings &targ_dim_names,const Strings  &targ_dim_units,
                                                 const std::vector<double> &dim_min, const std::vector<double> &dim_max)
{
    if(n_dim<1||n_dim>MAX_N_DIM){
        g_log.error()<< " Number of requested dimensions: "<<n_dim<<" exceeds the maxumal allowed numed or dimensions: "<<MAX_N_DIM<<std::endl;
        throw(std::invalid_argument(" Numer of requested dimensions exceeds maximal allowed number of dimensions"));
    }
    this->n_dimensions = (int)n_dim;

    // store input dimensions;
    this->dim_min.resize(n_dim);
    this->dim_max.resize(n_dim);
    this->targ_dim_names.resize(n_dim);
    this->targ_dim_units.resize(n_dim);

    for(size_t i=0;i<n_dim;i++){
        this->dim_min[i]=dim_min[i];
        this->dim_max[i]=dim_max[i];
        this->targ_dim_names[i]= targ_dim_names[i];
        this->targ_dim_units[i]= targ_dim_units[i];
    }

    wsCreator[n_dimensions]();

    return workspace;
}

/// method adds the data to the workspace which was initiated before;
void  
MDEventWSWrapper::addMDData(std::vector<float> &sig_err,std::vector<uint16_t> &run_index,std::vector<uint32_t> &det_id,std::vector<coord_t> &Coord,size_t data_size)
{
  // transfer pointers to data:
   this->sig_err = &sig_err[0];
   this->run_index=&run_index[0];
   this->det_id   =&det_id[0]; 
   this->Coord    =&Coord[0];

   this->data_size = data_size;

   // run the actual addition 
   mdEvSummator[n_dimensions]();

}
  

/** method should be called at the end of the algorithm, to let the workspace manager know that it has whole responsibility for the workspace
   (As the algorithm is static, it will hold the pointer to the workspace otherwise, not allowing the WS manager to delete WS on request or when it find this usefull)*/
void 
MDEventWSWrapper::releaseWorkspace()
{
    // decrease the sp count by one?
     workspace.reset();
     // mark the number dimensions invalid;
     n_dimensions=0;
}

void 
MDEventWSWrapper::throwNotInitiatedError()
{
    g_log.error()<<" MDEvent WSWrapper class has not been initiated with any number of dimensions\n";
    throw(std::invalid_argument(" class has not been initiated"));
}
//
template<size_t i>
class LOOP{
  public:
    static inline void EXEC(MDEventWSWrapper *pH){
            LOOP< i-1 >::EXEC(pH);
            pH->wsCreator.push_back(boost::bind(std::mem_fun(&MDEventWSWrapper::createEmptyEventWS<i>),pH));
            pH->mdEvSummator.push_back(boost::bind(std::mem_fun(&MDEventWSWrapper::add_MDData<i>),pH));

    }
};
template<>
class LOOP<0>{
  public:
    static inline void EXEC(MDEventWSWrapper *pH){           
            fpVoidMethod fp = (boost::bind(std::mem_fun(&MDEventWSWrapper::throwNotInitiatedError),pH));
            pH->wsCreator.push_back(fp);
            pH->mdEvSummator.push_back(fp);
      }
};

MDEventWSWrapper::MDEventWSWrapper():n_dimensions(0)
{
    wsCreator.reserve(MAX_N_DIM+1);
    mdEvSummator.reserve(MAX_N_DIM+1);
      LOOP<MAX_N_DIM>::EXEC(this);
}



} // endnamespace MDEvents
} // endnamespace Mantid