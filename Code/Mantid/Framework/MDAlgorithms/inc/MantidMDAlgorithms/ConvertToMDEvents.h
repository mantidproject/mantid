#ifndef MANTID_MD_CONVERT2_MDEVENTS
#define MANTID_MD_CONVERT2_MDEVENTS

#include "MantidMDAlgorithms/ConvertToMDEventsParams.h"
#include "MantidMDAlgorithms/IConvertToMDEventsMethods.h"
#include "MantidMDEvents/MDWSDescription.h"
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
    // helper function to obtain some characteristics of the workspace and invoked algorithm
   static  double      getEi(ConvertToMDEvents const *const pHost);
   static int          getEMode(ConvertToMDEvents const *const pHost);
//**<
  private:
    void init();
    void exec();
   /// Sets documentation strings for this algorithm
    virtual void initDocs();
   /// pointer to the input workspace;
   Mantid::API::MatrixWorkspace_sptr inWS2D;
 
   /// the variable which keeps preprocessed positions of the detectors if any availible (TODO: should it be a table ws and separate algorithm?);
   static PreprocessedDetectors det_loc;  
   /// the pointer to class which is responsible for adding data to N-dimensional workspace;
   boost::shared_ptr<MDEvents::MDEventWSWrapper> pWSWrapper;
  /// progress reporter
   std::auto_ptr<API::Progress > pProg;
    /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& convert_log;
  //------------------------------------------------------------------------------------------------------------------------------------------
    ConvertToMDEventsParams ParamParser;
    /// string -Key to identify the algorithm -- rather for testing and debugging, though may be reliet upon somewhere by bad practice
    std::string algo_id;


    /// the properties of the requested target MD workpsace:
    MDEvents::MDWSDescription TWS;
 
    protected: //for testing
        static Mantid::Kernel::Logger & getLogger();

   /** function provides the linear representation for the transformation matrix, which translate momentums from laboratory to crystal cartezian 
       (C)- Busing, Levi 1967 coordinate system */
   std::vector<double> getTransfMatrix(API::MatrixWorkspace_sptr inWS2D,MDEvents::MDWSDescription &TargWSDescription, 
                                       bool is_powder=false)const;
   /**function returns the linear representation for the transformation matrix, which transforms momentums from laboratory to target coordinate system
     defined by existing workspace */
    std::vector<double> getTransfMatrix( API::IMDEventWorkspace_sptr spws,API::MatrixWorkspace_sptr inWS,bool is_powder=false)const; 

   /// get transformation matrix currently defined for the algorithm
   std::vector<double> getTransfMatrix()const{return TWS.rotMatrix;}
   /// construct meaningful dimension names:
   void buildDimNames(MDEvents::MDWSDescription &TargWSDescription);
 
   /// map to select an algorithm as function of the key, which describes it
   std::map<std::string, IConvertToMDEventsMethods *> alg_selector;
  private: 
   //--------------------------------------------------------------------------------------------------   
     /// helper class to orginize metaloop instantiating various subalgorithms 
     template<Q_state Q,size_t N_ALGORITHMS >
     friend class LOOP_ALGS;
  
  
    /** helper function which verifies if projection vectors are specified and if their values are correct when present.
      * sets default values u and v to [1,0,0] and [0,1,0] if not present or any error. */
    void checkUVsettings(const std::vector<double> &ut,const std::vector<double> &vt,MDEvents::MDWSDescription &TargWSDescription)const;
 };

} // namespace Mantid
} // namespace MDAlgorithms

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
