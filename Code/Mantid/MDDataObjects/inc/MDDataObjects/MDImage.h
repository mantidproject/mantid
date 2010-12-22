#ifndef MD_IMAGE_H
#define MD_IMAGE_H
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"

#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/IMD_FileFormat.h"
#include "MDDataObjects/MDImageDatatypes.h"
#include <boost/shared_ptr.hpp>



/**
 * The kernel of the main class for visualisation and analysis operations,
 * which keeps the data itself and brief information about the data dimensions
 * (its organisation in the 1D array)
 *
 * This is equivalent of multidimensional Horace dataset without detailed pixel
 * information (the largest part of dnd dataset).
 *
 *
 * Note, Janik Zikovsky: It is my understanding that:
 *
 * MDImage is a dense representation of a specific slice of a larger data set,
 * generated from a MDWorkspace by (some kind of) rebinning algorithm.
 *

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
*/
namespace Mantid{
    namespace MDDataObjects{
//
class DLLExport MDImage
{
public:
    /// default constructor
    MDImage(Mantid::Geometry::MDGeometry* p_MDGeometry=NULL);
	/// the constructor which builds empty image from geometry the description (calling proper geometry constructor inside)
    MDImage(const Geometry::MDGeometryDescription &Description, const Geometry::MDGeometryBasis & pBasis);
    // destructor
    ~MDImage();
    /** function returns vector of points left after the selection has been applied to the multidimensinal image
	*
    * @param selection -- vector of indexes, which specify which dimensions are selected and the location of the selected point
    *                     e.g. selection[0]=10 -- selects the index 10 in the last expanded dimension or
    *                     selection.assign(2,10) for 4-D dataset lead to 2D image extracted from 4D image at range of points (:,:,10,10);
    *                     throws if attempeted to select more dimensions then the number of expanded dimensions
	*                     if the selection index extends beyond of range for an dimension, function returns last point in this dimension.
    */
    void getPointData(const std::vector<unsigned int> &selection,std::vector<point3D> & image_data)const;
    /// the same as getPointData(std::vector<unsigned int> &selection) but select inial (0) coordinates for all dimensions > 3
    void getPointData(std::vector<point3D> & image_data)const;
    /// returns the size of the Image array as 1D array;
    size_t getDataSize(void)const{return MD_IMG_array.data_size;}
	/// returns the size occupied by the data part of the MD_IMG_array;
    virtual long getMemorySize()const{return MD_IMG_array.data_array_size*sizeof(MD_image_point);}
   ///
    Geometry::MDGeometry * const getGeometry() const { return pMDGeometry.get(); }
 //******************************************************************************************************
//******************************************************************************************************
    /** initialises image, build with empty constructor, to working state. 
	 *  If called on existing image, ignores basis and tries to reshape image as stated in description	*/
	virtual void initialize(const Geometry::MDGeometryDescription &Description, const Geometry::MDGeometryBasis *const pBasis=NULL);
	bool is_initialized(void)const;
    /// get acces to the internal image dataset for further modifications; throws if dataset is undefinded;
    MD_image_point      * get_pData(void);
	MD_image_point const* get_const_pData(void)const;
    /// get acces to the whole MDga structure;
    MD_img_data         * get_pMDImgData(void){return (&MD_IMG_array);}

   //Temporary fix to get 3-D image data.
   MD_image_point getPoint(int i,int j,int k)       const
   {
     return this->MD_IMG_array.data[nCell(i, j, k)];
   }

  /// build allocation table of sparce data points (pixels) -> will be moved into points;
    void identify_SP_points_locations();

 protected:
    // dimensions strides in linear order; formulated in this way for faster access<- looks like obsolete;
    size_t nd2,nd3,nd4,nd5,nd6,nd7,nd8,nd9,nd10,nd11;

//*************************************************
//*************************************************
 //
 // location of cell in 1D data array shaped as 4 or less dimensional array;
     size_t nCell(int i)                    const{ return (i);}
     size_t nCell(int i,int j)              const{ return (i+j*nd2); }
     size_t nCell(int i,int j,int k)        const{ return (i+j*nd2+k*nd3); }
     size_t nCell(int i,int j,int k, int n) const{ return (i+j*nd2+k*nd3+n*nd4);}

	 // not used at the moment; if will, should be uncommented and pData pointer defined properly
	 //MD_image_point thePoint(int i)                   const{   return MD_IMG_array.data[nCell(i)];}
  //   MD_image_point thePoint(int i,int j)             const{   return pData[nCell(i,j)];}
  //
  //   MD_image_point thePoint(int i,int j,int k, int n)const{   return pData[nCell(i,j,k,n)];}

      static Kernel::Logger& g_log;
   /// the pointer for vector returning the image points for visualisation

private:
  //*************************************************

  std::auto_ptr<Geometry::MDGeometry> pMDGeometry;
//
   MD_img_data    MD_IMG_array;
 

  // probably temporary
  MDImage & operator = (const MDImage & other);
  // copy constructor;
  MDImage(const MDImage & other);

    /** function reshapes the geomerty of the array according to the pAxis array request; resets the data size in total MD_IMG_array  */
    void reshape_geometry(const Geometry::MDGeometryDescription &transf);

/// Clear all allocated memory as in the destructor; user usually do not need to call this function unless wants to clear image manually and then initiat it again
///	Otherwise it used internaly for reshaping the object for e.g. changing from defaults to something else. 
    void clear_class();
	/// function allocates memory for the MD image and resets all necessary auxilary settings;
	void alloc_image_data(size_t Imgsize,unsigned int nDims);
};
//
}
}
#endif
