#ifndef H_MDDATA
#define H_MDDATA
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidAPI/IMDWorkspace.h"

#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/IMD_FileFormat.h"
#include "MDDataObjects/point3D.h"

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

//
class DLLExport MDData:public MDGeometry,public IMDWorkspace
{
public:
    // default constructor
     MDData(unsigned int nDims=4);
    // destructor
    ~MDData();
    /** function returns vector of points left after the selection has been applied to the multidimensinal dataset 
    * @param selection -- vector of indexes, which specify which dimensions are selected and the location of the selected point
    *                     e.g. selection[0]=10 -- selects the index 10 in the last expanded dimension or
    *                     selection.assign(2,10) for 4-D dataset lead to 2D image extracted from 4D image at range of points (:,:,10,10);
    *                     attempt to make selection outside of the range of the dimension range lead to the selection of last point in the dimension. 
    */
    void getPointData(const std::vector<unsigned int> &selection,std::vector<point3D> & image_data)const;
    /// the same as getPointData(std::vector<unsigned int> &selection) but select inial (0) coordinates for all dimensions > 3 
    void getPointData(std::vector<point3D> & image_data)const;
 //****************************************************************************************************** 
// IMD workspace interface functions
  /// return ID specifying the workspace kind
    virtual const std::string id() const { return "MD-Workspace"; }
  
    /// 
  virtual unsigned int getNumDims(void) const{return Geometry::MDGeometry::getNumDims();}
//****************************************************************************************************** 
   bool read_mdd(void){if(this->theFile){
                           this->theFile->read_mdd(*this); return true;
                        }else{return false;}
    }
    /// function writes the MDD data using current file reader; if the file is not opened, a default file reader is used. 
    void write_mdd(const char *file_name);
    /// function writes back MDD data to the existing dataset attached to the class;  Should throw if the size of the data changed (and this should not happen)
    bool write_mdd(void){if(this->theFile){
                          this->theFile->write_mdd(*this);return true;
                          }else{return false;}
    }
    virtual void initialize(const Geometry::MDGeometryDescription &Description){
        alloc_mdd_arrays(Description);
    }
    /// get acces to the internal image dataset for further modifications; throws if dataset is undefinded;
    MD_image_point * get_pData(void);
    MD_image_point const* get_const_pData(void)const;
protected:
    size_t data_size;               ///< size of the data points array
    MD_image_point *data;           ///< multidimensional array of data points, represented as a single dimensional array;

    virtual long getMemorySize()const{return data_size*sizeof(MD_image_point);}
 
    // dimensions strides in linear order; formulated in this way for fast access 
    size_t nd2,nd3,nd4,nd5,nd6,nd7,nd8,nd9,nd10,nd11;       
    std::vector<size_t>dimStride;
    std::vector<unsigned long>dimSizes ;


   // interface to alloc_mdd_arrays below in case of full not collapsed mdd dataset
    void alloc_mdd_arrays(const MDGeometryDescription &transf);

/// clear all allocated memory as in the destructor; neded for reshaping the object for e.g. changing from defaults to something else. generally this is bad desighn. 
    void clear_class();

//*************************************************
// FILE OPERATIONS:
/// the name of the file with DND and SQW data;
    std::string fileName;
// the pointer to a class with describes correspondent mdd file format;
    IMD_FileFormat *theFile;
//  function selects the file reader given existing mdd or sqw file and sets up above pointer to the proper file reader;
//  throws if can not find the file, the file format is not supported or any other error;
    void select_file_reader(const char *file_name);
// class to read MATLAB written mdd_hdf
    friend class MD_File_hdfMatlab;
// cass to read binary HORACE mdd ***> not written yet;
    friend class MD_File_binMatlab;
// class to read/write our HDF files ***> not written yet;
    friend class MD_File_hdfV1;

   /// function selects a reader, which is appropriate to the file described by the file_name and reads dnd data into memory
    void read_mdd(const char *file_name){
        // select a file reader which corresponds to the proper file format of the data file
        this->select_file_reader(file_name);
        // read actual data
        this->read_mdd();
        // idetyfy pixels locations 
        this->identify_SP_points_locations();
    }
    /// build allocation table of sparce data points
    void identify_SP_points_locations();
//*************************************************
 //
 // location of cell in 1D data array shaped as 4 or less dimensional array;
     size_t nCell(int i)                    const{ return (i);}
     size_t nCell(int i,int j)              const{ return (i+j*nd2); }
     size_t nCell(int i,int j,int k)        const{ return (i+j*nd2+k*nd3); }
     size_t nCell(int i,int j,int k, int n) const{ return (i+j*nd2+k*nd3+n*nd4);}


     MD_image_point thePoint(int i)                   const{   return data[nCell(i)];}
     MD_image_point thePoint(int i,int j)             const{   return data[nCell(i,j)];}
     MD_image_point thePoint(int i,int j,int k)       const{   return data[nCell(i,j,k)];}
     MD_image_point thePoint(int i,int j,int k, int n)const{   return data[nCell(i,j,k,n)];}

      static Kernel::Logger& g_log;

    /** function reshapes the geomerty of the array according to the pAxis array request; returns the total array size */
    size_t reshape_geometry(const MDGeometryDescription &transf);
   /// the pointer for vector returning the image points for visualisation
  
private:
 //*************************************************
   // probably temporary
     MDData & operator = (const MDData & other);
    // copy constructor;
     MDData(const MDData & other);


};
// 
}
}
#endif;
