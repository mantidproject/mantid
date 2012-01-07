#ifndef MANTID_MD_CONVERT2_MDEVENTS
#define MANTID_MD_CONVERT2_MDEVENTS
    
#include "MantidMDAlgorithms/ConvertToMDEventsMethods.h"
#include "MantidMDEvents/BoxControllerSettingsAlgorithm.h"

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
 
 // vectors of strings are here everywhere
  typedef  std::vector<std::string> Strings;

 
/// Convert to MD Events class itself:
  class DLLExport ConvertToMDEvents  : public MDEvents::BoxControllerSettingsAlgorithm
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

//**> helper functions: To assist with units conversion done by separate class and get access to some important internal states of the algorithm
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

   /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& convert_log;
   /// pointer to the input workspace;
   Mantid::API::MatrixWorkspace_sptr inWS2D;
 
   /// the variable which keeps preprocessed positions of the detectors if any availible (TODO: should it be a table ws and separate algorithm?);
    static preprocessed_detectors det_loc;  
    /// the properties of the requested target MD workpsace:
    MDWSDescription TWS;
   /// the pointer to class which is responsible for adding data to N-dimensional workspace;
    boost::shared_ptr<MDEvents::MDEventWSWrapper> pWSWrapper;
   protected: //for testing
 
   //>---> Parts of the identifyTheAlg;
   /** function returns the list of the property names, which can be treated as additional dimensions present in current matrix workspace */
   void getAddDimensionNames(API::MatrixWorkspace_const_sptr inMatrixWS,std::vector<std::string> &add_dim_names,std::vector<std::string> &add_dim_units)const;
   /** function parses arguments entered by user, and identifies, which subalgorithm should be deployed on WS  as function of the input artuments and the WS format */
   std::string identifyMatrixAlg(API::MatrixWorkspace_const_sptr inMatrixWS,const std::string &Q_mode_req, const std::string &dE_mode_req,
                                 std::vector<std::string> &out_dim_names,std::vector<std::string> &out_dim_units);
   //>---> Parts of the identifyMatrixAlg, separated for unit testing:
   // identify Q - mode
   std::string parseQMode(const std::string &Q_mode_req,const Strings &ws_dim_names,const Strings &ws_dim_units,Strings &out_dim_names,Strings &out_dim_units, int &nQdims);
   // identify energy transfer mode
   std::string parseDEMode(const std::string &Q_MODE_ID,const std::string &dE_mode_req,const Strings &ws_dim_units,Strings &out_dim_names, 
                                 Strings &out_dim_units, int &ndE_dims,std::string &natural_units);
   // indentify input units conversion mode
   std::string parseConvMode(const std::string &Q_MODE_ID,const std::string &natural_units,const Strings &ws_dim_units);
   // identify what kind of input workspace is there:
   std::string parseWSType(API::MatrixWorkspace_const_sptr inMatrixWS)const;
   //<---< Parts of the identifyMatrixAlg;
   /** identifies the ID of the conversion subalgorithm to run on a workspace */
   std::string identifyTheAlg(API::MatrixWorkspace_const_sptr inMatrixWS,const std::string &Q_mode_req, const std::string &dE_mode_req,
                              const std::vector<std::string> &other_dim_names,MDWSDescription &TargWSDescription);
   //<---< Parts of the identifyTheAlg;

 

   /** function provides the linear representation for the transformation matrix, which translate momentums from laboratory to notional (fractional) coordinate system */
   std::vector<double> getTransfMatrix(API::MatrixWorkspace_sptr inWS2D,const Kernel::V3D &u=Kernel::V3D(1,0,0), const Kernel::V3D &v=Kernel::V3D(0,1,0), 
                                       bool is_powder=false)const;

   /// map to select an algorithm as function of the key, which describes it
   std::map<std::string, IConvertToMDEventMethods *> alg_selector;


    // strictly for testing!!!
    void setAlgoID(const std::string &newID){
        this->algo_id=newID;
    }
    // strictly for testing!!!
    void setAlgoUnits(int emode){
        if(emode==0){
            this->subalgorithm_units=native_elastic_unitID;
        }
        if(emode==1||emode==2){
            this->subalgorithm_units=native_inelastic_unitID;
        }
    }
  private: 
   //--------------------------------------------------------------------------------------------------
   ///** generic template to convert to any Dimensions workspace from a histohram workspace   */
   // template<Q_state Q, AnalMode MODE, CnvrtUnits CONV>
   // void processQNDHWS();
   ///** generic template to convert to any Dimensions workspace from an Event workspace   */
   // template<Q_state Q, AnalMode MODE, CnvrtUnits CONV>
   // void processQNDEWS();

    // temporary
    template<Q_state Q, AnalMode MODE, CnvrtUnits CONV>
    friend class ProcessHistoWS;
    template<Q_state Q, AnalMode MODE, CnvrtUnits CONV,XCoordType XTYPE> 
    friend struct COORD_TRANSFORMER;
  

     /// helper class to orginize metaloop on various algorithm options
     template<Q_state Q,size_t N_ALGORITHMS >
     friend class LOOP_ND;
  
 
    /// known momentum analysis mode ID-s (symbolic representation of correspondent enum);
    std::vector<std::string> Q_modes;
    /// known energy transfer modes ID-s (symbolic representation of correspondent enum)
    std::vector<std::string> dE_modes;
    /// known conversion modes ID-s       (symbolic representation of correspondent enum)
    std::vector<std::string> ConvModes;
    /// supported input workspace types  (names of supported workspace types)
    std::vector<std::string> SupportedWS;

    /// the ID of the unit, which is used in the expression to converty to QND. All other related elastic units should be converted to this one. 
    std::string  native_elastic_unitID; // currently it is Q
    /// the ID of the unit, which is used in the expression to converty to QND. All other related inelastic units should be converted to this one. 
    std::string  native_inelastic_unitID; // currently it is energy transfer (DeltaE)

    // The Units (different for different Q and dE mode), for input workspace, for the selected sub algorihm to work with. 
    // Any other input workspace units have to be converted into these:
    std::string subalgorithm_units;
  // string -Key to identify the algorithm -- rather for testing and debugging 
    std::string algo_id;
    //
    std::vector<double> getTransfMatrix()const{return TWS.rotMatrix;}
  
    /** helper function which verifies if projection vectors are specified and if their values are correct when present.
    * sets defaults [1,0,0] and [0,1,0] if not present or any error. */
   void checkUVsettings(const std::vector<double> &ut,const std::vector<double> &vt,Kernel::V3D &u,Kernel::V3D &v)const;
 };
 
} // namespace Mantid
} // namespace MDAlgorithms

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
