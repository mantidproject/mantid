#include "stdafx.h"
#include "MD_File_hdfMatlab.h"
namespace Mantid{
    namespace MDDataObjects{
//        using namespace Mantid::Kernel;


/// enum to identify fields, used in MATALB Horace DND-hdf file
enum matlab_mdd_fields_list{
    DatasetName,
    DataDescriptor,
    Pixels,
    N_DND_FIELDS
};
/// enum to identify attributes, used in MATLAB horace DND-hdf file;
enum matlab_mdd_attributes_list{
    nDND_dims,
    range,
    axis,
    N_MATLAB_FIELD_ATTRIBUTES
};
// MATLAB datatypes which may be written into hdf HORACE file
enum matlab_attrib_kind
{
    double_scalar,
    double_array,
    char_array, // usually string e.g. 1D array of characters
    empty,
    char_cellarray,
    double_cellarray
};

//***************************************************************************************

MD_File_hdfMatlab::MD_File_hdfMatlab(const char *file_name):
file_access_mode(H5P_DEFAULT),
pixel_dataset_h(-1),
file_handler(-1),
pixel_dataspace_h(-1)
{/// check if the file exists and is an hdf5 file;
    this->File_name.assign(file_name);


    htri_t rez=H5Fis_hdf5(File_name.c_str());
    if (rez<=0){
        if (rez==0){
            throw("the file is not an hdf5 file");
        }else{
            throw(" error processing existing hdf file");
        }
    }
    if(!H5Zfilter_avail(H5Z_FILTER_DEFLATE)){
        throw("can not obtain deflate filter to read MATLAB-hdf datatypes");
    }
/// actually opens the file
    this->file_handler = H5Fopen(File_name.c_str(),H5F_ACC_RDONLY,file_access_mode);
    if (file_handler<0){
        throw(" error opening existing hdf5 file");
    }
/// mdd dataset names in MATLAB HDF file;
    this->mdd_field_names.resize(N_DND_FIELDS);
    this->mdd_field_names[DatasetName].assign("Signals");        // mdd dataset name;
    this->mdd_field_names[DataDescriptor].assign("spe_header");  // dataset descriptors: contains information about various parts of the dataset
    this->mdd_field_names[Pixels].assign("pix");                 // pixels dataset name

/// mdd dataset arrtibutes in MATLAB HDF file  
    this->mdd_attrib_names.resize(N_MATLAB_FIELD_ATTRIBUTES);
    this->mdd_attrib_names[nDND_dims].assign("signal_dims");
    this->mdd_attrib_names[range].assign("urange");
    this->mdd_attrib_names[axis].assign("p");
 
}
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
//***************************************************************************************
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
//***************************************************************************************
bool 
MD_File_hdfMatlab::check_or_open_pix_dataset(void)
{
    bool was_opened(false); 
    if(this->pixel_dataset_h<0){
        this->pixel_dataset_h    = H5Dopen(this->file_handler,this->mdd_field_names[Pixels].c_str(),this->file_access_mode);
        if(this->pixel_dataset_h<0){
            throw(Exception::FileError("MD_File_hdfMatlab::check_or_open_pix_dataset: Can not open pixels dataset",this->File_name));
        }
    }else{
        was_opened        = true;
    }

    return was_opened;
}
void 
MD_File_hdfMatlab::read_mdd(MDData & dnd)
{
// The function accepts full 4D dataset only!!!
    hid_t h_signal_DSID=H5Dopen(file_handler,this->mdd_field_names[DatasetName].c_str(),H5P_DEFAULT);
    if (h_signal_DSID<0){
        throw(Exception::FileError("MD_File_hdfMatlab::check_or_open_pix_dataset: Can not open the hdf mdd dataset",this->File_name));
    }
    std::vector<int> arr_dims_vector;
    int    rank;
    unsigned int nDims,i;
    void  *data;
    herr_t err;
    matlab_attrib_kind kind;
    bool ok;


    // find and read the dimensions of the mdd dataset
    ok=read_matlab_field_attr(h_signal_DSID,this->mdd_attrib_names[nDND_dims].c_str(),data,arr_dims_vector,rank,kind,this->File_name);
    if(!ok){
        std::stringstream err;
        err<<"MD_File_hdfMatlab::check_or_open_pix_dataset: Error reading signal dimensions attribute: "<<mdd_attrib_names[nDND_dims]<<std::endl;
        throw(Exception::FileError(err.str(),this->File_name));
    }
    nDims= arr_dims_vector[0];
    SlicingData dnd_shape(nDims);

    for(i=0;i<nDims;i++){
        unsigned int dim_size=(unsigned int)*((double*)(data)+i);
        dnd_shape.setNumBins(i,dim_size);
    }
    delete [] data;
    arr_dims_vector.clear();


    // read other dataset descriptors
    hid_t descriptors_DSID=H5Gopen(file_handler,this->mdd_field_names[DataDescriptor].c_str(),H5P_DEFAULT);
    if (descriptors_DSID<0){
        std::stringstream err;
        err<<"MD_File_hdfMatlab::check_or_open_pix_dataset: Can not open the the data descriptors field in the dataset: "<<mdd_attrib_names[DataDescriptor]<<std::endl;
        throw(Exception::FileError(err.str(),this->File_name));
    }
    // read data limits
    ok=read_matlab_field_attr(descriptors_DSID,this->mdd_attrib_names[range],data,arr_dims_vector,rank,kind,this->File_name);
    if(!ok){
        std::stringstream err;
        err<<"MD_File_hdfMatlab::check_or_open_pix_dataset: Error reading mdd data range attribute: "<<mdd_attrib_names[range]<<std::endl;
        throw(Exception::FileError(err.str(),this->File_name));
    }
    for(i=0;i<nDims;i++){
        dnd_shape.setCutMin(i,*((double*)(data)+2*i));
        dnd_shape.setCutMax(i,*((double*)(data)+2*i+1));
    }
    delete [] data;
    arr_dims_vector.clear();

    // read axis
    ok=read_matlab_field_attr(descriptors_DSID,this->mdd_attrib_names[axis],data,arr_dims_vector,rank,kind,this->File_name);
    if(!ok){
       std::stringstream err;
        err<<"MD_File_hdfMatlab::check_or_open_pix_dataset: Error reading mdd data axis attribute: "<<mdd_attrib_names[axis]<<std::endl;
        throw(Exception::FileError(err.str(),this->File_name));
    }
    if (kind!=double_cellarray){
        throw(Exception::FileError("wrong type identifyed reading data axis",this->File_name));
    }
    // transform 2D array of axis into N-D vector of axis vectors;
    int nData=arr_dims_vector[0]*arr_dims_vector[1];
    double filler = *((double *)(data)+nData);
    std::vector<double> **rez    =(std::vector<double> **)transform_array2cells(data,arr_dims_vector,rank,kind,&filler);
    if(MAX_NDIMS_POSSIBLE<=arr_dims_vector[0]){
        throw(Exception::FileError("file_hdf_Matlab::read_mdd=>algorithm error: number of the data axis in mdd structure residing in file has to be less then MAX_NDIMS_POSSIBLE",this->File_name));
    }
    /* This is absolutely unnesessary for linear axis as n-bins has been already defined;
        for(i=0;i<nDims;i++){
          unsigned int dim_lentgh=(unsigned int)rez[i]->size();
          delete rez[i];
          mdd_shape.setNumBins(i,dim_lentgh);
    }   */
    delete [] data;
    delete [] rez;
    arr_dims_vector.clear();


    H5Gclose(descriptors_DSID);

// ***> because of this operator the function accepts full 4D dataset only; if we want accept 1,2 and 3D dataset we need to read pax 
// iax,iint and variable number of p and process them properly;


    dnd.alloc_mdd_arrays(dnd_shape);

//-------------------------------------------------------------------------
// read the dataset itself
// 1) create mem datatype to read data into. 

    hsize_t arr_dims_buf_[MAX_NDIMS_POSSIBLE];
    arr_dims_buf_[0] = 3;
    hid_t   memtype = H5Tarray_create(H5T_NATIVE_DOUBLE, 1, arr_dims_buf_);

/* TO DO: write this check!!!
    // check if the datasize has been calculated properly
    if(real_data_size!=dnd.data_size){
        std::stringstream err;
        err<<"file_hdf_Matlab::read_dnd: dataSize calculated from dimensions= "<<dnd.data_size<<" But real dataSize="<<real_data_size<<std::endl;
        throw(errorMantid(err.str()));
    }
*/
    double *buf = new double[3*(dnd.data_size+1)];
    //** Read the data.   
    err = H5Dread (h_signal_DSID, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf);
    if (err){
        throw(Exception::FileError("error reading signal data from the dataset",this->File_name));
    }
// transform the data into the format specified 
for(unsigned long i=0;i<dnd.data_size;i++){
    dnd.data[i].s   =buf[i*3+0];
    dnd.data[i].err =buf[i*3+1];
    dnd.data[i].npix=(long)buf[i*3+2];
}
delete [] buf;
H5Tclose(memtype);

H5Dclose(h_signal_DSID);

}
//***************************************************************************************

hsize_t
MD_File_hdfMatlab::getNPix(void)
{
    if(this->file_handler<0)return -1;

    this->check_or_open_pix_dataset();
    std::vector<int> arr_dims_vector;
    void *data;
    int rank;
    matlab_attrib_kind   kind;


// Find and read n-pixels attribute to indentify number of the pixels contributed into the mdd dataset
    bool ok=read_matlab_field_attr(this->pixel_dataset_h,"n_pixels",data,arr_dims_vector,rank,kind,this->File_name);
    if(!ok){
        throw(Exception::FileError("Error reading npix sqw attribute",this->File_name));
    }
    hsize_t nPixels=(hsize_t)*((double*)(data));
    delete [] data;

    return nPixels;
}
//***************************************************************************************

bool
MD_File_hdfMatlab::read_pix(MDPixels & sqw)
{ 

    unsigned long i;
    matlab_attrib_kind kind;
    std::vector<int> arr_dims_vector;

    void *data;
    // pixels dims dataset is the 1D dataset of the array datatype
    hsize_t  pix_dims[1],pix_dims_max[1];
// the variable was_opened identify if we have to close dataset after finishing with it (may be not if partial IO operations expected)
    bool was_opened=this->check_or_open_pix_dataset();

    if(this->pixel_dataspace_h<0){
        this->pixel_dataspace_h  = H5Dget_space(this->pixel_dataset_h);
    }

    int rank         = H5Sget_simple_extent_ndims(pixel_dataspace_h);
    if(rank!=1){
        throw(Exception::FileError("MD_File_hdfMatlab::read_pix: the pixel dataspace format differs from the one expected",this->File_name));
    }
    H5Sget_simple_extent_dims(pixel_dataspace_h,pix_dims,pix_dims_max);    
// Find and read n-pixels attribute to indentify number of the pixels contributed into the mdd dataset
    bool ok=read_matlab_field_attr(this->pixel_dataset_h,"n_pixels",data,arr_dims_vector,rank,kind,this->File_name);
    if(!ok){
        throw(Exception::FileError("MD_File_hdfMatlab::read_pix: Error reading npix sqw attribute",this->File_name));
    }
// checks if proper parameter has been read are needed
    sqw.nPixels=(unsigned long)*((double*)(data));
    delete [] data;
    arr_dims_vector.clear();


    if(pix_dims[0]!=sqw.nPixels){
        std::stringstream err;
        err<<"MD_File_hdfMatlab::read_pix: Number of pixels contributed into mdd dataset= "<<sqw.nPixels<<" and does not correspond to number of sqw pixels: "<<pix_dims[0]<<std::endl;
        throw(std::invalid_argument(err.str()));
    }
    hid_t type      = H5Dget_type(this->pixel_dataset_h);
    if(type<0){
        throw(Exception::FileError("MD_File_hdfMatlab::read_pix: can not obtain pixels dataset datatype",this->File_name));
    }
    void *pix_buf;
    bool data_double;
    hid_t data_type=H5Tget_native_type(type,H5T_DIR_ASCEND);
    if(data_type<0){
        throw(Exception::FileError("can not identify native datatype for pixels dataset",this->File_name));
    }

    bool type_error(false);
    try{
        // this does not work so we are not using double data to write pixel information
       // if(data_type==H5T_NATIVE_FLOAT){
       //     pix_buf = new float[pix_dims[0]*DATA_PIX_WIDTH];
       //     data_double = false;
       // }else if(data_type==H5T_NATIVE_DOUBLE){
       //     pix_buf = new double[pix_dims[0]*DATA_PIX_WIDTH];
       //     data_double = true;
       // }else{
            pix_buf = new float[(size_t)(pix_dims[0]*DATA_PIX_WIDTH)];
            data_double = false;

//            type_error=true;
//            throw("pixel wrong");
       // }
    }catch(...){
        if(type_error){
            throw(Exception::FileError("pixel dataset uses datatype different from native float or native double",this->File_name));
        }else{
            return false; // bad alloc thrown
        }
    }
    
    herr_t err=H5Dread(this->pixel_dataset_h, type,H5S_ALL, H5S_ALL, this->file_access_mode,pix_buf);   
    if(err){
        throw(Exception::FileError("Error reading the pixels dataset",this->File_name));
    }
    long nCellPic(0);
    long nPixel(0);
    long j;
    sqw_pixel null;
    null.En=0;
    null.err=0;
    null.idet=0;
    null.ien=0;
    null.irun=0;
    null.s=0;
 
    for(i=0;i<sqw.data_size;i++){
        if(sqw.data[i].npix>0){
            nCellPic  =sqw.data[i].npix;
            sqw.pix_array[i].cell_memPixels.reset(new std::vector<sqw_pixel>(nCellPic,null));
            for(j=0;j<nCellPic;j++){
                if (data_double){
                    sqw.pix_array[i].cell_memPixels->at(j).qx   =  (double)(*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+0));
                    sqw.pix_array[i].cell_memPixels->at(j).qy   =  (double)(*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+1));
                    sqw.pix_array[i].cell_memPixels->at(j).qz   =  (double)(*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+2));
                    sqw.pix_array[i].cell_memPixels->at(j).En   =  (double)(*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+3));
                    sqw.pix_array[i].cell_memPixels->at(j).s    =  (double)(*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+4));
                    sqw.pix_array[i].cell_memPixels->at(j).err  =  (double)(*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+5));
                    sqw.pix_array[i].cell_memPixels->at(j).irun =  (int)   (*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+6));
                    sqw.pix_array[i].cell_memPixels->at(j).idet =  (int)   (*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+7));
                    sqw.pix_array[i].cell_memPixels->at(j).ien  =  (int)   (*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+8));
                }else{
                    sqw.pix_array[i].cell_memPixels->at(j).qx   =  (double)(*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+0));
                    sqw.pix_array[i].cell_memPixels->at(j).qy   =  (double)(*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+1));
                    sqw.pix_array[i].cell_memPixels->at(j).qz   =  (double)(*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+2));
                    sqw.pix_array[i].cell_memPixels->at(j).En   =  (double)(*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+3));
                    sqw.pix_array[i].cell_memPixels->at(j).s    =  (double)(*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+4));
                    sqw.pix_array[i].cell_memPixels->at(j).err  =  (double)(*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+5));
                    sqw.pix_array[i].cell_memPixels->at(j).irun =  (int)   (*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+6));
                    sqw.pix_array[i].cell_memPixels->at(j).idet =  (int)   (*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+7));
                    sqw.pix_array[i].cell_memPixels->at(j).ien  =  (int)   (*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+8));
                }

                nPixel++;
            }
        } // data[i].npix>0;
   }
    delete [] pix_buf;

    H5Tclose(type);
    if(!was_opened){ // if we opened it in this program, we should close it now. 
        H5Sclose(this->pixel_dataspace_h);
        H5Dclose(this->pixel_dataset_h);
        this->pixel_dataset_h=-1;
        this->pixel_dataspace_h=-1;
    }
    return true;
}

size_t 
MD_File_hdfMatlab::read_pix_subset(const MDPixels &SQW,const std::vector<size_t> &selected_cells,size_t starting_cell,sqw_pixel *& pix_buf, size_t &nPix_buf_size,size_t &n_pix_in_buffer)
{
// open pixel dataset and dataspace if it has not been opened before;
    n_pix_in_buffer=0;
    this->check_or_open_pix_dataset();
    bool pixel_dataspece_opened(false);

    // get pixel dataspece and open it if it has not been done before;
    if(this->pixel_dataspace_h<0){
    
        this->pixel_dataspace_h  = H5Dget_space(this->pixel_dataset_h);
        if(this->pixel_dataspace_h<0){
            throw(Exception::FileError("MD_File_hdfMatlab::read_pix_subset: can not get pixels dataspace",this->File_name));
        }
    }else{
        pixel_dataspece_opened=true;
    }
// identify the cells to read and maximal buffer size necessary to do the preselection;
    size_t max_npix_fit(0),max_npix_selected,npix_tt;
    size_t n_cells_processed(starting_cell),i,j,n_cur_cells;
    size_t n_cells_final(selected_cells.size());

    for(i=starting_cell;i<n_cells_final;i++){
        npix_tt          =SQW.data[selected_cells[i]].npix; 
        max_npix_fit    +=npix_tt;

        if(max_npix_fit<=nPix_buf_size){
            max_npix_selected=max_npix_fit;
        }else{
            if(i==starting_cell){
                if(pix_buf){
                    delete [] pix_buf;
                }
                pix_buf       = new sqw_pixel[npix_tt];
                nPix_buf_size = npix_tt;
            }
            break;
        }
        n_cur_cells=i;
    }
    if(!pix_buf){
        pix_buf       = new sqw_pixel[npix_tt];
        nPix_buf_size = npix_tt;
    }
    n_cells_processed=n_cur_cells+1;
    if(max_npix_fit==0){
        if(!pixel_dataspece_opened){
            H5Sclose(this->pixel_dataspace_h);
            this->pixel_dataspace_h=-1;
        }

        return n_cells_processed;
    }


    hid_t type      = H5Dget_type(this->pixel_dataset_h);
    if(type<0){
        throw(Exception::FileError("MD_File_hdfMatlab::read_pix_subset: can not obtain pixels dataset datatype",this->File_name));
    }
    time_t start,end;
    time(&start);  //***********************************************>>>

   // identify the indexes of the preselected pixels;
    std::vector<hsize_t> cells_preselection_buf;
    cells_preselection_buf.resize(max_npix_selected);
    long ic(0);
    for(i=starting_cell;i<n_cells_processed;i++){
        npix_tt          =SQW.data[selected_cells[i]].npix; 
        for(j=0;j<npix_tt;j++){
            cells_preselection_buf[ic]=SQW.pix_array[selected_cells[i]].chunk_file_location0+j;
            ic++;
        }
    }
    herr_t err=H5Sselect_elements(this->pixel_dataspace_h, H5S_SELECT_SET,ic,&cells_preselection_buf[0]);
     if(err<0){
        throw(Exception::FileError("MD_File_hdfMatlab::read_pix_subset: error while doing pixels preselection",this->File_name));
    }
    hsize_t max_npix[2];
    max_npix[0]=max_npix_selected;
    max_npix[1]=0;
    hid_t  mem_space = H5Screate_simple(1,max_npix,NULL);
    H5Sselect_all(mem_space);
    time(&end);     //***********************************************<<<<
    std::stringstream message; 
    message<<" Dataset preselected in: "<<difftime (end,start)<<" sec\n";
    MDData::g_log.debug(message.str());


    time(&start);  //***********************************************>>>
    float *bin_pix_buf = new float[(max_npix_selected+1)*(DATA_PIX_WIDTH)];
 //  herr_t H5Dread(hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, void * buf  ) 
    err  =  H5Dread(this->pixel_dataset_h, type,mem_space, this->pixel_dataspace_h, this->file_access_mode,bin_pix_buf);   
    if(err){
        throw(Exception::FileError("MD_File_hdfMatlab::read_pix_subset: Error reading the pixels dataset",this->File_name));
    }else{
        n_pix_in_buffer=max_npix_selected;
    }
    time(&end); //***********************************************<<<<
    message.clear();
    message<<" Dataset read  in: "<<difftime (end,start)<<" sec\n";
    MDData::g_log.debug(message.str());

    time(&start);  //***********************************************>>>
    for(i=0;i<max_npix_selected;i++){
           pix_buf[i].qx   =  (double)(*((float *)bin_pix_buf+i*DATA_PIX_WIDTH+0));
           pix_buf[i].qy   =  (double)(*((float *)bin_pix_buf+i*DATA_PIX_WIDTH+1));
           pix_buf[i].qz   =  (double)(*((float *)bin_pix_buf+i*DATA_PIX_WIDTH+2));
           pix_buf[i].En   =  (double)(*((float *)bin_pix_buf+i*DATA_PIX_WIDTH+3));
           pix_buf[i].s    =  (double)(*((float *)bin_pix_buf+i*DATA_PIX_WIDTH+4));
           pix_buf[i].err  =  (double)(*((float *)bin_pix_buf+i*DATA_PIX_WIDTH+5));
           pix_buf[i].irun =  (int)   (*((float *)bin_pix_buf+i*DATA_PIX_WIDTH+6));
           pix_buf[i].idet =  (int)   (*((float *)bin_pix_buf+i*DATA_PIX_WIDTH+7));
           pix_buf[i].ien  =  (int)   (*((float *)bin_pix_buf+i*DATA_PIX_WIDTH+8));

    }
    delete [] bin_pix_buf;
    time(&end); //***********************************************<<<<
    
    message.clear();
    message<<" Dataset converted in: "<<difftime (end,start)<<" sec\n";
    MDData::g_log.debug(message.str());

    time(&start);  //***********************************************>>>

    H5Sclose(mem_space);
    H5Tclose(type);
    if(!pixel_dataspece_opened){
        H5Sclose(this->pixel_dataspace_h);
        this->pixel_dataspace_h=-1;
    }
     


//    nPix_buf_size=max_npix_selected;
    time(&end); //***********************************************<<<<

    message.clear();
    message<<" closing all while returning from file_hdf_read : "<<difftime (end,start)<<" sec\n";
    MDData::g_log.debug(message.str());

    return n_cells_processed;
}

MD_File_hdfMatlab::~MD_File_hdfMatlab(void)
{
    if(this->pixel_dataspace_h>0){  H5Sclose(pixel_dataspace_h);
    }

    if(this->pixel_dataset_h>0){    H5Dclose(pixel_dataset_h);
    }
    if(this->file_handler>0)   {    H5Fclose(this->file_handler);
    }

 }

}
}