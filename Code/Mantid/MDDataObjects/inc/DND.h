#ifndef H_DND
#define H_DND
#include "stdafx.h"
#include "file_format.h"
#include "Geometry.h"
#include "point3D.h"


/** the nucleus of the main class for visualisation and other operations; 
*   This is multidimensional Horace dataset without detailed pixel information
*/
class DND:public Geometry
{
public:
    // default constructor
     DND(unsigned int nDims=4);
    // destructor
    ~DND();
    /** function returns vector of points left after the selection has been applied to the multidimensinal dataset 
    * @param selection -- vector of indexes, which specify which dimensions are selected and the location of the selected point
    *                     e.g. selection[0]=10 -- selects the index 10 in the last expanded dimension or
    *                     selection.assign(2,10) for 4-D dataset lead to 2D image extracted from 4D image at range of points (:,:,10,10);
    *                     attempt to make selection outside of the range of the dimension range lead to the selection of last point in the dimension. 
    */
    std::vector<point3D> & getPointData(std::vector<unsigned int> &selection)const;
    /// the same as getPointData(std::vector<unsigned int> &selection) but select inial (0) coordinates for all dimensions > 3 
    std::vector<point3D> & getPointData(void)const;

    /// dangerous function, which clears internal memory allocated for the image points returned by the previous function; Should be used with care as it invalidares
    /// the points returned by the function getPointData, but using this function may be necessary for efficiency reasons.
    void clearPointsMemory(); 

    void read_dnd(const char *file_name){
        // select a file reader which corresponds to the proper file format of the data file
        this->select_file_reader(file_name);
        this->read_dnd();
    }
    void write_dnd(const char *file_name);
    bool read_dnd(void){if(this->theFile){
                           this->theFile->read_dnd(*this); return true;
                        }else{return false;}
    }
    bool write_dnd(void){if(this->theFile){
                          this->theFile->write_dnd(*this);return true;
                        }else{return false;}
    }

protected:
    size_t data_size;               ///< size of the data points array
    data_point *data;             ///< multidimensional array of data points, represented as a single dimensional array;
 
    // dimensions strides in linear order; formulated in this way for fast access 
    size_t nd2,nd3,nd4,nd5,nd6,nd7,nd8,nd9,nd10,nd11;       
    std::vector<size_t>dimStride;
    std::vector<unsigned long>dimSizes ;


   // interface to alloc_dnd_arrays below in case of full not collapsed dnd dataset
    void alloc_dnd_arrays(const SlicingData &transf);

/// clear all allocated memory as in the destructor; neded for reshaping the object for e.g. changing from defaults to something else. generally this is bad desighn. 
    void clear_class();

//*************************************************
// FILE OPERATIONS:
/// the name of the file with DND and SQW data;
    std::string fileName;
// the pointer to a class with describes correspondent dnd file format;
    file_format *theFile;
//  function selects the file reader given existing dnd or sqw file and sets up above pointer to the proper file reader;
//  throws if can not find the file, the file format is not supported or any other error;
    void select_file_reader(const char *file_name);
// class to read MATLAB written dnd_hdf
    friend class file_hdf_Matlab;
// cass to read binary HORACE dnd ***> not written yet;
    friend class dnd_bin_Matlab;
// class to read/write our HDF files ***> not written yet;
    friend class dnd_hdf;
//*************************************************
    // probably temporary
     DND & operator = (const DND & other);
    // copy constructor;
     DND(const DND& other);
//
  // location of cell in 1D data array shaped as 4 or less dimensional array;
     size_t nCell(int i)                    const{ return (i);}
     size_t nCell(int i,int j)              const{ return (i+j*nd2); }
     size_t nCell(int i,int j,int k)        const{ return (i+j*nd2+k*nd3); }
     size_t nCell(int i,int j,int k, int n) const{ return (i+j*nd2+k*nd3+n*nd4);}


     data_point thePoint(int i)                   const{   return data[nCell(i)];}
     data_point thePoint(int i,int j)             const{   return data[nCell(i,j)];}
     data_point thePoint(int i,int j,int k)       const{   return data[nCell(i,j,k)];}
     data_point thePoint(int i,int j,int k, int n)const{   return data[nCell(i,j,k,n)];}

private:
    /** function reshapes the geomerty of the array according to the pAxis array request; returns the total array size */
    size_t reshape_geometry(const SlicingData &transf);
   /// the pointer for vector returning the image points for visualisation
    std::vector<point3D> *image_points;
};

#endif;
