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

  // known sates for algorithms, caluclating Q-values
  enum Q_state{
       NoQ,
       modQ,
       Q3D
   };
//
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
    std::vector<std::string> dim_names;
    // the units of target workspace dimensions and properties of input MD workspace dimensions
    std::vector<std::string> dim_units;
  protected: //for testing
   /** function returns the list of names, which can be treated as dimensions present in current matrix workspace */
   std::vector<std::string > getDimensionNames(API::MatrixWorkspace_const_sptr inMatrixWS)const;
   /** The function to identify the target dimensions and target uints which can be obtained from workspace dimensions   */
   void getDimensionNamesFromWSMatrix(API::MatrixWorkspace_const_sptr inMatrixWS,std::vector<std::string> &ws_dimensions,std::vector<std::string> &ws_units)const;
   
   /** function processes arguments entered by user, calculates the number of dimensions and tries to establish which algorithm should be deployed;   */
   std::string identify_the_alg(const std::vector<std::string> &dim_names_availible,const std::string &Q_dim_requested, const std::vector<std::string> &other_dim_selected,size_t &nDims);

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

    /**Template defines common interface to common part of the algorithm, where all variables
     * needed within the loop calculated outside of the loop. 
     * In addition it caluclates the property-dependant coordinates 
     *
     * @param n_ws_variabes -- subalgorithm specific number of variables, calculated from the workspace data
     *
     * @return Coord        -- subalgorithm specific number of variables, calculated from properties and placed into specific place of the Coord vector;
     * @return true         -- if all Coord are within the range requested by algorithm. false otherwise
    */
    template<Q_state Q>
    bool calc_generic_variables(std::vector<coord_t> &Coord, size_t n_ws_variabes){
    UNUSED_ARG(Coord); UNUSED_ARG(n_ws_variabes);return false;}

   
    /** template generalizes the code to calculate Y-variables within the external loop of processQND workspace
     * @param X    -- vector of X workspace values
     * @param i    -- index of external loop, identifying current y-coordinate
     * 
     * @return Coord -- current Y coordinate, placed in the position of the Coordinate vector, specific for particular subalgorithm.    
     * @return true         -- if all Coord are within the range requested by algorithm. false otherwise   */
    template<Q_state Q>
    bool calculate_y_coordinate(std::vector<coord_t> &Coord,size_t i){
    UNUSED_ARG(Coord); UNUSED_ARG(i);return false;}

    /** template generalizes the code to calculate all remaining coordinates, defined within the inner loop
     * @param X    -- vector of X workspace values
     * @param i    -- index of external loop, identifying generic y-coordinate
     * @param j    -- index of internal loop, identifying generic x-coordinate
     * 
     * @return Coord --Subalgorithm specific number of coordinates, placed in the proper position of the Coordinate vector   
     * @return true  -- if all Coord are within the range requested by algorithm. false otherwise   */
    template<Q_state Q>
    bool calculate_ND_coordinates(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord){
        UNUSED_ARG(X); UNUSED_ARG(i); UNUSED_ARG(j); UNUSED_ARG(Coord);
        return false;}

   /** generic template to convert to any Dimensions workspace;
    * @param pOutWs -- pointer to initated target workspace, which should accomodate new events
    */
    template<size_t nd,Q_state Q>
    void processQND(API::IMDEventWorkspace *const pOutWs);    
    //--------------------------------------------------------------------------------------------------
    // the variables used for exchange data between different specific parts of the generic ND algorithm:
    // pointer to Y axis of MD workspace
    API::NumericAxis *pYAxis;
    // the energy of the incident neutrons
    double Ei;
    // the wavevector of incident neutrons
    double ki;
    // the matrix which transforms the neutron momentums from lablratory to crystall coordinate system. 
    std::vector<double> rotMat;
    //---------------------------------------------------------------------------------------------------
    /** template to build empty MDevent workspace with box controller and other palavra
     * @param split_into       -- the number of the bin the grid is split into
     * @param split_threshold  -- number of events in an intermediate cell?
     * @param split_maxDepth   -- maximal depth of the split tree;
    */
    template<size_t nd>
    API::IMDEventWorkspace_sptr  createEmptyEventWS(size_t split_into,size_t split_threshold,size_t split_maxDepth);

 };
 
} // namespace Mantid
} // namespace MDAlgorithms

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
