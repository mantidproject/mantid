#ifndef H_FILE_HDF_MATLAB
#define H_FILE_HDF_MATLAB
#include "file_format.h"
#include "sqw.h"
#include "SlicingData.h"
/*
*   Class supports test MATLAB written hdf5 dnd data format and will be used at the initial stage of the development;
*   to read the data initially provided by MATLAB
*/
// enum to identify fields, involved in MATLAB horace DND data structure;
enum matlab_dnd_fields_list{
    nDND_dims,
    range,
    axis,
    n_MATLAB_DND_fields
};

class file_hdf_Matlab :    public file_format
{
public:
    file_hdf_Matlab(const char *file_name);
    virtual bool is_open(void)const{
        if(this->file_handler>0){return true;
        }else{                   return false;}
    }
    virtual void read_dnd(DND & dnd);
    
    /// read whole pixels information in memory; usually impossible, then returns false;
    virtual bool read_pix(SQW & sqw){return false;}
    // read the information from the data pixels, specified by the selected cells, returns the number of cells actually processed 
    /// by this read operation
    virtual size_t read_pix_subset(const SQW &sqw,const std::vector<long> &selected_cells,long starting_cell,sqw_pixel *& pix_buf, long &nPixels){return 0;}
    /// get number of data pixels contributing into the dataset;
    virtual hsize_t getNPix(void);
    /// not implemented and probably will not be as we will develop our own dnd_hdf format
    virtual void write_dnd(const DND & dnd){throw("this function is not supported and should not be used");}
    
    virtual ~file_hdf_Matlab(void);
private:
// name of a file which keeps dnd dataset;
    std::string File_name;
// the variable which provides access to the open hdf file
    hid_t file_handler;


// the variable to access open pixels dataset (necessary for partial read operations)
    hid_t pixel_dataset_h;
   // the variable to deal with pixel dataspace; Especially usefull when dealing with number of partial reading operations;
    hid_t pixel_dataspace_h;
// the variable describes file access mode, which is complicated if parallel access is used 
    hid_t file_access_mode;

// dnd dataset name in MATLAB file;
    static const char *DatasetName;  //="Signals";
// descriptors field name
    static const char *descriptor;  //="spe_header"
//  pixels dataset name
    static const char *pixels;      //="pix"
//  number of fields in HORACE sqw dataset;
    static const int  DATA_PIX_WIDTH=9;
//  dnd_hdf attributes;
   std::string **MATLAB_dnd_fields;

// not used at the moment
//   static std::stringstream ErrBuf;
// private copy constructor and assighnment
   file_hdf_Matlab(const file_hdf_Matlab& p){};
   file_hdf_Matlab & operator = (const file_hdf_Matlab & other){};

   // function checks if pixel dataset is opened and if not opens it. true if it was already opened, false if did nothing
   bool check_or_open_pix_dataset(void);

};

#endif
