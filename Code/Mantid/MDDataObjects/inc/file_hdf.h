#ifndef H_FILE_HDF
#define H_FILE_HDF
#include "file_format.h"
// stub for a future binary (hdf) file format reader/writer which will be used for datasets.
class dnd_hdf :    public file_format
{
public:
    dnd_hdf(const char *file_name){throw("this format is not written yet");}
    virtual ~dnd_hdf(void){};
    virtual bool is_open(void)const{return false;}
    virtual void read_dnd(DND &){};
    virtual void write_dnd(const DND &){};
    virtual bool read_pix(SQW &){return false; }
    virtual size_t read_pix_subset(const SQW &,const std::vector<long> &selected_cells,long starting_cell,sqw_pixel *& pix_buf, long &nPixels){
        return selected_cells.size();}
    virtual hsize_t getNPix(void){return -1;}

};
#endif
