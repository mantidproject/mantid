#ifndef MDGEOMETRY_DESCRIPTION_H
#define MDGEOMETRY_DESCRIPTION_H

#include <deque>
#include "MantidGeometry/MDGeometry/MDGeometry.h"


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
  std::string Tag; //< unuque dimension identifier (tag)
  double trans_bott_left; //< shift in all directions (tans_elo is 4th element of transf_bott_left. Shift expressed in the physical units
  double cut_min; //< min limits to extract data;
  double cut_max; //< max limits to extract data;
  unsigned int nBins; //< number of bins in each direction, bins of size 1 are integrated (collased);
  bool isReciprocal; //< specifies if this dimension is reciprocal or not.
  std::string AxisName; //< new names for axis;
  DimensionDescription() :
    Tag(""), trans_bott_left(0), cut_min(-1), cut_max(1), nBins(1), isReciprocal(false), AxisName("")
  {}
};


class DLLExport MDGeometryDescription
{
public:
  //  MDGeometryDescription(std::vector<MDDataObjects::DimensionsID> &IDs);

    MDGeometryDescription(
      std::vector<MDDimension> dimensions, 
      const MDDimension& dimensionX, 
      const MDDimension& dimensionY, 
      const MDDimension& dimensionZ,
      const MDDimension& dimensiont);

    MDGeometryDescription(unsigned int numDims=4,unsigned int nReciprocalDims=3);
    MDGeometryDescription(const MDGeometry &origin);
    virtual ~MDGeometryDescription(void);
    unsigned int getNumDims(void)const{return nDimensions;}

	// returns the size of the image, described by this class
	size_t getImageSize()const;

   /// the function sets the rotation matrix which allows to transform vector inumber i into the basis;
   // TODO : it is currently a stub returning argument independant unit matrix; has to be written propely
   std::vector<double> setRotations(unsigned int i,const std::vector<double> basis[3]);

   void build_from_geometry(const MDGeometry &origin);

   std::string toXMLstring(void)const{return std::string("TEST PROPERTY");}
   bool fromXMLstring(const std::string &){
       return true;
   }
   std::vector<double> getRotations(void)const{return rotations;}

   std::vector<double> getCoord(unsigned int i)const;
   double shift(unsigned int i)const; 
   double cutMin(unsigned int i)const;
   double cutMax(unsigned int i)const;
   unsigned int numBins(unsigned int i)const;
   bool isAxisNamePresent(unsigned int i)const;
   std::string getAxisName(unsigned int i)const;
   std::string getTag(unsigned int i)const;  
 
   /// finds the location of the dimension, defined by the tag in the list of all dimensions;
   int getTagNum(const std::string &Tag, bool do_throw=false)const;
   ///returns the list of the axis tags sorted in the order requested for view 
   std::vector<std::string> getDimensionsTags(void)const;

   void renameTag(unsigned int num,const std::string &newID);

//*** SET -> t
   void setCoord(const std::string &Tag,const std::vector<double> &coord){
       int index = getTagNum(Tag,true);       this->setCoord(index,coord);
   }
   
   void setShift(const std::string &Tag,double Val){
       int index = getTagNum(Tag,true);       setShift(index,Val);
   }
   void setCutMin(const std::string &Tag,double Val){
      int index = getTagNum(Tag,true);      setCutMin(index,Val);
   }
   void setCutMax(const std::string &Tag,double Val){
     int index = getTagNum(Tag,true);     setCutMax(index,Val);
   }
   void setNumBins(const std::string &Tag,unsigned int Val){
      int index = getTagNum(Tag,true);      setNumBins(index,Val);
    }
   void setAxisName(const std::string &Tag,const std::string &Name){
      int index = getTagNum(Tag,true);      setAxisName(index,Name);
   }

   void setCoord(unsigned int i,const std::vector<double> &coord);

   void setShift(unsigned int i,double Val); 
   void setCutMin(unsigned int i,double Val);
   void setCutMax(unsigned int i,double Val);
   void setNumBins(unsigned int i,unsigned int Val);
   void setAxisName(unsigned int i,const std::string &Name);

/**  function sets the Tag requested into the position, defined by the index i;
 *
 * The requested tag is deleted from old location and inserted into the new location, leaving all other data as before. 
 * the tag has to be present in the array initially -> throws otherwise */
   void setPAxis(unsigned int i, const std::string &tag);
private:

    unsigned int nDimensions;               /**< real number of dimensions in the target dataset. 
                                                If source dataset has more dimensions than specified here, all other dimensions will be collapsed */
    unsigned int nReciprocalDimensions;    // number of reciprocal dimensions from the nDimensions

    std::vector<double> coordinates[3];     //< The coordinates of the target dataset in the WorkspaceGeometry system of coordinates (define the rotation matrix for qx,qy,qz coordinates;) 
    std::vector<double>     rotations;
 
    /** auxiliary function which check if the index requested is allowed. ErrinFuncName allows to add to the error message the name of the calling function*/
    void check_index(unsigned int i,const char *ErrinFuncName)const;
    /** the function which provides default constructor functionality */
    void intit_default_slicing(unsigned int nDims,unsigned int nRecDims);

 /// logger -> to provide logging, for MD workspaces
    static Kernel::Logger& g_log;

    std::deque<DimensionDescription> data;  //< data describes one dimension;

    //Helper method to generate slicing data.
    void createDimensionDescription(const MDDimension& dimension, const int i);

typedef std::deque<DimensionDescription>::iterator       it_data;
typedef std::deque<DimensionDescription>::const_iterator it_const_data;

};

} // Geometry
} // Mantid
#endif
