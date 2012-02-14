#ifndef H_MDEVENT_WS_WRAPPER
#define H_MDEVENT_WS_WRAPPER

#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDWSDescription.h"

namespace Mantid
{
namespace MDEvents
{
  /**  The class which wraps MD Events factory and allow to work with a N-dimensional templated MDEvent workspace like usuall class with n-dimension as a parameter
    *   
    *   Introduced to decrease code bloat in methods and algorithms, which use MDEvents write interface and run-time defined number of dimensions
    
    @date 2011-28-12

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
/// vectors of strings are often used here 
typedef  std::vector<std::string> Strings;

/// signature for a function which implement void method of this workspace
class MDEventWSWrapper;
/// signature to void templated function 
typedef void (MDEventWSWrapper::*fpVoidMethod)();
/// signature for the internal templated function pointer to add data to the existing workspace
typedef void(MDEventWSWrapper::*fpAddData)(float *,uint16_t *,uint32_t*,coord_t*,size_t)const;
/// signature for the internal templated function pointer to create workspace
typedef  void(MDEventWSWrapper::*fpCreateWS)(const Strings &,const Strings &t,const Strings &,
          const std::vector<double> &, const std::vector<double> &, size_t );


class DLLExport MDEventWSWrapper
{
public:
    MDEventWSWrapper();
    virtual ~MDEventWSWrapper(){};
    /// get maximal number of dimensions, allowed for the algorithm and embedded in algorithm during compilation time. 
    static size_t getMaxNDim(){return MAX_N_DIM;}

    /// get number of dimensions, for the workspace, currently accessed by the algorithm. 
    size_t nDimensions()const;
    /** function creates empty MD event workspace with given parameters (workspace factory) and stores internal pointer to this workspace for further usage */
    API::IMDEventWorkspace_sptr createEmptyMDWS(const MDWSDescription &WSD);
    /// add the data to the internal workspace. The workspace has to exist and be initiated 
    void  addMDData(std::vector<float> &sig_err,std::vector<uint16_t> &run_index,std::vector<uint32_t> &det_id,std::vector<coord_t> &Coord,size_t data_size)const;
    /// releases the function pointer of the workspace, stored by the class and makes the class instance undefined; 
    void releaseWorkspace();
    /// get access to the internal workspace
    API::IMDEventWorkspace_sptr pWorkspace(){return workspace;}
    // should it be moved to the IMDEvents?
    void refreshCentroid(){
        (this->*(mdCalCentroid[n_dimensions]))();
    };
private:
   /// maximal nuber of dimensions, currently supported by the class;
   static const size_t MAX_N_DIM=8;
   /// actual number of dimensions, initiated in current MD workspace 0 if not initated;
   size_t n_dimensions;
   /// logger -> to provide logging, for MD dataset file operations
   //static Mantid::Kernel::Logger& g_log;
   /// taret workspace:
   API::IMDEventWorkspace_sptr workspace;
  
   /// helper class to generate methaloop on MD workspaces dimensions:
   template< size_t i>
   friend class LOOP;
   ///
   /// vector holding function pointers to the code, creating different number of dimension worspace as function of dimensions number
   std::vector<fpCreateWS> wsCreator;
   /// vector holding function pointers to the code, which adds diffrent dimension number events to the workspace
   std::vector<fpAddData> mdEvSummator;
   /// vector holding function pointers to the code, which refreshes centroid (could it be moved to IMD?)
   std::vector<fpVoidMethod> mdCalCentroid;

  
   /// helper function to create empty MDEventWorkspace with nd dimensions 
   template<size_t nd>
   void  createEmptyEventWS(const Strings &targ_dim_names,const Strings &targ_dim_ID,const Strings &targ_dim_units,
                             const std::vector<double> &dim_min, const std::vector<double> &dim_max, size_t numBins)
   {

           boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> > ws = 
           boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd> >(new MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd>());
    
        // Give all the dimensions
        for (size_t d=0; d<nd; d++)
        {
             Geometry::MDHistoDimension * dim = new Geometry::MDHistoDimension(targ_dim_names[d], targ_dim_ID[d], targ_dim_units[d], 
                 coord_t(dim_min[d]), coord_t(dim_max[d]), numBins);
                 ws->addDimension(Geometry::MDHistoDimension_sptr(dim));
        }
        ws->initialize();

      // Build up the box controller
      // bc = ws->getBoxController();
      // Build up the box controller, using the properties in BoxControllerSettingsAlgorithm 
     //     this->setBoxController(bc);
      // We always want the box to be split (it will reject bad ones)
     //ws->splitBox();
        this->workspace = ws;
   }
    /// templated by number of dimesnions function to add multidimensional data to the workspace
   template<size_t nd>
   void add_MDData(float *sig_err,uint16_t *run_index,uint32_t* det_id,coord_t* Coord,size_t data_size)const
   {
  
        MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> *const pWs = dynamic_cast<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> *>(this->workspace.get());
        if(!pWs){
           // g_log.error()<<"MDEventWSWrapper: can not cast  worspace pointer into pointer to proper target workspace\n"; 
            throw(std::bad_cast());
        }      
        for(size_t i=0;i<data_size;i++){
            pWs->addEvent(MDEvents::MDEvent<nd>(*(sig_err+2*i),*(sig_err+2*i+1),*(run_index+i),*(det_id+i),(Coord+i*nd)));
        }

       // This splits up all the boxes according to split thresholds and sizes.
         //Kernel::ThreadScheduler * ts = new ThreadSchedulerFIFO();
         //ThreadPool tp(NULL);
         //pWs->splitAllIfNeeded(NULL);
        //tp.joinAll();        
 
    }
   //
  /// helper function to create empty MDEventWorkspace with nd dimensions 
    template<size_t nd>
    void  calc_Centroid(void)
    {

       MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> *const pWs = dynamic_cast<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> *>(this->workspace.get());
        if(!pWs){
           // g_log.error()<<"MDEventWSWrapper: can not cast  worspace pointer into pointer to proper target workspace\n"; 
            throw(std::bad_cast());
        }
        pWs->getBox()->refreshCentroid(NULL);
    }
   /// terminator for 0 dimensions, will throw
   template<>void  calc_Centroid<0>(void);
   /// terminator for 0 dimensions, will throw
   template<>void createEmptyEventWS<0>(const Strings &,const Strings &,const Strings &,
                                        const std::vector<double> &, const std::vector<double> &, size_t );
   template<>void add_MDData<0>(float *,uint16_t *,uint32_t*,coord_t* ,size_t)const;

 };


} // endnamespace MDEvents
} // endnamespace Mantid

#endif
