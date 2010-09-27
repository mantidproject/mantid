#ifndef H_FILE_FORMAT
#define H_FILE_FORMAT
// interface to various DND file formats (and may be other parts of sqw file in a future);
#include <hdf5.h>
class DND;
class SQW;
struct sqw_pixel;

class file_format
{
public:
    file_format(void){};
    virtual bool is_open(void)const{return false;}
    virtual void read_dnd(DND &)=0;
    virtual bool read_pix(SQW &)=0; 
    virtual size_t read_pix_subset(const SQW &,const std::vector<long> &selected_cells,long starting_cell,sqw_pixel *& pix_buf, long &nPixels)=0; 
    virtual hsize_t getNPix(void)=0;
    virtual void write_dnd(const DND &)=0;
    virtual ~file_format(void){};
};
#endif