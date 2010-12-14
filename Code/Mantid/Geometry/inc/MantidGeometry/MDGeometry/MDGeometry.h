#ifndef H_MD_GEOMETRY
#define H_MD_GEOMETRY
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <boost/shared_ptr.hpp>
#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"
//#include "MantidGeometry/MDGeometry/MDDimension.h"
#include "MantidGeometry/MDGeometry/MDDimensionRes.h"
//#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"


/** The class describes the geometry of the N-D visualisation workspace
 * and provides interface and convenient container to sizes and shapes of DND object
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

    class DLLExport MDGeometry
    {
    public:
      MDGeometry(const MDGeometryBasis &basis);
      MDGeometry(const MDGeometryBasis &basis, const MDGeometryDescription &description);

      ~MDGeometry(void);

      // the functions return the particular dimensions; Throws if correspondent dimension does not exist (e.g. less th 
      boost::shared_ptr<IMDDimension> getXDimension(void)const{return (theDimension[0]);}
      boost::shared_ptr<IMDDimension> getYDimension(void)const;
      boost::shared_ptr<IMDDimension> getZDimension(void)const;
      boost::shared_ptr<IMDDimension> getTDimension(void)const;
      std::vector<boost::shared_ptr<IMDDimension> > getIntegratedDimensions(void)const;
      /// obtains pointers to all dimensions defined in the geometry
      std::vector<boost::shared_ptr<IMDDimension> > getDimensions(void)const;

      /// returns the number of expanded (non-integrated) dimensions
      unsigned int getNExpandedDims(void)const{return n_expanded_dim;}

	  /// function returns the number of cells, which an Image with this geometry would have;
	  size_t getGeometryExtend()const{return nGeometrySize;}

 
      /// function returns an axis vector of the dimension, specified by ID; it is 1 for orthogonal dimensions and triplet for the reciprocal 
      /// (but in a form of <1,0,0> if reciprocals are orthogonal to each other;
      std::vector<double> getOrt(const std::string &tag)const;

      /// return the numbers of dimensions in current geometry; 
      unsigned int getNumDims(void)const{return m_basis.getNumDims();}
      /// returns the number of reciprocal dimensions
      unsigned int getNumReciprocalDims(void)const{return m_basis.getNumReciprocalDims();};
      ///
      std::vector<std::string> getBasisTags(void)const;
      ///
      unsigned int getNumExpandedDims(void)const{return n_expanded_dim;}

     /// function returns the pointer to the dimension requested as the dimension num. Throws if dimension is out of range. Convenient for looping though dimensions instead of
      /// asking for DimX, Y and Z;
      boost::shared_ptr<const IMDDimension>  get_constDimension(unsigned int i)const;
      /// functions return the pointer to the dimension requested by the dimension tag. throws if such dimension is not present in the Geometry (or NULL if not throwing);
      boost::shared_ptr<const IMDDimension>  get_constDimension(const std::string &tag,bool do_throw=true)const;


      /** function resets MDGeometryBasis and MDGeometry to new state;
      *   
	  *  modified substantially from initial idea
	  *  throws if any dimension ID in geometry descrition (trf) lies outside of the id-s currently present in the geometry;
      */
      void reinit_Geometry(const MDGeometryDescription &trf);
    protected: 
     /// functions return the pointer to the dimension requested as the dimension num. Throws if dimension is out of range. Convenient for looping though dimensions instead of
      /// asking for DimX, Y and Z;
      boost::shared_ptr<MDDimension>  getDimension(unsigned int i);
      /// functions return the pointer to the dimension requested by the dimension tag. throws if such dimension is not present in the Geometry (or NULL if not throwing);
      boost::shared_ptr<MDDimension>  getDimension(const std::string &tag,bool do_throw=true);

      /// the parameter describes the dimensions, which are not integrated. These dimensions are always at the beginning of the dimensions vector. 
      unsigned int n_expanded_dim;
      /// the array of Dimensions. Some are collapsed (integrated over)
      std::vector<boost::shared_ptr<MDDimension> >  theDimension;


      /* function returns tne location of the dimension specified by the tag, in the array theDimension (in the MDGeomerty)
      negative value specifies that the requested dimension is not present in the array. */
      //  int getDimNum(const std::string &tag,bool do_trow=false)const;

    private:
      /** function sets ranges of the data as in transformation request; Useless without real change of the ranges */
      void setRanges(const MDGeometryDescription &trf);
	  /// the number of data cells, which such geometry would occupy
	  size_t nGeometrySize;

      MDGeometryBasis m_basis;
      //void init_empty_dimensions();
      /// the map used for fast search of a dumension from its tag. 
      std::map<std::string,boost::shared_ptr<MDDimension> > dimensions_map;
      //Defaults should do: ->NO?
      MDGeometry& operator=(const MDGeometry&);   
      /// logger -> to provide logging, for MD workspaces
      static Kernel::Logger& g_log;

      /// currently similar to arrangeDimensionsProperly as reinit is currently disabled but checks if tags are valid;
      void reinit_Geometry(const std::vector<std::string> &DimensionTags);
      /** function used to arrange dimensions properly, e.g. according to the order of the dimension tags supplied as input argument
      and moving all non-collapsped dimensions first. Throws if an input tag is not among the tags, defined in the geometry */
      void arrangeDimensionsProperly(const std::vector<std::string> &tags);

	  void init_empty_dimensions();
    };
  }  // Geometry
}  // Mantid
#endif
