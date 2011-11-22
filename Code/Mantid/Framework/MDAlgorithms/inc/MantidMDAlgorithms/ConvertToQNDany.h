#ifndef MANTID_MD_CONVERT2_Q_ND_ANY
#define MANTID_MD_CONVERT2_Q_ND_ANY
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidAPI/Progress.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidMDAlgorithms/ConvertToQ3DdE.h"
//#include <boost/function>

namespace Mantid
{
namespace MDAlgorithms
{

/** ConvertToDiffractionMDWorkspace :
   *  Transfrom a workspace into MD workspace with components defined by user. 
   *
   * Gateway for number of subalgorithms, some are very important, some are questionable 
   * Intended to cover wide range of cases; 

   * @date 11-10-2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class ConvertToQNDany;
  typedef boost::function<void (ConvertToQNDany*, API::IMDEventWorkspace *const)> pMethod;

  enum Q_state{
       NoQ,
       modQ,
       Q3D
   };
//
  class DLLExport ConvertToQNDany  : public API::Algorithm
  {
  public:
    ConvertToQNDany();
    ~ConvertToQNDany();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "ConvertToQNDany";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Inelastic;MDAlgorithms";}  
  private:
    void init();
    void exec();
   /// Sets documentation strings for this algorithm
    virtual void initDocs();

      /// Progress reporter (shared)
    Kernel::ProgressBase * prog;
 
  /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& convert_log;

 
   /// helper function which does exatly what it says
   void check_max_morethen_min(const std::vector<double> &min,const std::vector<double> &max);
     /// the variable which describes the number of the dimensions, currently used by algorithm. Changes in input properties can change this number;
   size_t n_activated_dimensions;
   /// this variable describes default possible ID-s for Q-dimensions
   std::vector<std::string> Q_ID_possible;
 
   /// pointer to input workspace;
   Mantid::DataObjects::Workspace2D_sptr inWS2D;
   // the variable which keeps preprocessed positions of the detectors if any availible (TODO: should it be a table ws?);
    static preprocessed_detectors det_loc;  
 /** the function, does preliminary calculations of the detectors positions to convert results into k-dE space ;
      and places the resutls into static cash to be used in subsequent calls to this algorithm */
    static void process_detectors_positions(const DataObjects::Workspace2D_const_sptr inWS2D);
     /// the names of the log variables, which are used as dimensions
    std::vector<std::string> other_dim_names;
    Kernel::V3D u,v;
  protected: //for testing
   /** function returns the list of names, which can be treated as dimensions present in current matrix workspace */
   std::vector<std::string > get_dimension_names(const std::vector<std::string> &default_prop,API::MatrixWorkspace_const_sptr inMatrixWS)const;
   
   /** function processes arguments entered by user and tries to establish which algorithm should be deployed;   */
   std::string identify_the_alg(const std::vector<std::string> &dim_names_availible,const std::string &Q_dim_requested, const std::vector<std::string> &other_dim_selected,size_t &nDims)const;

   /** function provides the linear representation for the transformation matrix, */
   std::vector<double> get_transf_matrix()const;
   //
   template<size_t nd,Q_state Q>
   void process_QND(API::IMDEventWorkspace *const pOutWs);

   //  modQdE, // specific algorithm  -- 2D, powder
    void process_ModQ_dE_();
    //  modQND, // ModQND -- good for powders
    void process_ModQ__ND();
    //   modQdEND, // inelastic powders plus something
    void process_ModQ_dE_ND();
    //   Q3D,    // specific algorithm -- diffraction
    void process_Q3D___();
    //  Q3DdE,  // specific algorithn -- inelastic
    void process_Q3D_dE_();
    //  Q3DND,  // generic diffraction algorithm. 
    void process_Q3D__ND();
    //  Q3DdEND, // generic algorithn -- inelastic + other dependencies
    void process_Q3D_dE_ND();
    //
    std::map<std::string, pMethod> alg_selector;
 };

 template<typename T>
 void Q_analysis(coord_t &Coord, double X) { }
} // namespace Mantid
} // namespace MDAlgorithms

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
