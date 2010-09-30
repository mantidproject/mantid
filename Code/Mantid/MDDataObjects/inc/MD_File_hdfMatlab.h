#ifndef H_FILE_HDF_MATLAB
#define H_FILE_HDF_MATLAB
#include "MD_FileFormat.h"
#include "MDPixels.h"
#include "SlicingData.h"
/**    Class supports MATLAB-written hdf5 mdd data format and will be used at the initial stage of the development;
*      to read the data initially provided by MATLAB, Horace
*/

namespace Mantid{
    namespace MDDataObjects{
        using namespace Mantid::Kernel;
//
class MD_File_hdfMatlab :    public MD_FileFormat
{
public:
    MD_File_hdfMatlab(const char *file_name);

    virtual bool is_open(void)const{
        if(this->file_handler>0){return true;
        }else{                   return false;}
    }
    virtual void read_mdd(MDData & mdd);
    
    /// read whole pixels information in memory; usually impossible, then returns false;
    virtual bool read_pix(MDPixels & sqw);
    // read the information from the data pixels, specified by the selected cells, returns the number of cells actually processed 
    /// by this read operation
    virtual size_t read_pix_subset(const MDPixels &sqw,const std::vector<long> &selected_cells,long starting_cell,sqw_pixel *& pix_buf, long &nPixels){return 0;}
    /// get number of data pixels contributing into the dataset;
    virtual hsize_t getNPix(void);
    /// not implemented and probably will not be as we will develop our own mdd_hdf format
    virtual void write_mdd(const MDData & dnd){throw(Exception::NotImplementedError("write_mdd-Matlab format function is not supported and should not be used"));}
    
    virtual ~MD_File_hdfMatlab(void);
private:
    /// name of a file which keeps mdd dataset;
    std::string File_name;
    /// the variable which provides access to the open hdf file
    hid_t file_handler;
   /// the variable to access open pixels dataset (necessary for partial read operations)
    hid_t pixel_dataset_h;
   // the variable to deal with pixel dataspace; Especially usefull when dealing with number of partial reading operations;
    hid_t pixel_dataspace_h;
   /// the variable describes file access mode, which is complicated if parallel access is used 
    hid_t file_access_mode;

    /// the vector of DND field names used by the reader/writer
    std::vector<std::string> mdd_field_names;
    ///  the vector of mdd_hdf attributes used by the reader/writer
    std::vector<std::string> mdd_attrib_names;

///  number of fields in HORACE sqw dataset;
    static const int  DATA_PIX_WIDTH=9;

// not used at the moment
//   static std::stringstream ErrBuf;
// private copy constructor and assighnment
   MD_File_hdfMatlab(const MD_File_hdfMatlab& p){};
   MD_File_hdfMatlab & operator = (const MD_File_hdfMatlab & other);

   // function checks if pixel dataset is opened and if not opens it. true if it was already opened, false if did nothing
   bool check_or_open_pix_dataset(void);

};
//
    }
}
#endif
