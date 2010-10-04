#ifndef H_ALL_GEOMETRY
#define H_ALL_GEOMETRY

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "stdafx.h"
#include "MDWorkspaceConstants.h"

/** The class is the part of the VisualisationWorkspace and describes the basic multidimentional geometry of the object, 
*   e.g. the dimensions of the reciprocal space and other possible dimenions  
*   the reference reciprocal lattice, the size and shape of a primary crystall cell
*   and number of additional ortogonal dimensions, e.g. temperature, pressure etc. 
*
*   Class provides the reference framework for visualisation and analysis operations alowing to compare different workspaces and to modify 
    the visualisation geometry as requested by user.

    @author Alex Buts, RAL ISIS
    @date 27/09/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
	  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

namespace Mantid
{
    namespace MDDataObjects
    {
        /// The constand defines the maximal number of dimensions, which N-D workspace could ever have. 



    class DLLExport WorkspaceGeometry
    {

     public:
    /*! class constructor:. 
    *  @param nDimensions -- maximal number of the dimensions all datasets will have
    *                        Default is 4 dimensions for crystall e.g. 3 wave-vectors (coordinates in reciprocal space) and energy
    *                        or 2 for powder (angle and energy or n_detector and energy or |Q| and energy)
    */
     WorkspaceGeometry(unsigned int nDimensions=4);  

    ~WorkspaceGeometry(void);


     /// function rerutns the reference coordinate of the dimension, ID requested;
    const std::vector<double> & getOrt(DimensionsID id)const;

    /** return the number of the space dimension which corresponds to the ID provided
    *   in a future there will be group of this kind of functions used to construct a transformation matrix from the set of 
    *   dim ID-s 
    *   @param ID       -- the dimention ID
    *   @param nothrow  -- function throws if the dimension ID falls outside of the range defined for this geometry
    *                      if nothrow==true, function returns negative value instead
    */  
    int getDimRefNum(DimensionsID ID, bool nothrow=false)const;

    /// 
    double getScale(unsigned int nDim);
    /// copy constructor
    WorkspaceGeometry(const WorkspaceGeometry&);                 
   /// return the numbers of dimensions in current geometry; 
    unsigned int getNumDims(void)const{return n_total_dim;}

protected: 

    /// number of total dimensions in dataset;
    unsigned int n_total_dim;
    /// number of reciprocal dimensions (which are non-orthogonal to each other n_rsprcl_dim<=n_total_dim)
    unsigned int n_rsprcl_dim;
    /// vector of dimensions id-s, specific for current architecture, the size of dimensions is n_total_dimensions,
    std::vector<DimensionsID> DimensionIDs;

      /// function returns the id of the dimension No requseted
    DimensionsID getDimensionID(unsigned int nDim)const;

       // this are the vectors of the primitive cell of the reciprocal lattice
    // expressed in the framework of ???; All other vectors are orthogonal to this triplet
    std::vector<double> lattice_ort[3];
    std::vector<double> unit;

    // init class with new number of dimensions and new dimensions types regardless of previous initialisation (which will be lost)
    void reinit_WorkspaceGeometry(const std::vector<DimensionsID> &ID);
private:
    // build default geometry based on cubic lattice. 
    void buildCubicGeometry(void);

    /// it is unclear what is the meaning of =
    WorkspaceGeometry& operator=(const WorkspaceGeometry&);      
    };
    };
};
#endif
