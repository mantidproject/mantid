#ifndef MD_IMAGE_DATA_H
#define MD_IMAGE_DATA_H
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
//#include "MantidAPI/IMDWorkspace.h"

#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/IMD_FileFormat.h"
#include "MDDataObjects/point3D.h"
#include <boost/shared_ptr.hpp>

//#include "c:/Mantid/Code/Mantid/API/inc/MantidAPI/IMDWorkspace.h"


/** the kernel of the main class for visualisation and analysis operations, which keeps the data itself and brief information about the data dimensions (its organisation in the 1D array)
*
*   This is equivalent of multidimensional Horace dataset without detailed pixel information (the largest part of dnd dataset)

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



using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

/// structure of the multidimension data array, which is the basis of MDData class and should be exposed to modyfying algorighms
struct MD_img_data{
    size_t data_size;               ///< size of the data points array expressed as 1D array;
    MD_image_point *data;           ///< multidimensional array of data points, represented as a single dimensional array;
    // integer descriptor for dimensions;
    std::vector<size_t>dimStride;
    std::vector<size_t>dimSize;     ///< number of bin in this dimension
    std::vector<double> min_value;  ///< min value in the selected dimension
    std::vector<double> max_value;  ///< max value in the selected dimension
    MD_img_data():data(NULL){}

};

//
class DLLExport MDImage
{
public:

    // default constructor
  MDImage(Mantid::Geometry::MDGeometry* p_geometry);
    // destructor
    ~MDImage();
    /** function returns vector of points left after the selection has been applied to the multidimensinal dataset
    * @param selection -- vector of indexes, which specify which dimensions are selected and the location of the selected point
    *                     e.g. selection[0]=10 -- selects the index 10 in the last expanded dimension or
    *                     selection.assign(2,10) for 4-D dataset lead to 2D image extracted from 4D image at range of points (:,:,10,10);
    *                     attempt to make selection outside of the range of the dimension range lead to the selection of last point in the dimension.
    */
    void getPointData(const std::vector<unsigned int> &selection,std::vector<point3D> & image_data)const;
    /// the same as getPointData(std::vector<unsigned int> &selection) but select inial (0) coordinates for all dimensions > 3
    void getPointData(std::vector<point3D> & image_data)const;
    /// returns the size of the Image array as 1D array;
    size_t getDataSize(void)const{return MDStruct.data_size;}
    /// returns dimension strides e.g. the changes of a position in 1D array when an M-th dimension index changes by 1;
    std::vector<size_t> getStrides(void)const;

    const MDGeometry * const getGeometry() const { return m_pgeometry.get(); }
 //******************************************************************************************************
//******************************************************************************************************
   virtual void initialize(const Geometry::MDGeometryDescription &Description){
        alloc_mdd_arrays(Description);
    }
    /// get acces to the internal image dataset for further modifications; throws if dataset is undefinded;
    MD_image_point      * get_pData(void);
    MD_image_point const* get_const_pData(void)const;
    /// get acces to the whole MD structure;
    MD_img_data            * get_pMDImgData(void){return (&MDStruct);}

    // interface to alloc_mdd_arrays below in case of full not collapsed mdd dataset
    void alloc_mdd_arrays(const MDGeometryDescription &transf);

/// function selects a reader, which is appropriate to the file described by the file_name and reads dnd data into memory
  //  void read_mdd(boost::shared_ptr<IMD_FileFormat> spFile,bool old4DMatlabReader=false);
    
    virtual long getMemorySize()const{return MDStruct.data_size*sizeof(MD_image_point);}
  /// build allocation table of sparce data points (pixels)
    void identify_SP_points_locations();
 protected:
 

    // dimensions strides in linear order; formulated in this way for faster access<- looks like obsolete;
    size_t nd2,nd3,nd4,nd5,nd6,nd7,nd8,nd9,nd10,nd11;



/// clear all allocated memory as in the destructor; neded for reshaping the object for e.g. changing from defaults to something else. generally this is bad desighn.
    void clear_class();

//*************************************************

   
//*************************************************
 //
 // location of cell in 1D data array shaped as 4 or less dimensional array;
     size_t nCell(int i)                    const{ return (i);}
     size_t nCell(int i,int j)              const{ return (i+j*nd2); }
     size_t nCell(int i,int j,int k)        const{ return (i+j*nd2+k*nd3); }
     size_t nCell(int i,int j,int k, int n) const{ return (i+j*nd2+k*nd3+n*nd4);}


     MD_image_point thePoint(int i)                   const{   return pData[nCell(i)];}
     MD_image_point thePoint(int i,int j)             const{   return pData[nCell(i,j)];}
     MD_image_point thePoint(int i,int j,int k)       const{   return pData[nCell(i,j,k)];}
     MD_image_point thePoint(int i,int j,int k, int n)const{   return pData[nCell(i,j,k,n)];}

      static Kernel::Logger& g_log;

    /** function reshapes the geomerty of the array according to the pAxis array request; returns the total array size */
    size_t reshape_geometry(const MDGeometryDescription &transf);
   /// the pointer for vector returning the image points for visualisation

private:
  //*************************************************

  std::auto_ptr<MDGeometry> m_pgeometry;
//
   MD_img_data  MDStruct;
   MD_image_point *pData; // the additional pointer to data structure in MDStruct;

  // probably temporary
  MDImage & operator = (const MDImage & other);
  // copy constructor;
  MDImage(const MDImage & other);

};
//
}
}
#endif;
