#include "MDDataObjects/IMD_FileFormat.h"
#include "MDDataObjects/MDWorkspace.h"

#include <vector>
namespace Mantid{
namespace MDDataObjects{

 Logger& IMD_FileFormat::f_log=Kernel::Logger::get("IMD_fileOperations");
//***************************************************************************************
bool
read_matlab_field_attr(hid_t group_ID,const std::string &field_name,void *&data, std::vector<int> &dims,int &rank,matlab_attrib_kind &kind,const std::string &file_name)
{
/*
  The function is the interface to the attributes written by MATLAB horace hdf

  MATLAB hdf crashes writing cell arrays or empty datasets, so tricks are implemented to encode such structures into hdf
  empty datasets are marked by special attribute, and cellarrays are written as an regular 2D array of max dimension with filler

  The function returns the array of the data written by Horace hdf and identifies its kind

% function reads the a dataset from hdf5 location  defined by group_ID
% Because of HDF5-matlab bug, the function has to be used to read data written by
% MATLAB functions write_attrib_list  and write_fields_list_attr  functions only
%
% Inputs:
% group_ID    -- hdf5 dataset or group location;
% field_name  -- list of the entries to read;
%
% Output:
% data        -- the allocatable array  to place the read data into; has to be deallocated in main program if not being used
% dims        -- vector-array of dataset dimensions, 1 for a scalar
% rank        -- number of dataset dimensions, 0 for scalar, 1 for 1D and 2 for 2D arrays
%                does not support more than 2D arrays at the moment;
% kind        -- parameter, which describes the kind of the data actually read by the function. The calling program
%                supposed to know what kind of data it asks for (to reinterpred the void at least),
%                and can use this parameter to verify if it really get what it asked for
%
%             Key-words are used to incode unsupported data formats
%             These key-words can not be present in the fiedls_names list:
% EMPTY_  and FILLER_

%
% V1: AB on 20/03/2010
%
%
% HORACE Revision: 438  (Date: 2010-04-27 13:23:01 +0100 (Tue, 27 Apr 2010) )
%
*/
bool filler_present(false);  // the array of variable length vectors or string is written but it is written as 2D array (MATLAB hdf bug)
int i;
char prim_kind;  // character or double
double filler_buf; // where to read the data filler if one exists
hsize_t data_size;


if(!H5Aexists(group_ID, field_name.c_str())){ // then may be the dataset is empty
     std::string new_field_name="EMPTY_"+field_name;
    // only attribute presence is identified, but attribute itself is empty (MATLAB hdf bug)
     if(H5Aexists(group_ID, new_field_name.c_str())){ // the attribute exists and it is empty
        rank=0;
        dims.clear();
        data = NULL;
        kind = empty;
        return true;
     }else{
        return false;
     }
}

// open
hid_t attr     = H5Aopen(group_ID, field_name.c_str(), H5P_DEFAULT);
if (attr<=0){
    std::stringstream err;
    err<<"read_matlab_field_attr: error opening existing attribute: "<<field_name<<std::endl;
    throw(Exception::FileError(err.str(),file_name));
}
// learn all about the dataset; MATLAB written datasets are simple arrays or scalars or characters
// as this is the attribute, the size of the array can be only limited;
 hid_t   type     = H5Aget_type(attr);
 if (type==H5T_NATIVE_UCHAR||type==H5T_NATIVE_CHAR||type==H5T_NATIVE_SCHAR){
      prim_kind='C';
 }else{
      prim_kind='D';
 }

 hsize_t *t_dims;
 hid_t  space     = H5Aget_space(attr);
 int    ndims     = H5Sget_simple_extent_ndims(space) ;
 if(ndims==0){  // scalar
     rank=1;
     t_dims           = new hsize_t[1];
     t_dims[0]        = 1;
     kind             = double_scalar;
 }else{
     t_dims           = new hsize_t[ndims];
     rank             = H5Sget_simple_extent_dims(space,t_dims,NULL) ;
     kind             = double_array;
     if (rank>2){
         throw(Exception::FileError("MATLAB HORACE reader does not currently understand arrays of more then 2 dimesions",file_name));
     }

 }
 data_size=1;
 for(i=0;i<ndims;i++){
     data_size*= t_dims[i];
 }

 //hid_t  type_name = H5Tget_class(type);


 if(rank==2||(prim_kind=='C'&&rank==1)){ //  % the array can be the representation of a cellarray (MATLAB hdf bug)
     // presence of filler attribute will tell if this is the case
     std::string filler_name="FILLER_"+field_name;

     if(H5Aexists(group_ID, filler_name.c_str())){
        filler_present=true;
        if(prim_kind=='C'){
              kind = char_cellarray;
        }else{
             kind = double_cellarray;
        }

        hid_t attr_f = H5Aopen(group_ID, filler_name.c_str(),H5P_DEFAULT);
        if (attr_f<=0){
              std::stringstream err;
              err<<"read_matlab_field_attr: error opening existing Filler attribute: "<<filler_name<<std::endl;
              throw(Exception::FileError(err.str(),file_name));
        }
        hid_t type_f = H5Aget_type(attr_f);
        herr_t err = H5Aread(attr_f, type_f,&filler_buf);
        if(err<0){
              std::stringstream err;
              err<<"read_matlab_field_attr: error reading existing Filler attribute: "<<filler_name<<std::endl;
              throw(Exception::FileError(err.str(),file_name));
        }
//
//        rez = transform_array2cells(rez,filler,type_name);
           H5Tclose(type_f);
           H5Aclose(attr_f);

    }

 }
 // expand output array by one to arrange make place for filler, placed there
 if (filler_present) data_size++;


 if (prim_kind=='C'){
    data = new char[(size_t)data_size];
    if(filler_present) *((char *)(data)+data_size-1)=*((char*)(&filler_buf));

 }else{
    data = new double[(size_t)data_size];
    if(filler_present) *((double *)(data)+data_size-1)=filler_buf;
 }
 herr_t err = H5Aread(attr, type,data);
 if (err<0){
     std::stringstream err;
     err<<"read_matlab_field_attr: error reading attribute: "<<field_name<<std::endl;
     throw(Exception::FileError(err.str(),file_name));
 }


 H5Sclose(space);
 H5Tclose(type);
 H5Aclose(attr);
 for(i=0;i<ndims;i++){
    dims.push_back(int(t_dims[i]));
 }
delete [] t_dims;

return true;
}

void ** 
transform_array2cells(void *data, std::vector<int> dims,int rank,matlab_attrib_kind kind,void *pFiller)
{
/* MATLAB Horace dataset compartibility function 
  it takes the data in the form of an array with filler and returns 
  array of strings or array of variable length vectors depending on kind of the data

function cellarray=transform_array2cells(rdata,filler,type_name)
% function constructs selarray from array rdata, written to the hdf file
% instead of variable length array to avoid hdf5-matlab bug;
*/
    int i,j;
    int nData(dims[0]);
    int length(dims[1]);
    switch(kind){
        case char_cellarray: {
            std::string ** rez=(new std::string *[nData]);
            char *arr = (char*)(data);
            char filler=*((char *)pFiller);
            char data;

            for(i=0;i<nData;i++){
                rez[i] = new std::string;
                for(j=0;j<length;j++){
                    data=arr[i*length+j];
                    if(data != filler){
                        rez[i]->push_back(data);
                    }else{
                        break;
                    }
                }
            }

            return (void **)rez;
        }
        case double_cellarray: {
            std::vector<double> **rez= new std::vector<double> *[nData];
            double *arr = (double*)(data);
            double filler=*((double *)pFiller);
            double data;

            for(i=0;i<nData;i++){
                rez[i] = new std::vector<double>;
                rez[i]->reserve(length);
                for(j=0;j<length;j++){
                    data=arr[i*length+j];
                    if(data != filler){
                        rez[i]->push_back(data);
                    }else{
                        break;
                    }
                }
            }
            return (void **)rez;

        }
        default: throw(std::invalid_argument("transform array2cells: unsupported datatype"));
    }
    return NULL;
}

} // namespace MDDataObjects
} // namespace Mantid