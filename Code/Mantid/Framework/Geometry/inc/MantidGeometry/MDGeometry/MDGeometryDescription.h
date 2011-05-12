#ifndef MDGEOMETRY_DESCRIPTION_H
#define MDGEOMETRY_DESCRIPTION_H

#include <deque>
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"
#include <boost/shared_ptr.hpp>

/** class describes slicing and rebinning matrix and the geometry of the MD workspace. 

 *  Unlike the geometry itself, it allows to describe  various changes to the geometry.
 *  Afer applying these changes to the MD image and MD geometry, an algorithm calculates various mutually
 *  dependent changes to MDImage e.g. new coherent MD image data and MD geometry
 *

    @author Alex Buts (ISIS, RAL)
    @date 05/10/2010

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
namespace Geometry
{
//
/// class describes data in one dimension;
//
class DimensionDescription
{
public:
  std::string Tag; //< unique dimension identifier (tag)

  /** shift in this direction (tans_elo is 4th element of transf_bott_left. Shift expressed in the physical units
     the data, which are binned into MD image and plotted along this dimension shifted by this value */
  double data_shift;

  double cut_min; //< min limits to extract data;
  double cut_max; //< max limits to extract data;
  double data_scale; //< Length of projection axes vectors in Ang^-1 or meV 
  size_t nBins; //< number of bins in each direction, bins of size 1 are integrated (collased);
  bool isReciprocal; //< specifies if this dimension is reciprocal or not.
  std::string AxisName; //< new names for axis;


  DimensionDescription() :
    Tag(""), data_shift(0), cut_min(-1), cut_max(1), data_scale(1), nBins(1), isReciprocal(false), AxisName("")
  {}
};

typedef boost::shared_ptr<IMDDimension> Dimension_sptr;
typedef std::vector<boost::shared_ptr<IMDDimension> > DimensionVec;
typedef std::vector<boost::shared_ptr<IMDDimension> >::iterator DimensionVecIterator;
typedef std::vector<double> RotationMatrix;

class DLLExport MDGeometryDescription
{
public:
  //  MDGeometryDescription(std::vector<MDDataObjects::DimensionsID> &IDs);

  MDGeometryDescription(
      DimensionVec dimensions, 
      Dimension_sptr dimensionX, 
      Dimension_sptr dimensionY,  
      Dimension_sptr dimensionZ, 
      Dimension_sptr dimensiont,
      RotationMatrix rotationMatrix
  );

  MDGeometryDescription(const MDGeometryBasis &basis);
  MDGeometryDescription(size_t numDims=4, size_t nReciprocalDims=3);
  MDGeometryDescription(const MDGeometry &origin);
  virtual ~MDGeometryDescription(void);
  /** sets the transformation matrix, () which would transform MDDPoints from MDDData system of coordinates
   *  into MDImage system of coordinates  As there are no info about 
      MDDPoints system of coodinates in this class, the matrix has to be calculated externaly (except when building from geomerty,
	  which provides example of retrieving this matrix	  */
  void setRotationMatrix(const MantidMat &rotMatrix){this->Rotations = rotMatrix;}
/// return the rotations, which transform old coordinate system into the new one, set by the projection axis;
  MantidMat const & getRotations()const;

  /// obtain number of dimensions in the geometry
  size_t getNumDims(void)const{return nDimensions;}

  /// obtain number of reciprocal dimensions in the geometry
  size_t getNumRecDims(void)const{return nReciprocalDimensions;}

  /// returns the size of the image, described by this class
  size_t getImageSize()const;

 
  void build_from_geometry(const MDGeometry &origin);

  std::string toXMLstring(void)const{return std::string("TEST PROPERTY");}
  bool fromXMLstring(const std::string &)
  {
    return true;
  }
  // Looks like unnesessary here -> we are not setting this for each dimension description any more;
 // Geometry::V3D getDirection(size_t i)const;

  bool isAxisNamePresent(size_t i)const;

  /** finds the location of the dimension, defined by the tag in the list of all dimensions;
   * informatiom about the axis to display is encoded by the tag numbers */
  int getTagNum(const std::string &Tag, bool do_throw=false)const;

  ///returns the list of the axis tags sorted in the order requested for view
  std::vector<std::string> getDimensionsTags(void)const;

  //   void renameTag(unsigned int num,const std::string &newID);
  // access to all
  DimensionDescription * pDimDescription(size_t i){
    check_index(i,"pDimDescription");
    return &data[i];
  }

  DimensionDescription * pDimDescription(size_t i)const{
    check_index(i,"pDimDescription");
    return const_cast<DimensionDescription *>(&data[i]);
  }

  // access to all
  DimensionDescription * pDimDescription(const std::string &Tag){
    int index = getTagNum(Tag,true);
    return &data[index];
  }

  DimensionDescription * pDimDescription(const std::string &Tag)const{
    int index = getTagNum(Tag,true);
    return const_cast<DimensionDescription *>(&data[index]);
  }



  /**  function sets the Tag requested into the position, defined by the index i;
   *
   * The requested tag is deleted from old location and inserted into the new location, leaving all other data as before.
   * the tag has to be present in the array initially -> throws otherwise */
  void setPAxis(size_t i, const std::string &tag);

private:

  /**< real number of dimensions in the target dataset.
  If source dataset has more dimensions than specified here, all other dimensions will be collapsed */
  size_t nDimensions;

  /// number of reciprocal dimensions from the nDimensions
  size_t nReciprocalDimensions;
  /** the matrix which would allow to thransform a vector, expressed in MDGeomBasis (MDDPoints) system of coordinates into 
      MDImage system of coordinates. */
   MantidMat  Rotations;

 
  /** auxiliary function which check if the index requested is allowed. ErrinFuncName allows to add to the error message the name of the calling function*/
  void check_index(size_t i,const char *ErrinFuncName)const;

  /** the function which provides default constructor functionality */
  void intit_default_slicing(size_t nDims, size_t nRecDims);

  /// logger -> to provide logging, for MD workspaces
  static Kernel::Logger& g_log;

  std::deque<DimensionDescription> data;  //< data describes one dimension;

  ///Helper method to generate slicing data.
  void createDimensionDescription(Dimension_sptr dimension, const int i);

  typedef std::deque<DimensionDescription>::iterator       it_data;
  typedef std::deque<DimensionDescription>::const_iterator it_const_data;

};

} // Geometry
} // Mantid
#endif
