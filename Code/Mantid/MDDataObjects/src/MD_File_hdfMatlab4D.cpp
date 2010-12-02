#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MD_File_hdfMatlab4D.h"
#include "MDDataObjects/MDWorkspace.h"

namespace Mantid{
    namespace MDDataObjects{



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

bool
MD_File_hdfMatlab4D::read_pix(MDDataPoints & sqw)
{ 

    unsigned long i;

    std::vector<int> arr_dims_vector;

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

// this file-reader deals with 9 of 4 bit-fields only;
    sqw.numFields()     = DATA_PIX_WIDTH;

    // pixel array has to be allocated earlier when image data were read; It is possible then it was less than the pixel buffer. In this case this function has to fail as it is 
    // not possible to read all pixels into memory;
    sqw_pixel *pix_array = (sqw_pixel *)sqw.get_pBuffer();
    if(!pix_array){
      f_log.fatal()<<" pixel array has not been properly allocated\n";
	  throw(std::runtime_error("pixels array has not been allocated properly"));
    }
    size_t n_pix_inDataset  = this->getNPix();
   // let's verify if we indeed can read pixels into the buffer;
    size_t buf_size = sqw.get_pix_bufSize();

    if(buf_size<n_pix_inDataset){
      return false;
    }


    hid_t type      = H5Dget_type(this->pixel_dataset_h);
    if(type<0){
        throw(Exception::FileError("MD_File_hdfMatlab::read_pix: can not obtain pixels dataset datatype",this->File_name));
    }
 
    bool data_double;
    hid_t data_type=H5Tget_native_type(type,H5T_DIR_ASCEND);
    if(data_type<0){
        throw(Exception::FileError("can not identify native datatype for pixels dataset",this->File_name));
    }

    bool type_error(false);
    void *pix_buf;
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
            //delete [] pix_buf;
            throw(Exception::FileError("pixel dataset uses datatype different from native float or native double",this->File_name));
        }else{
            return false; // bad alloc thrown
        }
    }
    
    herr_t err=H5Dread(this->pixel_dataset_h, type,H5S_ALL, H5S_ALL, this->file_access_mode,pix_buf);   
    if(err){
        delete [] pix_buf;
        throw(Exception::FileError("Error reading the pixels dataset",this->File_name));
    }
    size_t nCellPic(0);
    size_t nPixel(0);

  
  
    for(i=0;i<n_pix_inDataset;i++){
        if (data_double){
           pix_array[i].qx   =  (double)(*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+0));
           pix_array[i].qy   =  (double)(*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+1));
           pix_array[i].qz   =  (double)(*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+2));
           pix_array[i].En   =  (double)(*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+3));
           pix_array[i].s    =  (double)(*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+4));
           pix_array[i].err  =  (double)(*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+5));
           pix_array[i].irun =  (int)   (*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+6));
           pix_array[i].idet =  (int)   (*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+7));
           pix_array[i].ien  =  (int)   (*((double *)pix_buf+nPixel*DATA_PIX_WIDTH+8));
         }else{
            pix_array[i].qx   =  (double)(*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+0));
            pix_array[i].qy   =  (double)(*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+1));
            pix_array[i].qz   =  (double)(*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+2));
            pix_array[i].En   =  (double)(*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+3));
            pix_array[i].s    =  (double)(*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+4));
            pix_array[i].err  =  (double)(*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+5));
            pix_array[i].irun =  (int)   (*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+6));
            pix_array[i].idet =  (int)   (*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+7));
            pix_array[i].ien  =  (int)   (*((float *)pix_buf+nPixel*DATA_PIX_WIDTH+8));
        }

       nPixel++;
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
MD_File_hdfMatlab4D::read_pix_subset(const MDImage &SQW,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_raw_buf, size_t &n_pix_in_buffer)
{
// open pixel dataset and dataspace if it has not been opened before;
  
    this->check_or_open_pix_dataset();
    bool pixel_dataspece_opened(false);

     n_pix_in_buffer=0;
     size_t nPix_buf_size = pix_raw_buf.size()/sizeof(sqw_pixel);

    // get access to pixels allocation table;
    const MD_image_point * pData = SQW.get_const_pData();
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
    size_t max_npix_in_buffer(0),max_npix_selected(0),npix_tt;
    size_t i,j,n_selected_cells,n_cells_processed(0);
    size_t n_cells_final(selected_cells.size());

    n_selected_cells=n_cells_final-1;     
    for(i=starting_cell;i<n_cells_final;i++){
        npix_tt             =pData[selected_cells[i]].npix; 
        max_npix_in_buffer +=npix_tt;

        if(max_npix_in_buffer<=nPix_buf_size){
            max_npix_selected=max_npix_in_buffer;
            n_cells_processed++;
        }else{
            // if one cell does not fit the buffer, we should increase buffer size .     
            if(i==starting_cell){
                pix_raw_buf.resize(max_npix_in_buffer*sizeof(sqw_pixel));
                nPix_buf_size = max_npix_in_buffer;
                n_selected_cells=i;
                n_cells_processed=1;
            }else{
                n_selected_cells=i-1;
            }
            break;
        }
    }
    

    if(pix_raw_buf.capacity()<max_npix_in_buffer*sizeof(sqw_pixel)){
        pix_raw_buf.resize(max_npix_in_buffer*sizeof(sqw_pixel));
        nPix_buf_size = max_npix_in_buffer;
    }


    if(max_npix_in_buffer==0){
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
    size_t ic(0);
    hsize_t max_npix_indataset = this->getNPix();
    size_t pixel_num,block_location;
    for(i=starting_cell;i<=n_selected_cells;i++){
        npix_tt          =pData[selected_cells[i]].npix; 
        block_location   =pData[selected_cells[i]].chunk_location;
        for(j=0;j<npix_tt;j++){
            pixel_num=block_location+j;
            // this to go around the bug in hdf dataset creation. 
            if(pixel_num>=max_npix_indataset){
              continue;
            }
            cells_preselection_buf[ic]=pixel_num;
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
    f_log.debug(message.str());


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
    f_log.debug(message.str());

    sqw_pixel *pix_buf =(sqw_pixel *)(&pix_raw_buf[0]);

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
    f_log.debug(message.str());

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
    f_log.debug(message.str());

    return n_cells_processed;
}

MD_File_hdfMatlab4D::~MD_File_hdfMatlab4D(void){}

}
}