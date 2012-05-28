#ifndef H_IMD_TRANSFORMATION
#define H_IMD_TRANSFORMATION
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid
{
namespace MDAlgorithms
{
/** Interface to set of sub-classes used by ConvertToMDEvents algorithm and responsible for conversion of input workspace 
  * data into MD events.
  *
  * its output is the vector of n-dimensions which contains the values of n-coorinates in n-dimensional space
  * 
  *
  *   Usual transformation constis of 4 steps
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
// forvard declaration for a class, which would provide all necessary parameters for the transformation; should be refactored;
class ConvToMDEventsBase;

namespace ConvertToMD
{
    /* enum describes known eneergy conversion/analysis modes
    *  It is important to assign enums proper numbers, as direct correspondence between enums and their emodes 
    *  used by the external units conversion algorithms within the Mantid, so the agreement should be the stame     
    */
    enum EModes
    {
          Elastic = 0,  //< int emode = 0; Elastic analysis
          Direct  = 1,  //< emode=1; Direct inelastic analysis mode
          Indir   = 2,  //< emode=2; InDirect inelastic analysis mode
          No_DE         //< couples with NoNonentum analysis, means just copying existing data (may be doing units conversion), 
                       // it is also the counter for the number of availible modes, used to initiate the mode names
    };
}


class MDTransfInterface
{
public:
    /** the function returns the unit ID for the input units, the particular transformation expects. 
     if one wants the transformation to be meaningful, Input X-coordinates
     used by the transformation have to be expressed in the uinits  specified */
    virtual const std::string usedUnitID()const=0;
    /**The function returns the name, under which the transformation would be known to user */
    virtual const std::string transfID()const=0;
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
    virtual bool calcMatrixCoord(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord)const
    {
       UNUSED_ARG(i);
       double X_ev =double(0.5*(X[j]+X[j+1])); // ! POSSIBLE FACTORY HERE !!! if the histogram interpolation is different
       return calcMatrixCoord(X_ev,Coord);
    }
  
 /**  The method to calculate all remaining coordinates, defined within the inner loop
    * given that the input described by sinble value only
     * @param X    -- X workspace value
     * 
     * @param Coord  -- subalgorithm specific number of coordinates, placed in the proper position of the Coordinate vector
     * @return true  -- if all Coord are within the range requested by algorithm. false otherwise   
     * */
    virtual bool calcMatrixCoord(const double & X,std::vector<coord_t> &Coord)const=0;

    /** set up transformation and retrieve the pointer to the incorporating class, which runs the transformation and can provide
      * all variables necessary for the conversion */
    virtual void initialize(const ConvToMDEventsBase &)=0;
  
    /** MD transformation can often be used together with energy analysis mode; This function should be overloaded 
       if the transformation indeed can do the energy conversion */
    virtual std::vector<std::string> getEmodes()const{return std::vector<std::string>(1,std::string("No dE"));}
    virtual std::string getEmode(ConvertToMD::EModes Mode)const{UNUSED_ARG(Mode); return std::string("No dE");}

    /** return the number of dimensions, calculated by the transformation from the workspace. This numebr is usually varies from 1 to 4*/
    virtual int getNMatrixDimensions()const=0;

    virtual ~MDTransfInterface(){};
}; 

typedef boost::shared_ptr<MDTransfInterface> MDTransf_sptr; 

} // End MDAlgorighms namespace
} // End Mantid namespace

#endif