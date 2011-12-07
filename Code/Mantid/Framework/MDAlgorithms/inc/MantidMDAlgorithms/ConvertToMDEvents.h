#ifndef MANTID_MD_CONVERT2_MDEVENTS
#define MANTID_MD_CONVERT2_MDEVENTS
    
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Algorithm.h" 

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidMDAlgorithms/ConvertToQ3DdE.h"


namespace Mantid
{
namespace MDAlgorithms
{

/** ConvertToMDEvents :
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
  class ConvertToMDEvents;
  // signature for an algorithm processing n-dimension event workspace
  typedef boost::function<void (ConvertToMDEvents*, API::IMDEventWorkspace *const)> pMethod;
  // signature for a fucntion, creating n-dimension workspace
  //typedef boost::function<API::IMDEventWorkspace_sptr (ConvertToMDEvents*, const std::vector<std::string> &,const std::vector<std::string> &, size_t ,size_t ,size_t )> pWSCreator;
  typedef boost::function<API::IMDEventWorkspace_sptr (ConvertToMDEvents*, size_t ,size_t ,size_t )> pWSCreator;
 // vectors of strings are here everywhere
  typedef  std::vector<std::string> Strings;
  /// known sates for algorithms, caluclating Q-values
  enum Q_state{
       NoQ, 
       modQ,
       Q3D
   };
  /** known analysis modes, arranged according to emodes 
      It is importent to assign enums proper numbers, as directc correspondence between enums and their emodes 
      used by algorithm;
  */
  enum AnalMode{  
      Elastic = 0,  //< int emode = 0; Elastic analysis
      Direct  = 1,  //< emode=1; Direct inelastic analysis mode
      Indir   = 2,  //< emode=2; InDirect inelastic analysis mode
      ANY_Mode      //< couples with NoQ, means just copying existing data (may be douing units conversion)
  };
  /// enum describes if there is need to convert workspace units and different units conversion modes
  enum CnvrtUnits
  {
      ConvertNo,   //< no, input workspace has the same units as output workspace or in units used by Q-dE algorithms naturally
      ConvertFast, //< the input workspace has different units from the requested and fast conversion is possible
      ConvByTOF,   //< conversion possible via TOF
      ConvFromTOF  //< Input workspace units are the TOF 
  };
/// predefenition of the class, which does all coordinate transformations, Linux compilers need this. 
  template<Q_state Q, AnalMode MODE, CnvrtUnits CONV> 
  struct COORD_TRANSFORMER;
  
  class DLLExport ConvertToMDEvents  : public API::Algorithm
  {
  public:
    ConvertToMDEvents();
    ~ConvertToMDEvents();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "ConvertToMDEvents";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDAlgorithms";}  

//**> helper functions: To assist with units conversion and get access to some important internal states of the algorithm
    static std::string          getNativeUnitsID(ConvertToMDEvents const *const pHost);
    static Kernel::Unit_sptr    getAxisUnits(ConvertToMDEvents const *const pHost);
    static preprocessed_detectors & getPrepDetectors(ConvertToMDEvents const *const pHost);
    static  double              getEi(ConvertToMDEvents const *const pHost);
    static int                  getEMode(ConvertToMDEvents const *const pHost);
//**<
  private:
    void init();
    void exec();
   /// Sets documentation strings for this algorithm
    virtual void initDocs();

    /// Progress reporter 
    std::auto_ptr<API::Progress> pProg;
 
  /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& convert_log;

 
   /// helper function which does exatly what it says
   void check_max_morethen_min(const std::vector<double> &min,const std::vector<double> &max);
   /// the variable which describes the number of the dimensions, currently used by algorithm. Changes in input properties can change this number;
   size_t n_activated_dimensions;
  
   /// pointer to input workspace;
   Mantid::DataObjects::Workspace2D_sptr inWS2D;
   // the variable which keeps preprocessed positions of the detectors if any availible (TODO: should it be a table ws?);
    static preprocessed_detectors det_loc;  
 /** the function, does preliminary calculations of the detectors positions to convert results into k-dE space ;
      and places the resutls into static cash to be used in subsequent calls to this algorithm */
    static void process_detectors_positions(const DataObjects::Workspace2D_const_sptr inWS2D);
     /// the names of the log variables, which are used as dimensions
//    std::vector<std::string> other_dim_names;
    /// the 
    
    /// minimal and maximal values for the workspace dimensions:
    std::vector<double>      dim_min,dim_max;
    // the names for the target workspace dimensions and properties of input MD workspace
    std::vector<std::string> targ_dim_names;
    // the units of target workspace dimensions and properties of input MD workspace dimensions
    std::vector<std::string> targ_dim_units;
  protected: //for testing
   /** function returns the list of the property names, which can be treated as additional dimensions present in current matrix workspace */
  void getAddDimensionNames(API::MatrixWorkspace_const_sptr inMatrixWS,std::vector<std::string> &add_dim_names,std::vector<std::string> &add_dim_units)const;
     
   /** function parses arguments entered by user, and identifies, which subalgorithm should be deployed on WS matrix as function of the input artuments and the WS format */
   std::string identifyMatrixAlg(API::MatrixWorkspace_const_sptr inMatrixWS,const std::string &Q_mode_req, const std::string &dE_mode_req,
                                 std::vector<std::string> &out_dim_names,std::vector<std::string> &out_dim_units);

   // Parts of the identifyMatrixAlg, separated for unit testing:
   std::string parseQMode(const std::string &Q_mode_req,const Strings &ws_dim_names,const Strings &ws_dim_units,Strings &out_dim_names,Strings &out_dim_units, int &nQdims);
   std::string parseDEMode(const std::string &Q_MODE_ID,const std::string &dE_mode_req,const Strings &ws_dim_units,Strings &out_dim_names, 
                                 Strings &out_dim_units, int &ndE_dims,std::string &natural_units, int &emode);
   std::string parseConvMode(const std::string &Q_MODE_ID,const std::string &natural_units,const Strings &ws_dim_units);

   /** identifies conversion subalgorithm to run on a workspace */
   std::string identifyTheAlg(API::MatrixWorkspace_const_sptr inMatrixWS,const std::string &Q_mode_req, const std::string &dE_mode_req,
                              const std::vector<std::string> &other_dim_names,std::vector<std::string> &targ_dim_names,std::vector<std::string> &targ_dim_units);
   /** function extracts the coordinates from additional workspace porperties and places them to proper position within array of coodinates */
   void fillAddProperties(std::vector<coord_t> &Coord,size_t nd,size_t n_ws_properties);

   /** function provides the linear representation for the transformation matrix, which translate momentums from laboratory to hkl coordinate system */
   std::vector<double> get_transf_matrix(const Kernel::V3D &u=Kernel::V3D(1,0,0), const Kernel::V3D &v=Kernel::V3D(0,1,0))const;
 
   //void process_ModQ_dE_();
   /// map to select an algorithm as function of the key, which describes it
    std::map<std::string, pMethod> alg_selector;
   /// map to select an workspace, as function of the dimensions number
    std::map<size_t, pWSCreator> ws_creator;

 
  private: 
   //--------------------------------------------------------------------------------------------------
   /** generic template to convert to any Dimensions workspace;
    * @param pOutWs -- pointer to initated target workspace, which should accomodate new events
    */
    template<size_t nd,Q_state Q, AnalMode MODE, CnvrtUnits CONV>
    void processQND(API::IMDEventWorkspace *const pOutWs);
    /// shalow class which is invoked from processQND procedure and describes the transformation from workspace coordinates to target coordinates
    /// presumably will be completely inlined
     template<Q_state Q, AnalMode MODE, CnvrtUnits CONV> 
     friend struct COORD_TRANSFORMER;
     /// helper class to orginize metaloop on number of dimensions
     template< size_t i, Q_state Q, AnalMode MODE, CnvrtUnits CONV >
     friend class LOOP_ND;

    /** template to build empty MDevent workspace with box controller and other palavra
     * @param split_into       -- the number of the bin the grid is split into
     * @param split_threshold  -- number of events in an intermediate cell?
     * @param split_maxDepth   -- maximal depth of the split tree;
    */
    template<size_t nd>
    API::IMDEventWorkspace_sptr  createEmptyEventWS(size_t split_into,size_t split_threshold,size_t split_maxDepth);

    // known momentum analysis modes ID-s;
    std::vector<std::string> Q_modes;
    // known energy transfer modes ID-s
    std::vector<std::string> dE_modes;
    // known conversion modes ID-s
    std::vector<std::string> ConvModes;

    // the ID of the unit, which is used in the expression to converty to QND. All other related elastic units should be converted to this one. 
    std::string  native_elastic_unitID; // currently it is Q
    // the ID of the unit, which is used in the expression to converty to QND. All other related inelastic units should be converted to this one. 
    std::string  native_inelastic_unitID; // currently it is energy transfer (DeltaE)

    // The Units (different for different Q and dE mode), for input workspace, for the selected sub algorihm to work with. 
    // Any other input workspace units have to be converted into these:
    std::string subalgorithm_units;
    // the variable describing energy transformation mode. (0 -- elastic, 1-- direct; 2 -- indirect)
    // assigned by 
    int emode;




 };
 
} // namespace Mantid
} // namespace MDAlgorithms

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
