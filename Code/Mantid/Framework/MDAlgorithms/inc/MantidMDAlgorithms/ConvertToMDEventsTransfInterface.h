#ifndef  H_CONVERT_TO_MDEVENTS_TRANSF_INTERFACE
#define  H_CONVERT_TO_MDEVENTS_TRANSF_INTERFACE
//
#include "MantidMDAlgorithms/ConvertToMDEventsParams.h"
#include "MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h"


#undef EXCLUDE_TRANSFORMATION_INTERFACE   // define interface
#undef EXCLUDE_Q_TRANSFORMATION_NOQ
#undef EXCLUDE_Q_TRANSFORMATION_MODQ
#undef EXCLUDE_Q_TRANSFORMATION_Q3D

// turn different parts of coordinate transformation code on and off.
#define EXCLUDE_TRANSFORMATION_INTERFACE
//#define EXCLUDE_Q_TRANSFORMATION_MODQ
//#define EXCLUDE_Q_TRANSFORMATION_NOQ
//#define EXCLUDE_Q_TRANSFORMATION_Q3D
//
namespace Mantid
{
namespace MDAlgorithms
{
/** Interface to set of internal classes used by ConvertToMDEvents algorithm and responsible for conversion of input workspace 
  * data into from 1 to 4 output dimensions as function of input parameters;
  * 
  * The class itself should never be instantiated (and this is actually impossible unless in debug mode, when adding new code)
  *
  ** The template below describes general ingerface to coordinate transformation:
  *
  *   Usual transformation constis of 4 steps
  * 1) Initiate the transformation itself.
  * 2) set-up, calculation and copying generic multidimensional variables which are not depenent on data
  * 3) set-up, calculation and copying the multidimensional variables which dependent on detectors id only 
  * 4) calculation of the multidimensional variables which depend on the data along x-axis of the workspace
  *    and possibly on detectors parameters. 
  * 
  *  Generic template defines interface to 4 functions which perform these four steps. 
  *
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
////----------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
// the module for the momentum transfer wavevector of the scattered neutrons
template<ConvertToMD::AnalMode MODE>
inline double k_trans(double Ei, double E_tr){
    UNUSED_ARG(Ei);UNUSED_ARG(E_tr);
    throw(Kernel::Exception::NotImplementedError("Generic K_tr should not be implemented"));
}
// Direct Inelastic analysis
template<>
inline double k_trans<ConvertToMD::Direct>(double Ei, double E_tr)
{
    return sqrt((Ei-E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
}
// Indirect Inelastic analysis
template<>
inline double k_trans<ConvertToMD::Indir>(double Ei, double E_tr)
{
    return sqrt((Ei+E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
}

#ifndef EXCLUDE_TRANSFORMATION_INTERFACE
template<ConvertToMD::QMode Q,ConvertToMD::AnalMode MODE,ConvertToMD::CnvrtUnits CONV,ConvertToMD::XCoordType TYPE,ConvertToMD::SampleType Sample>
struct CoordTransformer
{
      
    /**Template defines common interface to common part of the algorithm, where all variables
     * needed within the loop calculated before the loop starts. 
     *
     * In addition it caluclates the property-dependant coordinates 
     *
     * @param Coord        -- subalgorithm specific number of variables, calculated from properties and placed into specific place of the Coord vector;
     * @param n_ws_variabes -- subalgorithm specific number of variables, calculated from the workspace data
     *
     * @return true         -- if all Coord are within the range requested by algorithm. false otherwise
     *
     * has to be specialized
    */  

    bool calcGenericVariables(std::vector<coord_t> &Coord, size_t n_ws_variabes)
    {       
       throw(std::logic_error("CoordTransformer::This part of code should not be deployed"));
       return false;
    }
   
    /** template generalizes the code to calculate Y-variables within the detector's loop of processQND workspace
     * @param Coord  -- current Y coordinate, placed in the position of the Coordinate vector, specific for particular subalgorithm.
     * @param i    -- index of external loop, identifying current y-coordinate
     * 
     * @return true   -- if all Coord are within the range requested by algorithm. false otherwise   
     * 
     *  some default implementations possible (e.g mode Q3D,ragged  Any_Mode( Direct, indirect,elastic), 
     */
    
    bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)
    {       
       throw(std::logic_error("CoordTransformer::This part of code should not be deployed"));
       return false;
    }

    /** template generalizes the code to calculate all remaining coordinates, defined within the inner loop
     * @param X    -- vector of X workspace values
     * @param i    -- index of external loop, identifying generic y-coordinate
     * @param j    -- index of internal loop, identifying generic x-coordinate
     * 
     * @param Coord  -- subalgorithm specific number of coordinates, placed in the proper position of the Coordinate vector
     * @return true  -- if all Coord are within the range requested by algorithm. false otherwise   
     *
     * has to be specialized
     */
    //calcMatrixCoord(X,i,j,Coord)

    bool calcMatrixCoord(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord)const
    {       
       throw(std::logic_error("CoordTransformer::This part of code should not be deployed"));
       return false;
    }
  
 /** template generalizes the code to calculate all remaining coordinates, defined within the inner loop
    * given that the input described by sinble value only
     * @param X    -- X workspace value
     * 
     * @param Coord  -- subalgorithm specific number of coordinates, placed in the proper position of the Coordinate vector
     * @return true  -- if all Coord are within the range requested by algorithm. false otherwise   
     *
     * has to be specialized    */
    bool calc1MatrixCoord(const double & X,std::vector<coord_t> &Coord)const
     {       
       throw(std::logic_error("CoordTransformer::This part of code should not be deployed"));
       return false;
     }
    /** template generalizes the conversion of single x-variable using unit conversion as the first step 
     * @param X    -- X workspace value
     * 
     * @param Coord  -- subalgorithm specific number of coordinates, placed in the proper position of the Coordinate vector
     * @return true  -- if all Coord are within the range requested by algorithm. false otherwise   
     *
     * has to be specialized     */
    bool convertAndCalcMatrixCoord(const double & X,std::vector<coord_t> &Coord)const
    {       
       throw(std::logic_error("CoordTransformer::This part of code should not be generated or deployed"));
       return false;
    }
    /** set up transformation and retrieve the pointer to the incorporating class, which runs the transformation and provides 
      * necessary variables */

    void setUpTransf(IConvertToMDEventsWS *)
    {
       throw(std::logic_error("CoordTransformer::This part of code should not be deployed"));
    }
  
}; // end CoordTransformer structure:
#else
template<ConvertToMD::QMode Q,ConvertToMD::AnalMode MODE,ConvertToMD::CnvrtUnits CONV,ConvertToMD::XCoordType TYPE,ConvertToMD::SampleType Sample>
struct CoordTransformer
{
    bool calcGenericVariables(std::vector<coord_t> &Coord, size_t n_ws_variabes); 
    bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i);
    bool calcMatrixCoord(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord)const;
    bool calc1MatrixCoord(const double & X,std::vector<coord_t> &Coord)const;
    bool convertAndCalcMatrixCoord(const double & X,std::vector<coord_t> &Coord)const;
    void setUpTransf(IConvertToMDEventsWS *);  
}; // end CoordTransformer structure:

#endif

} // End MDAlgorighms namespace
} // End Mantid namespace

#endif
