#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDPixels.h"

               
namespace Mantid{
    namespace MDDataObjects{
        using namespace Mantid::Kernel;
MDPixels::MDPixels(unsigned int nDims):
MDData(nDims),
ignore_inf(false),
ignore_nan(true),
memBased(false),
nPixels(0),
pix_array(NULL)
{
    this->box_min.assign(nDims,FLT_MAX);
    this->box_max.assign(nDims,-FLT_MAX);
}
size_t
MDPixels::getNumPixels(void)
{
    if(this->nPixels>=0){   return nPixels;
    }

    if(this->theFile){
        this->nPixels=this->theFile->getNPix();
    }else{
        MDData::g_log.information("MDPixels::getNumPixels: Attemting to get number of pixels from undefined dataset");
        
    }
    return nPixels;
}
// 
void 
MDPixels::read_mdd(const char *file_name)
{
    // select file reader and read image part of the data
    this->MDData::read_mdd(file_name);
    // alocate memory for pixels;
    this->alloc_pix_array();
}
//
void
MDPixels::read_pix(void)
{
        if(this->theFile){
            if(!this->data){
                throw(Exception::NullPointerException("MDPixels::read_pix","MDPixels->data"));
            }
            this->alloc_pix_array();
            if(!this->theFile->read_pix(*this)){
                this->memBased=false;
                throw(std::bad_alloc("can not place all data pixels in the memory, file operations needs to be performed"));
            }else{
                this->memBased=true;
            }
        }else{
            throw(Exception::NullPointerException("MDPixels::read_pix","MDPixels->theFile"));
        }
}
 
//***************************************************************************************
void
MDPixels::alloc_pix_array()
{
    /*
if(!this->pix_array){
    this->pix_array = new pix_location[this->data_size];
    this->pix_array[0].chunk_file_location0=0;


    for(size_t i=0;i<this->data_size;i++){
        this->pix_array[i].chunk_file_location0= this->pix_array[i-1].chunk_file_location0+this->data[i].npix;
    }
    this->nPixels = this->pix_array[this->data_size-1].chunk_file_location0+this->data[this->data_size-1].npix;

}
*/
}
//***************************************************************************************
MDPixels::~MDPixels()
{
    if(pix_array){
        delete [] pix_array;
    }
}
size_t 
MDPixels::read_pix_selection(const std::vector<size_t> &cells_nums,size_t &start_cell,sqw_pixel *& pix_buf,size_t &pix_buf_size,size_t &n_pix_in_buffer)
{
    if(!this->theFile){
        throw(std::bad_alloc("MDPixels::read_selected_pix: file reader has not been defined"));
    }
    return this->theFile->read_pix_subset(*this,cells_nums,start_cell,pix_buf,pix_buf_size,n_pix_in_buffer);
}
//***************************************************************************************

void
MDPixels::finalise_rebinning()
{
size_t i;
// normalize signal and error of the dnd object;
if(this->data[0].npix>0){
    this->data[0].s   /= this->data[0].npix;
    this->data[0].err /=(this->data[0].npix*this->data[0].npix);
}
// and calculate cells location for pixels;
this->data[0].chunk_location=0;

// counter for the number of retatined pixels;
size_t nPix = this->data[0].npix;
for(i=1;i<this->data_size;i++){   
    this->data[i].chunk_location=this->data[i-1].chunk_location+this->data[i-1].npix; // the next cell starts from the the boundary of the previous one
                                              // plus the number of pixels in the previous cell
    if(this->data[i].npix>0){
        nPix              +=this->data[i].npix;
        this->data[i].s   /=this->data[i].npix;
        this->data[i].err /=(this->data[i].npix*this->data[i].npix);
    }
};
this->nPixels=nPix;
}//***************************************************************************************

/*
//***************************************************************************************
void
SQW::extract_pixels_from_memCells(const std::vector<long> &selected_cells,long nPix,sqw_pixel *pix_extracted)
{
    long i,ind,ic(0);
    size_t j,npix;
    for(i=0;i<selected_cells.size();i++){
        ind=selected_cells[i];
        npix=this->pix_array[ind].cell_memPixels.size();
        for(j=0;j<npix;j++){
            pix_extracted[ic]=this->pix_array[ind].cell_memPixels[j];
            ic++;
#ifdef _DEBUG
            if(ic>nPix){
                throw("extract_pixels_from_memCells::Algorithm error=> there are more real pixels then was estimated during preselection");
            }
#endif
        }
    }
}
/*
//***************************************************************************************
void 
SQW::rebin_dataset4D(const transf_matrix &trf,sqw &newsqw)
{
    std::vector<long> selected_cells;
    long n_preselected_pixels; // number of preselected pixels siting in the array of preselected cells;
    time_t start,end;
    time(&start);  //////////////////////////////////////

    this->preselect_cells(trf, selected_cells,n_preselected_pixels);

    time(&end);   /////////////////////////////////////////////
    double dif=difftime (end,start);
    std::cout<<"***********  cells choosen in "<<dif<<" sec\n";

    if(selected_cells.size()==0||n_preselected_pixels==0){ // dataset is empty; nothing to select;
        return;
    }

    if(this->memBased){
        sqw_pixel *pix_extracted= new sqw_pixel[n_preselected_pixels];  // pointer to array of preselected pixels;

        this->extract_pixels_from_memCells(selected_cells,n_preselected_pixels,pix_extracted);
        selected_cells.clear(); // empty array of selected cells indexes to save memory;

        // rescale transformation to the newsqw coordinate system
        transf_matrix transf_rescaled = newsqw.rescale_transformations(trf,false);
        newsqw.rebin_dataset4D(transf_rescaled,pix_extracted,n_preselected_pixels);
        
        delete [] pix_extracted;
        // scale signals and errors in new dataset to correct values;
        newsqw.complete_rebinning();
    }else{
        long n_pixels_to_read = PIX_BUFFER_SIZE;

        time(&start); //////////////////////////////////////////////
        
        this->alloc_pix_array();

        time(&end);   //////////////////////////////////////////////
        double dif = difftime (end,start);
        std::cout<<"***********  time to allocate pix array: "<<dif<<" sec\n";


        sqw_pixel *pix_buf = new sqw_pixel[n_pixels_to_read];
        long nCells_selected = selected_cells.size();
        long start_cell=0,end_cell;
        time_t start,end;


        transf_matrix transf_rescaled = newsqw.rescale_transformations(trf,false);
        while(start_cell<nCells_selected){
            time(&start);    ////////////////////////////////

            // read subset of pixels fitting the buffer
            end_cell=this->theFile->read_pix_subset(*this,selected_cells,start_cell,pix_buf, n_pixels_to_read);

            time(&end);      //////////////////////////////////////////////////////
            std::cout<<"*********** data chunk obtained in: "<<difftime(end,start)<<" sec\n";


            std::cout<<"*********** read "<<n_pixels_to_read<<" pixels from "<<end_cell-start_cell<<"  cells, retained: ";
            start_cell=end_cell;
            // rebin the subset to new object;
            time(&start);   /////////////////////////////////////////////
            long n_pix_retained=newsqw.rebin_dataset4D(transf_rescaled,pix_buf,n_pixels_to_read);
            time(&end);     /////////////////////////////////////////////

            std::cout<< n_pix_retained<< " pixels\n";
            std::cout<<"***********  data chunk processed in: "<<difftime(end,start)<<" sec\n";

            // reset buffer size to maximal size again;
            n_pixels_to_read=PIX_BUFFER_SIZE;
        }

        delete [] pix_buf;
    }

}
*/
}
}