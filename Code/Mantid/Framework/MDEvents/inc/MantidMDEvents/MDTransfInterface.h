#ifndef H_IMD_TRANSFORMATION
#define H_IMD_TRANSFORMATION
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidMDEvents/MDTransfDEHelper.h"
#include "MantidMDEvents/MDWSDescription.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace MDEvents
{
/** Interface to set of sub-classes used by ConvertToMDEvents algorithm and responsible for conversion of input workspace 
  * data into MD events.
* The inferface provide information for two tasks. 
* 1) Definition of target MD workspace properties and 
* 2) Calculation of MD coordinates for single measurement 

1) First task resolved during algorithm initialization and defines the number of dimensions, coordinate system, dimension units and ID-s etc.
  This information is used when creating the target MD workspace or checking if existing MD workspace can be used as target for the selected subalgorithm

2) Second task achieved during conversion itself. The subclass will works with input workpsace and convert a single point of the input ws into 
   the vector of MD coordinates.  
  * MD coordinates are stored in the vector of n-dimensions* 

  *   Usual transformation occurs in 4 stages
  *
  * 1) Initiate the transformation itself.
  * 2) set-up, calculation and copying generic multidimensional variables which are not depenent on data (logs)
  * 3) set-up, calculation and copying the multidimensional variables which dependent on detectors id only 
  * 4) calculation of the multidimensional variables which depend on the data along x-axis of the workspace
  *    and possibly on detectors parameters (values along y-axis)
  * 
  *
  * @date 16-05-2012

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

class MDTransfInterface
{
public:
    /**The method returns the name, under which the transformation would be known to a user */
    virtual const std::string transfID()const=0;
 /** MD transformation can often be used together with energy analysis mode; This function should be overloaded 
       if the transformation indeed can do the energy analysis */
    virtual std::vector<std::string> getEmodes()const{return std::vector<std::string>(1,std::string("No dE"));}
 
//***************> the method below involwed in the calculations of MD coordinates
    /** Method deployed out of the loop and calculates all variables needed within the loop.
     * In addition it calculates the property-dependant coordinates, which do not depend on workspace
     *
     * @param Coord        --  vector of ND-coordinates. 
     *                         Method calculates subalgorithm specific number of variables, 
     *                         calculated from properties and placed into specific place of the Coord vector;
     * @param n_ws_variabes -- specific number of additional variables, calculated from the workspace data
     *
     * @return true         -- if all Coord are within the range requested by the conversion algorithm. false otherwise
    */  
    virtual bool calcGenericVariables(std::vector<coord_t> &Coord, size_t n_ws_variabes)=0;
   
    /** generalizes the code to calculate Y-variables within the detector's loop of the  workspace
     * @param Coord  -- current Y coordinate, placed in the position of the Coordinate vector, specific for particular subalgorithm.
     * @param i    -- index of external loop, identifying current y-coordinate
     * 
     * @return true   -- if all Coord are within the range requested by algorithm. false otherwise   
     * 
     */   
    virtual bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)=0;
    /** Calculate all remaining coordinates, defined within the inner loop
     * @param X    -- vector of X workspace values
     * @param i    -- index of external loop, identifying generic y-coordinate
     * @param j    -- index of internal loop, identifying generic x-coordinate
     * 
     * @param Coord  -- subalgorithm specific number of coordinates, placed in the proper position of the Coordinate vector
     * @return true  -- if all Coord are within the range requested by algorithm. false otherwise   
     *
     * in most existing algorighms X does not depend on Y coordinate, so we can place generalization here; 
     * It should be overridden if the dependence on Y coordinate do exist.
     */
    virtual bool calcMatrixCoordinates(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord)const
    {
       UNUSED_ARG(i);
       double X_ev =double(0.5*(X[j]+X[j+1])); // ! POSSIBLE FACTORY HERE !!! if the histogram interpolation is different
       return calcMatrixCoord(X_ev,Coord);
    }
  
 /**  The method to calculate all remaining coordinates, defined within the inner loop
    *  given that the input described by sinble value only
     * @param X    -- X workspace value
     * 
     * @param Coord  -- subalgorithm specific number of coordinates, placed in the proper position of the Coordinate vector
     * @return true  -- if all Coord are within the range requested by algorithm. false otherwise   
     * */
    virtual bool calcMatrixCoord(const double & X,std::vector<coord_t> &Coord)const=0;

    /* clone method allowing to provide the copy of the particular class */
    virtual MDTransfInterface * clone() const = 0;
    // destructor
    virtual ~MDTransfInterface(){};
    /** set up transformation from the class, which can provide all variables necessary for the conversion */
    virtual void initialize(const MDWSDescription &)=0;

//***************> the methods below are mainly involved in defining the target workspace properties. 
//                 Thay also can be involwed in the preparation to calculations of MD coordinates 
// WARNING!!!! THESE METHODS ARE USED BEFORE INITIALIZE IS EXECUTED SO THEY CAN NOT RELY ON THE CONTENTS OF THE CLASS TO BE DEFINED (THEY ARE VIRTUAL STATIC METHODS)

    /** returns the unit ID for the input units, the particular transformation expects. 
     if one wants the transformation to be meaningful, the X-coordinates of input workspace 
     used by the transformation have to be expressed in the uinits  with ID, returned by this method */
    virtual const std::string inputUnitID(ConvertToMD::EModes dEmode, API::MatrixWorkspace_const_sptr inWS)const=0;
    /** The transformation generates output MD events in particular units. This method returns these Units ID-s */ 
    virtual std::vector<std::string> outputUnitID(ConvertToMD::EModes dEmode, API::MatrixWorkspace_const_sptr inWS)const = 0;

      /** when one builds MD workspace, he needs a dimension names/ID-s which can be different for different Q-transformatons and in different E-mode 
       The position of each dimID in the output vector should correspond the position of each coordinate in the Coord vector     */
    virtual std::vector<std::string> getDefaultDimID(ConvertToMD::EModes dEmode, API::MatrixWorkspace_const_sptr inWS)const = 0;

    /** return the number of dimensions, calculated by the transformation from the workspace. This number is usually varies from 1 to 4
      * and depends on emode and possibly on some WS parameters.     */
    virtual unsigned int getNMatrixDimensions(ConvertToMD::EModes mode,API::MatrixWorkspace_const_sptr inWS)const=0;
    
}; 

typedef boost::shared_ptr<MDTransfInterface>       MDTransf_sptr; 
typedef boost::shared_ptr<const MDTransfInterface> MDTransf_const_sptr; 

} // End MDAlgorighms namespace
} // End Mantid namespace

#endif