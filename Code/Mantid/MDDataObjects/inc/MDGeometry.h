#ifndef H_MD_GEOMETRY
#define H_MD_GEOMETRY
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "WorkspaceGeometry.h"
#include "Dimension.h"
#include "DimensionRes.h"
/** The class describes the geometry of the N-D visualisation workspace and provides interface and convenient container to sizes and shapes of DND object
*
*   It is specific workspace geometry, which is used for visualisation and analysis. 
*   It describes current size and shape of the data and its dimensions, including the dimensions which are integrated. 
*   It changes as the result of operations as user tries to look at the reciprocal space from different points of view and selects for 
*   analysis different dimensions and different parts of the space.

    @author Alex Buts, RAL ISIS
    @date 28/09/2010

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

**/
namespace Mantid{
    namespace MDDataObjects{

// predefine helper class which keeps all information about the requested transformation 
    class SlicingData;

class DLLExport MDGeometry :   public WorkspaceGeometry
{
public:
    // the functions return the particular dimensions 
    Dimension & getXDimension(void)const{return *(theDimension[0]);}
    Dimension & getYDimension(void)const;
    Dimension & getZDimension(void)const;
    Dimension & getTDimension(void)const;
    std::vector<Dimension *> getIntegratedDimensions(void);

    /// functions return the pointer to the dimension requested as the dimension num. Throws if dimension is out of range. 
    Dimension * getDimension(unsigned int i)const;
    /// functions return the pointer to the dimension requested as the dimension ID. Returns NULL if no such dimension present in the Geometry or throws is ID is not casted properly;
    Dimension * getDimension(DimensionsID ID)const;

    /// function returns an axis vector of the dimension, specified by ID; it is 1 for orthogonal dimensions and triplet for the reciprocal 
    /// (but in a form of <1,0,0> if reciprocals are orthogonal to each other;
    std::vector<double> getOrt(DimensionsID id)const;
 
    ~MDGeometry(void);
protected: 

    MDGeometry(unsigned int nDimensions=4);
    /// the parameter describes the dimensions, which are not integrated. These dimensions are always at the beginning of the dimensions vector. 
    unsigned int n_expanded_dim;
    /// the array of Dimensions. Some are collapsed (integrated over)
     std::vector<Dimension *>  theDimension;
     /// array specifying the location of the dimension as the function of the specified ID
     std::vector<int>          theDimensionIDNum;

     /** function sets ranges of the data as in transformation request; Useless without real change of the ranges */
     void setRanges(const SlicingData &trf);

   /** function used to reset dimensions according to the requested transformaton and to arrange them properly, e.g. according to the order of the 
       dimensions in IDs   plus all non-collapsped dimensions first */
     void arrangeDimensionsProperly(const std::vector<DimensionsID> &IDS);
   /* function returns tne location of the dimension specified by the ID, in the array theDimension (in the Geomerty)
       negative value specifies that the requested dimension is not present in the array. */
    int getDimNum(DimensionsID ID)const;
    /** function resets WorkspaceGeometry and MDGeometry as new 
    *   if any ID in the list is different from existing or just resets the structure into new ID shape if new ID-s list includes all from the old one;
    *   when the structure is indeed 
    */
    void reinit_Geometry(const SlicingData &trf);
private:
    void init_empty_dimensions(const std::vector<DimensionsID> &IDS);

    // currently undefined
    MDGeometry& operator=(const MDGeometry&);      
};
}  // MDDataObjects
}  // Mantid
#endif
