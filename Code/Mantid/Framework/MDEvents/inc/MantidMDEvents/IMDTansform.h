#ifndef H_IMD_TRANSFORMATION
#define H_IMD_TRANSFORMATION


namespace Mantid
{
namespace MDAlgorithms
{
/** Interface to set of sub-classes used by ConvertToMDEvents algorithm and responsible for conversion of input workspace 
  * data into MD events.
  *
  * it fills in vector of n-dimensions which contains the values 
  * 
  *
  *   Usual transformation constis of 4 steps
  * 1) Initiate the transformation itself.
  * 2) set-up, calculation and copying generic multidimensional variables which are not depenent on data
  * 3) set-up, calculation and copying the multidimensional variables which dependent on detectors id only 
  * 4) calculation of the multidimensional variables which depend on the data along x-axis of the workspace
  *    and possibly on detectors parameters. 
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
class IMDTransform
{
      
    /** Method deployed out of the loop and calculates all variables needed within the loop.
     * In addition it calculates the property-dependant coordinates, which do not depend on workspace
     *
     * @param Coord        -- subalgorithm specific number of variables, calculated from properties and placed into specific place of the Coord vector;
     * @param n_ws_variabes --  specific number of variables, calculated from the workspace data
     *
     * @return true         -- if all Coord are within the range requested by algorithm. false otherwise
    */  

    virtual bool calcGenericVariables(std::vector<coord_t> &Coord, size_t n_ws_variabes)=0;
   
    /**  generalizes the code to calculate Y-variables within the detector's loop of processQND workspace
     * @param Coord  -- current Y coordinate, placed in the position of the Coordinate vector, specific for particular subalgorithm.
     * @param i    -- index of external loop, identifying current y-coordinate
     * 
     * @return true   -- if all Coord are within the range requested by algorithm. false otherwise   
     * 
     */   
    bool calcYDepCoordinates(std::vector<coord_t> &Coord,size_t i)=0;
    /** Calculate all remaining coordinates, defined within the inner loop
     * @param X    -- vector of X workspace values
     * @param i    -- index of external loop, identifying generic y-coordinate
     * @param j    -- index of internal loop, identifying generic x-coordinate
     * 
     * @param Coord  -- subalgorithm specific number of coordinates, placed in the proper position of the Coordinate vector
     * @return true  -- if all Coord are within the range requested by algorithm. false otherwise   
     *
     */
     bool calcMatrixCoord(const MantidVec& X,size_t i,size_t j,std::vector<coord_t> &Coord)const=0;
  
 /** template generalizes the code to calculate all remaining coordinates, defined within the inner loop
    * given that the input described by sinble value only
     * @param X    -- X workspace value
     * 
     * @param Coord  -- subalgorithm specific number of coordinates, placed in the proper position of the Coordinate vector
     * @return true  -- if all Coord are within the range requested by algorithm. false otherwise   
     * */
    bool calcMatrixCoord(const double & X,std::vector<coord_t> &Coord)const=0;

    /** set up transformation and retrieve the pointer to the incorporating class, which runs the transformation and can provide
      * all necessary variables necessary for the conversion */
    void initialize(IConvertToMDEventsWS *)=0;
  
}; 

} // End MDAlgorighms namespace
} // End Mantid namespace

#endif