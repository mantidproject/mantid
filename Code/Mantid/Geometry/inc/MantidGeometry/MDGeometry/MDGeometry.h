#ifndef H_MD_GEOMETRY
#define H_MD_GEOMETRY
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"
//#include "MantidGeometry/MDGeometry/MDDimension.h"
#include "MantidGeometry/MDGeometry/MDDimensionRes.h"
//#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"

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
    namespace Geometry{
   
   class  MDGeometryDescription;

class DLLExport MDGeometry :   public MDGeometryBasis
{
public:
    // the functions return the particular dimensions; Throws if correspondent dimension does not exist (e.g. less th 
    MDDimension & getXDimension(void)const{return *(theDimension[0]);}
    MDDimension & getYDimension(void)const;
    MDDimension & getZDimension(void)const;
    MDDimension & getTDimension(void)const;
    std::vector<MDDimension *> getIntegratedDimensions(void);

    /// functions return the pointer to the dimension requested as the dimension num. Throws if dimension is out of range. Convenient for looping though dimensions instead of
    /// asking for DimX, Y and Z;
    MDDimension * getDimension(unsigned int i)const;
    /// functions return the pointer to the dimension requested by the dimension tag. throws if no such dimension is present in the Geometry ;
    MDDimension * getDimension(const std::string &tag)const;

    /// function returns an axis vector of the dimension, specified by ID; it is 1 for orthogonal dimensions and triplet for the reciprocal 
    /// (but in a form of <1,0,0> if reciprocals are orthogonal to each other;
    std::vector<double> getOrt(const std::string &tag)const;
 
    ~MDGeometry(void);

    MDGeometry(unsigned int nDimensions=4,unsigned int nReciprocalDimensions=3);
    //Defaults should do:?
   // MDGeometry& operator=(const MDGeometry&);    
 protected: 
   /// the parameter describes the dimensions, which are not integrated. These dimensions are always at the beginning of the dimensions vector. 
    unsigned int n_expanded_dim;
    /// the array of Dimensions. Some are collapsed (integrated over)
     std::vector<MDDimension *>  theDimension;
 
     /** function sets ranges of the data as in transformation request; Useless without real change of the ranges */
     void setRanges(const MDGeometryDescription &trf);

   /** function used to arrange dimensions properly, e.g. according to the order of the dimension tags supplied as input argument
       and moving all non-collapsped dimensions first. Throws if an input tag is not among the tags, defined in the geometry */
     void arrangeDimensionsProperly(const std::vector<std::string> &tags);
   /* function returns tne location of the dimension specified by the tag, in the array theDimension (in the MDGeomerty)
       negative value specifies that the requested dimension is not present in the array. */
  //  int getDimNum(const std::string &tag,bool do_trow=false)const;
    /** function resets MDGeometryBasis and MDGeometry to new state;
    *   if any ID in the list is different from existing or just resets the structure into new ID shape if new ID-s list includes all from the old one;
    *   when the structure is indeed 
    */
    void reinit_Geometry(const MDGeometryDescription &trf,unsigned int nReciprocalDims=3);
    void reinit_Geometry(const std::vector<std::string> &DimensionTags,unsigned int nReciprocalDims=3);
private:
    void init_empty_dimensions(const std::vector<DimensionID> &tags);

    int  getDimNum(const std::string &tag,bool do_throw=true)const;
    /// the map used for fast search of a dumension from its tag. 
    std::map<size_t,int> dimensions_map;
};
}  // Geometry
}  // Mantid
#endif
