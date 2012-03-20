#ifndef H_CONVERT2MDEVENTS_PARAMS
#define H_CONVERT2MDEVENTS_PARAMS
#include <vector>
#include <string>
#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidMDEvents/MDWSDescription.h"

namespace Mantid
{
namespace MDAlgorithms
{
/** Helper class describes the possible properties of the algorithm, converting a workspace to a MDEventWorkspace 
  *
  *  It is used to convert user input and input data into the key, which corresponds to appropriate subalgorithm, 
  *  performing the actual conversion.
  *
  * @date 14-03-2012

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

        File/ change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

  /// known sates for algorithms, caluclating momentums
  enum Q_state
  {
       modQ,    //< calculate mod Q
       Q3D,     //< calculate 3 component of Q in fractional coordinate system.
       NoQ,     //< no Q transformatiom, just copying values along X axis (may be with units transformation)
       NQStates  // number of various recognized Q-analysis modes used to terminate Q-state algorithms metalooop.
   };
  /**  known analysis modes, arranged according to emodes 
    *  It is importent to assign enums proper numbers, as direct correspondence between enums and their emodes 
    *  used by the external units conversion algorithms and this algorithm, so the agreement should be the stame     */
  enum AnalMode
  {  
      Elastic = 0,  //< int emode = 0; Elastic analysis
      Direct  = 1,  //< emode=1; Direct inelastic analysis mode
      Indir   = 2,  //< emode=2; InDirect inelastic analysis mode
      ANY_Mode,      //< couples with NoQ, means just copying existing data (may be doing units conversion), also used to terminate AnalMode algorithms initiation metaloop
      NAnalModes
  };
  /** enum describes if there is need to convert workspace units and different unit conversion modes 
   * this modes are identified by algorithm from workpace parameters and user input.  See UnitConversion algorithm for different modes meaning  */
  enum CnvrtUnits   // here the numbers are specified to enable proper metaloop on conversion
  {
      ConvertNo,   //< no, input workspace has the same units as output workspace or in units used by Q-dE algorithms naturally
      ConvFast , //< the input workspace has different units from the requested and fast conversion is possible
      ConvByTOF,   //< conversion possible via TOF
      ConvFromTOF,  //< Input workspace units are the TOF 
      NConvUintsStates //< number of various recognized unit conversion modes used to terminate CnvrtUnits algorithms initiation metalooop.
  };
  enum InputWSType  // Algorithm recognizes 2 input workspace types with different interface. 
  {
      Ws2DHistoType, //< 2D matirix workspace with the x-axis for each sign
      EventWSType,   //< Event worskapce
      NInWSTypes     //< number of input ws types which should be treated differently
  };
/// way to treat the X-coorinate in the workspace:
  enum XCoordType
  {
        Histohram, // typical for Matrix workspace -- deploys central average 0.5(X[i]+X[i+1]); other types of averaging are possible if needed 
        Centered   // typical for events
  };

/** describes default dimensions ID currently used by multidimensional workspace
 * 
 *  DimensionID is the short name which used to retrieve this dimesnion from MD workspace.
 *  The names themself are defined in constructor
 */
enum defaultDimID
{
    modQ_ID,  //< the defauld |Q| id for mod Q or powder mode
    Q1_ID,    //< 1 of 3 dimID in Q3D mode
    Q2_ID,    //< 2 of 3 dimID in Q3D mode
    Q3_ID,    //< 3 of 3 dimID in Q3D mode
    dE_ID,    //< energy transfer ID
    nDefaultID //< ID conunter
};

// vectors of strings are here everywhere
typedef  std::vector<std::string> Strings;

 /** the structure, which provides helper variables and varions text parameters for the algorithm */
class DLLExport ConvertToMDEventsParams
{

 //
    /// known momentum analysis mode ID-s (symbolic representation of correspondent enum);
    Strings Q_modes;
    /// known energy transfer modes ID-s (symbolic representation of correspondent enum)
    Strings dE_modes;
    /// known conversion modes ID-s       (symbolic representation of correspondent enum)
    Strings ConvModes;
    /// supported input workspace types  (names of supported workspace types )
    Strings SupportedWS;

    /// the ID of the unit, which is used in the expression to converty to QND. All other related elastic units should be converted to this one. 
    std::string  native_elastic_unitID; // currently it is Q
    /// the ID of the unit, which is used in the expression to converty to QND. All other related inelastic units should be converted to this one. 
    std::string  native_inelastic_unitID; // currently it is energy transfer (DeltaE)
   /**  The Units (different for different Q and dE mode), for input workspace, for the selected sub algorihm to work with. 
      *  Any other input workspace units have to be converted into these, and these have to correspond to actual units, defined in workspace */
    std::string natural_units;
    // the vector describes default dimension names, specified along the axis if no names are explicitly requested;
    Strings default_dim_ID;
 public:

  /** The main purpose of this class: identifies the ID of the conversion subalgorithm to run on a workspace */
  std::string identifyTheAlg(API::MatrixWorkspace_const_sptr inMatrixWS,const std::string &Q_mode_req, const std::string &dE_mode_req,
                               const Strings &other_dim_names,bool convert_to_hkl,size_t maxNdim,MDEvents::MDWSDescription &TargWSDescription);
  /** get the identifier of the correspondent algorithm as function of integer ws ID-s. This function is used during subalgorithm instanciation
    * to generate algorithmID, which will be used later (through funcion identifyTheAlg) to retrive suitable subalgorithm.  */
  std::string getAlgoID(Q_state Q,AnalMode Mode,CnvrtUnits Conv,InputWSType WS)const;

  /** auxiliary function working opposite to getAlgoID and returns conversion modes given the algorithm ID */
  void  getAlgoModes(const std::string &AlgoID, Q_state &Q,AnalMode &Mode,CnvrtUnits &Conv,InputWSType &WS);

  //----------> service and helper functions

   /// list of all existing mode names to convert momentum
   Strings getQModes()const{return Q_modes;}
   /// list of all existing mode names to convert energy transfer
   Strings getDEModes()const{return dE_modes;}
   /// function returns default names for dimensions in different Q analysis modes;
   Strings getDefaultQNames(Q_state Qmode=modQ,AnalMode=Direct)const;
    /// constructor
    ConvertToMDEventsParams(); 

  //>---> Parts of the identifyMatrixAlg, separated for unit testing:
  /// indentify input units conversion mode
  std::string parseConvMode(const std::string &Q_MODE_ID,const std::string &UnitsToConvert2,const Strings &ws_dim_unit)const;
  /// identify momentum transfer mode
  std::string parseQMode(const std::string &Q_mode_req,const Strings &ws_dim_names,const Strings &ws_dim_units,Strings &out_dim_names,Strings &out_dim_units, int &nQdims)const;
   /// identify energy transfer mode
  std::string parseDEMode(const std::string &Q_MODE_ID,const std::string &dE_mode_req,const Strings &ws_dim_units,                                
                                Strings &out_dim_IDs,Strings &out_dim_units,int &ndE_dims,std::string &natural_units)const;
 /// identify what kind of input workspace is there:
  std::string parseWSType(API::MatrixWorkspace_const_sptr inMatrixWS, MDEvents::MDWSDescription &TargWSDescription)const;
  /** function returns the list of the property names, which can be treated as additional dimensions present in current matrix workspace */
   void getAddDimensionNames(API::MatrixWorkspace_const_sptr inMatrixWS,Strings &add_dim_names,Strings &add_dim_units)const;
  //<---< Parts of the identifyMatrixAlg;
  /** function parses arguments entered by user, and identifies, which subalgorithm should be deployed on WS  as function of the input artuments and the WS format */
  std::string identifyMatrixAlg(API::MatrixWorkspace_const_sptr inMatrixWS,const std::string &Q_mode_req, const std::string &dE_mode_req,
                                Strings &out_dim_IDs,Strings &out_dim_units,MDEvents::MDWSDescription &TargWSDescription);


   /// helper function to obtain the eMode from existing algorithm ID;
   int getEMode(const std::string &AlgID)const;
   /// helper function to obtain input energy of neurtons from input workspace. Will work properly only for the workspace where this energy is defined. 
   double getEi(API::MatrixWorkspace_const_sptr inMatrixWS)const;

private:
   static Mantid::Kernel::Logger& convert_log;

 };

 } // end namespace
 }

#endif