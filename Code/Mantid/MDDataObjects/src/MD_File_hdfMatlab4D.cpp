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

//***************************************************************************************
MD_File_hdfMatlab4D::MD_File_hdfMatlab4D(const char *file_name):
    file_handler(-1),
    pixel_dataset_h(-1),
    pixel_dataspace_h(-1),
    file_access_mode(H5P_DEFAULT)

{
  /// check if the file exists and is an hdf5 file;
  this->File_name.assign(file_name);


  htri_t rez=H5Fis_hdf5(File_name.c_str());
  if (rez<=0){
    if (rez==0){
      f_log.error()<<" file "<<File_name<<" is not an hdf5 file\n";
      throw("the file is not an hdf5 file");
    }else{
      f_log.error()<<" error while processing existing hdf5 file: "<<File_name<<" \n";
      throw(" error processing existing hdf file");
    }
  }
  if(!H5Zfilter_avail(H5Z_FILTER_DEFLATE)){
    f_log.error()<<" can not obtain deflate filter (szip or zip) to read MATLAB-hdf file: "<<File_name<<" \n";
    throw("can not obtain deflate filter to read MATLAB-hdf datatypes");
  }
  /// actually opens the file
  this->file_handler = H5Fopen(File_name.c_str(),H5F_ACC_RDONLY,file_access_mode);
  if (file_handler<0){
    f_log.error()<<" error opening existing hdf5 file: "<<File_name<<" \n";
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

//***************************************************************************************
bool 
MD_File_hdfMatlab4D::check_or_open_pix_dataset(void)
{
  bool was_opened(false);
  if(this->pixel_dataset_h<0)
  {
    //this->pixel_dataset_h    = H5Dopen(this->file_handler,this->mdd_field_names[Pixels].c_str(),this->file_access_mode);
    this->pixel_dataset_h    = H5Dopen(this->file_handler,this->mdd_field_names[Pixels].c_str());
    if(this->pixel_dataset_h<0){
      f_log.error()<<" MD_File_hdfMatlab::check_or_open_pix_dataset  Can not open pixels dataset "<< this->mdd_field_names[Pixels]<<" in file: "<<File_name<<" \n";
      throw(Exception::FileError("MD_File_hdfMatlab::check_or_open_pix_dataset: Can not open pixels dataset",this->File_name));
    }
  }else{
    was_opened        = true;
  }

  return was_opened;
}
void 
MD_File_hdfMatlab4D::read_mdd(MDImage & dnd)
{
  // get pointer to MD structure which should be read from memory
  MD_img_data *pMD_struct  = dnd.get_pMDImgData();
  // The function accepts full 4D dataset only!!!
  hid_t h_signal_DSID=H5Dopen(file_handler,this->mdd_field_names[DatasetName].c_str()); //,H5P_DEFAULT);
  if (h_signal_DSID<0){
    f_log.error()<<" MD_File_hdfMatlab::check_or_open_pix_dataset  Can not open mdd dataset "<< this->mdd_field_names[DatasetName]<<" in file: "<<File_name<<" \n";
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
    f_log.error()<<err.str();
    throw(Exception::FileError(err.str(),this->File_name));
  }
  nDims= arr_dims_vector[0];
  MDGeometryDescription dnd_shape(nDims);

  for(i=0;i<nDims;i++){
    unsigned int dim_size=(unsigned int)*((double*)(data)+i);
    dnd_shape.setNumBins(i,dim_size);
  }
  delete [] data;
  arr_dims_vector.clear();


  // read other dataset descriptors
  hid_t descriptors_DSID=H5Gopen(file_handler,this->mdd_field_names[DataDescriptor].c_str()); //,H5P_DEFAULT);
  if (descriptors_DSID<0){
    std::stringstream err;
    err<<"MD_File_hdfMatlab::check_or_open_pix_dataset: Can not open the the data descriptors field in the dataset: "<<mdd_attrib_names[DataDescriptor]<<std::endl;
    f_log.error()<<err.str();
    throw(Exception::FileError(err.str(),this->File_name));
  }
  // read data limits
  ok=read_matlab_field_attr(descriptors_DSID,this->mdd_attrib_names[range],data,arr_dims_vector,rank,kind,this->File_name);
  if(!ok){
    std::stringstream err;
    err<<"MD_File_hdfMatlab::check_or_open_pix_dataset: Error reading mdd data range attribute: "<<mdd_attrib_names[range]<<std::endl;
    f_log.error()<<err.str();
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
    f_log.error()<<err.str();
    throw(Exception::FileError(err.str(),this->File_name));
  }
  if (kind!=double_cellarray){
    throw(Exception::FileError("wrong type identifyed reading data axis",this->File_name));
  }
  // transform 2D array of axis into N-D vector of axis vectors;
  int nData=arr_dims_vector[0]*arr_dims_vector[1];
  double filler = *((double *)(data)+nData);
  std::vector<double> **rez    =(std::vector<double> **)transform_array2cells(data,arr_dims_vector,rank,kind,&filler);
  if(MAX_MD_DIMS_POSSIBLE<=arr_dims_vector[0]){
    f_log.error()<<"file_hdf_Matlab::read_mdd=>algorithm error: number of the data axis in mdd structure residing in file has to be less then MAX_NDIMS_POSSIBLE\n";
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


  dnd.initialize(dnd_shape);

  //-------------------------------------------------------------------------
  // read the dataset itself
  // 1) create mem datatype to read data into.

  hsize_t arr_dims_buf_[MAX_MD_DIMS_POSSIBLE];
  arr_dims_buf_[0] = 3;
  hid_t   memtype = H5Tarray_create2(H5T_NATIVE_DOUBLE, 1, arr_dims_buf_);

  /* TO DO: write this check!!!
    // check if the datasize has been calculated properly
    if(real_data_size!=dnd.data_size){
        std::stringstream err;
        err<<"file_hdf_Matlab::read_dnd: dataSize calculated from dimensions= "<<dnd.data_size<<" But real dataSize="<<real_data_size<<std::endl;
        throw(errorMantid(err.str()));
    }
   */
  double *buf = new double[3*(pMD_struct->data_size+1)];
  //** Read the data.
  err = H5Dread (h_signal_DSID, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf);
  if (err){
    throw(Exception::FileError("error reading signal data from the dataset",this->File_name));
  }
  MD_image_point *pData = pMD_struct->data;
  // transform the data into the format specified
  for(unsigned long i=0;i<pMD_struct->data_size;i++){
    pData[i].s   =buf[i*3+0];
    pData[i].err =buf[i*3+1];
    pData[i].npix=(long)buf[i*3+2];
  }
  delete [] buf;
  H5Tclose(memtype);

  H5Dclose(h_signal_DSID);

}
//***************************************************************************************

hsize_t
MD_File_hdfMatlab4D::getNPix(void)
{
  if(this->file_handler<0)return -1;

  this->check_or_open_pix_dataset();

  hsize_t *data;

  // Analyse pixels array to indentify number of the pixels contributed into the mdd dataset
  hid_t pixels_space = H5Dget_space(this->pixel_dataset_h);
  if(pixels_space<=0){
    throw(Exception::FileError("can not get space for pixel dataset",this->File_name));
  }
  int nDims = H5Sget_simple_extent_ndims(pixels_space);
  if(nDims<=0){
    throw(Exception::FileError("can not obtain pixel dataset dimensions",this->File_name));
  }
  hsize_t nPixels=0;
  data = new hsize_t[nDims];
  H5Sget_simple_extent_dims(pixels_space, data, NULL) ;
  nPixels = data[0];

  delete [] data;

  H5Sclose(pixels_space);
  return nPixels;
}
//***************************************************************************************

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

MD_File_hdfMatlab4D::~MD_File_hdfMatlab4D(void)
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
