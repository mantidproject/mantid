#include "stdafx.h"
#include "MDPixels.h"

               
namespace Mantid{
    namespace MDDataObjects{
        using namespace Mantid::Kernel;
MDPixels::MDPixels(unsigned int nDims):
MDData(nDims),
ignore_inf(false),
ignore_nan(true),
memBased(false),
nPixels(-1),
pix_array(NULL)
{
}
long
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
if(!this->pix_array){
    this->pix_array = new pix_location[this->data_size];
    this->pix_array[0].chunk_file_location0=0;


    for(size_t i=0;i<this->data_size;i++){
        this->pix_array[i].chunk_file_location0= this->pix_array[i-1].chunk_file_location0+this->data[i].npix;
    }
    this->nPixels = this->pix_array[this->data_size-1].chunk_file_location0+this->data[this->data_size-1].npix;

}
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
/*
void
MDPixels::complete_rebinning()
{
size_t i;
// normalize signal and error of the dnd object;
if(this->data[0].npix>0){
    this->data[0].s   /= this->data[0].npix;
    this->data[0].err /=(this->data[0].npix*this->data[0].npix);
}
// and calculate cells location for pixels;
this->pix_array[0].chunk_file_location0=0;
// counter for the number of retatined pixels;
long nPix = this->data[0].npix;
for(i=1;i<this->data_size;i++){   
    this->pix_array[i].chunk_file_location0=this->pix_array[i-1].chunk_file_location0+this->data[i-1].npix; // the next cell starts from the the boundary of the previous one
                                              // plus the number of pixels in the previous cell
    if(this->data[i].npix>0){
        nPix              +=this->data[i].npix;
        this->data[i].s   /=this->data[i].npix;
        this->data[i].err /=(this->data[i].npix*this->data[i].npix);
    }
};
this->nPixels=nPix;
}

//***************************************************************************************
/*
long  
SQW::rebin_dataset4D(const transf_matrix &rescaled_transf, const sqw_pixel *pix_array, long nPix)
{
// set up auxiliary variables and preprocess them. 
double xt,yt,zt,xt1,yt1,zt1,Et,Inf(0),
       pix_Xmin,pix_Ymin,pix_Zmin,pix_Emin,pix_Xmax,pix_Ymax,pix_Zmax,pix_Emax;

double rotations_ustep[9],axis_step_inv[MAX_DND_DIMS],shifts[MAX_DND_DIMS],min_limit[MAX_DND_DIMS],max_limit[MAX_DND_DIMS];
bool  ignore_something,ignote_all,ignore_nan(this->ignore_nan),ignore_inf(this->ignore_inf);

ignore_something=ignore_nan|ignore_inf;
ignote_all      =ignore_nan&ignore_inf;
if(ignore_inf){
    Inf=std::numeric_limits<double>::infinity();
}

//newsqw.rescale_transformations(trf,rotations_ustep,axis_step,shifts,min_limit,max_limit);
for(int ii=0;ii<this->maxNDimsInDataset;ii++){
    axis_step_inv[ii]=1/rescaled_transf.axis_step[ii];
}
for(int ii=0;ii<rescaled_transf.nDimensions;ii++){
    shifts[ii]   =rescaled_transf.trans_bott_left[ii];
    min_limit[ii]=rescaled_transf.cut_min[ii];
    max_limit[ii]=rescaled_transf.cut_max[ii];
}
for(int ii=0;ii<9;ii++){
    rotations_ustep[ii]=rescaled_transf.rotations[ii];
}
int num_OMP_Threads(1);
bool keep_pixels(true);

//int nRealThreads;
long i,indl;
int    indX,indY,indZ,indE;
int    nDimX(this->ndx),nDimY(this->ndy),nDimZ(this->ndz),nDimE(this->nde); // reduction dimensions; if 0, the dimension is reduced;

long  nPixel_retained=0;


// min-max value initialization

pix_Xmin=pix_Ymin=pix_Zmin=pix_Emin=  std::numeric_limits<double>::max();
pix_Xmax=pix_Ymax=pix_Zmax=pix_Emax=- std::numeric_limits<double>::max();
//
// work at least for MSV 2008
#ifdef _OPENMP  
omp_set_num_threads(num_OMP_Threads);


#pragma omp parallel default(none), private(i,j0,xt,yt,zt,xt1,yt1,zt1,Et,indX,indY,indZ,indE), \
     shared(actual_pix_range,this->pix,ok,ind, \
     this->nPixels,newsqw), \
     firstprivate(pix_Xmin,pix_Ymin,pix_Zmin,pix_Emin, pix_Xmax,pix_Ymax,pix_Zmax,pix_Emax,\
                  ignote_all,ignore_nan,ignore_inf,ignore_something,transform_energy,
                  ebin_inv,Inf,trf,\
                  nDimX,nDimY,nDimZ,nDimE), \
     reduction(+:nPixel_retained)
#endif
{
//	#pragma omp master
//{
//    nRealThreads= omp_get_num_threads()
//	 mexPrintf(" n real threads %d :\n",nRealThread);}

#pragma omp for schedule(static,1)
    for(i=0;i<nPix;i++){
        sqw_pixel pix=pix_array[i];

      // Check for the case when either data.s or data.e contain NaNs or Infs, but data.npix is not zero.
      // and handle according to options settings.
            if(ignore_something){
                if(ignote_all){
                    if(pix.s==Inf||isNaN(pix.s)||
                    pix.err==Inf ||isNaN(pix.err)){
                            continue;
                    }
                }else if(ignore_nan){
                    if(isNaN(pix.s)||isNaN(pix.err)){
                        continue;
                    }
                }else if(ignore_inf){
                    if(pix.s==Inf||pix.err==Inf){
                        continue;
                    }
                }
            }

      // Transform the coordinates u1-u4 into the new projection axes, if necessary
      //    indx=[(v(1:3,:)'-repmat(trans_bott_left',[size(v,2),1]))*rot_ustep',v(4,:)'];  % nx4 matrix
            xt1=pix.qx    -shifts[u1];
            yt1=pix.qy    -shifts[u2];
            zt1=pix.qz    -shifts[u3];

            // transform energy
            Et=(pix.En    -shifts[en])*axis_step_inv[en];

//  ok = indx(:,1)>=cut_range(1,1) & indx(:,1)<=cut_range(2,1) & indx(:,2)>=cut_range(1,2) & indx(:,2)<=urange_step(2,2) & ...
//       indx(:,3)>=cut_range(1,3) & indx(:,3)<=cut_range(2,3) & indx(:,4)>=cut_range(1,4) & indx(:,4)<=cut_range(2,4);
            if(Et<min_limit[en]||Et>=max_limit[en])     continue;

            xt=xt1*rotations_ustep[0]+yt1*rotations_ustep[3]+zt1*rotations_ustep[6];
            if(xt<min_limit[u1]||xt>=max_limit[u1])     continue;

            yt=xt1*rotations_ustep[1]+yt1*rotations_ustep[4]+zt1*rotations_ustep[7];
            if(yt<min_limit[u2]||yt>=max_limit[u2])     continue;

            zt=xt1*rotations_ustep[2]+yt1*rotations_ustep[5]+zt1*rotations_ustep[8];
            if(zt<min_limit[u3]||zt>=max_limit[u3])     continue;

            nPixel_retained++;



//     indx=indx(ok,:);    % get good indices (including integration axes and plot axes with only one bin)
            indX=(int)floor(xt-min_limit[u1]);
            indY=(int)floor(yt-min_limit[u2]);
            indZ=(int)floor(zt-min_limit[u3]);
            indE=(int)floor(Et-min_limit[en]);
//
            indl  = indX*nDimX+indY*nDimY+indZ*nDimZ+indE*nDimE;
 // i0=nPixel_retained*OUT_PIXEL_DATA_WIDTH;    // transformed pixels;
#pragma omp atomic
            this->data[indl].s   +=pix.s;  
#pragma omp atomic
            this->data[indl].err +=pix.err;
#pragma omp atomic
            this->data[indl].npix++;
#pragma omp atomic
            this->pix_array[indl].cell_memPixels.push_back(pix);

//
//    actual_pix_range = [min(actual_pix_range(1,:),min(indx,[],1));max(actual_pix_range(2,:),max(indx,[],1))];  % true range of data
            if(xt<pix_Xmin)pix_Xmin=xt;
            if(xt>pix_Xmax)pix_Xmax=xt;

            if(yt<pix_Ymin)pix_Ymin=yt;
            if(yt>pix_Ymax)pix_Ymax=yt;

            if(zt<pix_Zmin)pix_Zmin=zt;
            if(zt>pix_Zmax)pix_Zmax=zt;

            if(Et<pix_Emin)pix_Emin=Et;
            if(Et>pix_Emax)pix_Emax=Et;

    } // end for i -- imlicit barrier;
#pragma omp critical
    {
        if(this->box_min[u1]>pix_Xmin/axis_step_inv[u1])this->box_min[u1]=pix_Xmin/axis_step_inv[u1];
        if(this->box_min[u2]>pix_Ymin/axis_step_inv[u2])this->box_min[u2]=pix_Ymin/axis_step_inv[u2];
        if(this->box_min[u3]>pix_Zmin/axis_step_inv[u3])this->box_min[u3]=pix_Zmin/axis_step_inv[u3];
        if(this->box_min[en]>pix_Emin/axis_step_inv[en])this->box_min[en]=pix_Emin/axis_step_inv[en];

        if(this->box_max[u1]<pix_Xmax/axis_step_inv[u1])this->box_max[u1]=pix_Xmax/axis_step_inv[u1];
        if(this->box_max[u2]<pix_Ymax/axis_step_inv[u2])this->box_max[u2]=pix_Ymax/axis_step_inv[u2];
        if(this->box_max[u3]<pix_Zmax/axis_step_inv[u3])this->box_max[u3]=pix_Zmax/axis_step_inv[u3];
        if(this->box_max[en]<pix_Emax/axis_step_inv[en])this->box_max[en]=pix_Emax/axis_step_inv[en];
    }
} // end parallel region

this->nPixels+=nPixel_retained;
return nPixel_retained;
}
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