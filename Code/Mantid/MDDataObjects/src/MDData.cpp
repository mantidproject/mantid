#include "stdafx.h"
#include "MDData.h"
#include "MD_File_hdfMatlab.h"
#include "MD_File_hdfV1.h"

namespace Mantid{
    namespace MDDataObjects{
//
    using namespace Mantid::Kernel;
//
void
MDData::getPointData(std::vector<point3D> &image_points)const{
    std::vector<unsigned int> selection;
    if(this->n_expanded_dim>3){
        selection.assign(this->n_expanded_dim-3,0);
    }else{
        selection.resize(0);
    }
    this->getPointData(selection,image_points);
     
}
//
void
MDData::getPointData(const std::vector<unsigned int> &selection,std::vector<point3D> &image_points)const
{
    unsigned int selection_size  =  (unsigned int )selection.size();
    if(selection_size >this->n_expanded_dim){
        throw(std::invalid_argument("MDData::getPointData: selection-> attempting to select more dimensions then there are expanded dimensions"));
    }
    unsigned int i,j,k,iMin,jMin,kMin,iMax,jMax,kMax,isel;
    size_t   base(0);

    // calculate shift for all selected dimensions;
    int the_expanded_dim= this->n_expanded_dim-1;
    for(int iii=selection_size-1;iii>=0;iii--){
        if(selection[iii]>=this->getDimension(the_expanded_dim)->getNBins()){
            isel=this->getDimension(the_expanded_dim)->getNBins()-1;
        }else{
            isel=selection[iii];
        }
        if(the_expanded_dim>2){  // all lower dimensions shifs will be processed in the loop;
            base+=this->dimStride[the_expanded_dim]*isel;
        }
        the_expanded_dim--;
    }

    // check how the selection relates to 3 dimensions we are working with;
    unsigned int current_selected_dimension(0);
    size_t   rez_size(0);
    if(the_expanded_dim>=0){ 
        iMin=0;
        iMax=this->getDimension(0)->getNBins();
        rez_size = iMax;
    }else{     
        iMin=selection[current_selected_dimension];
        iMax=selection[current_selected_dimension]+1;
        rez_size = 1;
        current_selected_dimension++;
    }
    std::vector<double> xx;
    this->getDimension(0)->getAxisPoints(xx);


    if(the_expanded_dim>0){   
        jMin=0;
        jMax=this->getDimension(1)->getNBins();
        rez_size *= jMax;
    }else{
        jMin=selection[current_selected_dimension];
        jMax=selection[current_selected_dimension]+1;
        current_selected_dimension++;
    }
    std::vector<double> yy;
    this->getDimension(1)->getAxisPoints(yy);

    if(the_expanded_dim>1){   
        kMin=0;
        kMax=this->getDimension(2)->getNBins();
        rez_size *= kMax;
    }else{
        kMin=selection[current_selected_dimension];
        kMax=selection[current_selected_dimension]+1;
        current_selected_dimension++;
    }
    std::vector<double> zz;
    this->getDimension(2)->getAxisPoints(zz);

// build full array of 3D points

    image_points.resize(rez_size);
    size_t ic(0);
    size_t indexZ,indexY,index;
    for(k=kMin;k<kMax;k++){
        indexZ=base+nd3*k;
        for(j=jMin;j<jMax;j++){
            indexY =indexZ+nd2*j;
            for(i=iMin;i<iMax;i++){
                index=indexY+i;
                image_points[ic].r.x=xx[i];                
                image_points[ic].r.y=yy[j];                
                image_points[ic].r.z=zz[k];                
                image_points[ic].data= this->data[index];
                    
                ic++;
            }
        }
    }

}
/*! function calculates min and max values of the array of 8 points (vertices of a cube)
*
*/
void 
minmax(double &rMin,double &rMax,double box[8])
{
    rMin=box[0];
    rMax=box[0];
    for(int i=1;i<8;i++){
        if(box[i]<rMin)rMin=box[i];
        if(box[i]>rMax)rMax=box[i];
    }
}
/*! function returns the list of the cell numbers which can contribute into the cut described by transformation matrix
 *  input arguments:
 @param matrix          -- the transformation matrix which describes the cut.
 @param cells_to_select -- the list of the cell indexes, which can contribute into the cut
*/
/*
void
DND::preselect_cells(const transf_matrix &matrix, std::vector<long> &cells_to_select,long &n_preselected_pix)
{
    int i;
    n_preselected_pix=0;
    transf_matrix scaled_trf = this->rescale_transformations(matrix);

   // transform the grid into new system of coordinate and calculate cell indexes, which contribute into the 
   // dataset;
   int j,k,l,m,mp,mm,sizem,ind3;
   double xt1,yt1,zt1,Etm,Etp;

   // evaluate the capacity of the orthogonal dimensions;
   int nOrthogonal=this->maxNDimsInDataset-3;
   long ind,orthoSize=1;
   int  nContributed(0);
   std::vector<long> *enInd = new std::vector<long>[nOrthogonal];
   for(l=en;l<this->maxNDimsInDataset;l++){
       nContributed=0;
       sizem  = this->dim_sizes[l];
       for(m=0;m<sizem;m++){
           // is rightmpst for min or leftmost for max in range?
              mp=m+1; 
              mm=m-1; if(mm<0)    mm=0;
           // transforn an axis points into new coordinate system
              Etm=(this->Axis[l]->at(mm)-scaled_trf.trans_bott_left[l])/scaled_trf.axis_step[l];
              Etp=(this->Axis[l]->at(mp)-scaled_trf.trans_bott_left[l])/scaled_trf.axis_step[l];
            // check if it can be in ranges
            if(Etp<scaled_trf.cut_min[l]||Etm>=scaled_trf.cut_max[l]) continue;
            // remember the index of THIS axis 
            enInd[l-en].push_back(m*this->ndn[l]);
            nContributed++;
       }
       orthoSize*=nContributed;
       if(nContributed==0){  // no cells contribute into the cut; Return
           return;
       }

   }
   // multiply all orthogonal vectors providing size(en)*size(ga1)*size(ga2)*size(ga3) matrix;
   std::vector<long> *orthoInd = new std::vector<long>;
   orthoInd->reserve(orthoSize);
   for(i=0;i<enInd[0].size();i++){
       orthoInd->push_back(enInd[0].at(i));
   }
   for(l=1;l<nOrthogonal;l++){
       size_t orthSize=orthoInd->size();
       for(j=0;j<orthSize;j++){
           long indDim0=orthoInd->at(j);
           for(i=0;enInd[l].size();i++){
               orthoInd->push_back(indDim0+enInd[l].at(i));
           }
       }
   }
   delete [] enInd;

// evaluate the capacity of the 3D space;
// Define 3D subspace and transform it into the coordinate system of the new box;
   long size3D(1);
   for(i=0;i<3;i++){
           size3D*=(this->dim_sizes[i]+1);
   }

   std::vector<double> rx,ry,rz,xx,yy,zz;
   rx.reserve(size3D); xx.reserve(size3D);
   ry.reserve(size3D); yy.reserve(size3D);
   rz.reserve(size3D); zz.reserve(size3D);
   for(k=0;k<=this->dim_sizes[u3];k++){
       for(j=0;j<=this->dim_sizes[u2];j++){
           for(i=0;i<=this->dim_sizes[u1];i++){
               xx.push_back(Axis[u1]->at(i));
               yy.push_back(Axis[u2]->at(j));
               zz.push_back(Axis[u3]->at(k));

               rx.push_back(Axis[u1]->at(i)-scaled_trf.trans_bott_left[u1]);
               ry.push_back(Axis[u2]->at(j)-scaled_trf.trans_bott_left[u2]);
               rz.push_back(Axis[u3]->at(k)-scaled_trf.trans_bott_left[u3]);
           }
       }
   }
   for(i=0;i<size3D;i++){
        xt1=rx[i];yt1=ry[i];zt1=rz[i];

        xx[i]=xt1*scaled_trf.rotations[0]+yt1*scaled_trf.rotations[3]+zt1*scaled_trf.rotations[6];
        yy[i]=xt1*scaled_trf.rotations[1]+yt1*scaled_trf.rotations[4]+zt1*scaled_trf.rotations[7];
        zz[i]=xt1*scaled_trf.rotations[2]+yt1*scaled_trf.rotations[5]+zt1*scaled_trf.rotations[8];
   }
   rx.clear();   ry.clear();   rz.clear();
   int im,ip,jm,jp,km,kp;
   nCell3D sh(this->dim_sizes[u1]+1,this->dim_sizes[u2]+1),
           ind3D(this->dim_sizes[u1],this->dim_sizes[u2]);
   double r[8],rMin,rMax;

   for(k=0;k<dim_sizes[u3];k++){
       km=k-1; if(km<0)            km=0;
       kp=k+1; 
       for(j=0;j<dim_sizes[u2];j++){
           jm=j-1; if(jm<0)            jm=0;
           jp=j+1; 

           for(i=0;i<dim_sizes[u1];i++){
               im=i-1; if(im<0)            im=0;
               ip=i+1; 

               r[0]=xx[sh.nCell(im,jm,km)];  r[1]=xx[sh.nCell(ip,jm,km)];r[2]=xx[sh.nCell(im,jp,km)];  r[3]=xx[sh.nCell(ip,jp,km)];
               r[4]=xx[sh.nCell(im,jm,kp)];  r[5]=xx[sh.nCell(ip,jm,kp)];r[6]=xx[sh.nCell(im,jp,kp)];  r[7]=xx[sh.nCell(ip,jp,kp)];
    
               minmax(rMin,rMax,r);
               if(rMax<scaled_trf.cut_min[u1]||rMin>=scaled_trf.cut_max[u1])continue;

               r[0]=yy[sh.nCell(im,jm,km)];  r[1]=yy[sh.nCell(ip,jm,km)];r[2]=yy[sh.nCell(im,jp,km)];  r[3]=yy[sh.nCell(ip,jp,km)];
               r[4]=yy[sh.nCell(im,jm,kp)];  r[5]=yy[sh.nCell(ip,jm,kp)];r[6]=yy[sh.nCell(im,jp,kp)];  r[7]=yy[sh.nCell(ip,jp,kp)];
    
               minmax(rMin,rMax,r);
               if(rMax<scaled_trf.cut_min[u2]||rMin>=scaled_trf.cut_max[u2])continue;

               r[0]=zz[sh.nCell(im,jm,km)];  r[1]=zz[sh.nCell(ip,jm,km)];r[2]=zz[sh.nCell(im,jp,km)];  r[3]=zz[sh.nCell(ip,jp,km)];
               r[4]=zz[sh.nCell(im,jm,kp)];  r[5]=zz[sh.nCell(ip,jm,kp)];r[6]=zz[sh.nCell(im,jp,kp)];  r[7]=zz[sh.nCell(ip,jp,kp)];
    
               minmax(rMin,rMax,r);
               if(rMax<scaled_trf.cut_min[u3]||rMin>=scaled_trf.cut_max[u3])continue;

               ind3=i*this->ndx+j*this->ndy+k*this->ndz;
               for(l=0;l<orthoInd->size();l++){
                   ind = ind3+orthoInd->at(l);
                   if(this->data[ind].npix>0){
                        cells_to_select.push_back(ind);
                        n_preselected_pix+=this->data[ind].npix;
                   }
               }


           }
       }
    }
    delete orthoInd;
}
*/
//****************************************

//****************************************
// function rescales the transformation matrix which is initially expressed in physical units into the coordinate system of this object.
// 
/*
void
DND::rescale_transformations(const transf_matrix &trf,
                             std::vector<double >* const Axis[MAX_DND_DIMS],
                             double rotations[9],
                             double axis_step[MAX_DND_DIMS],double shifts[MAX_DND_DIMS],
                             double min_limit[MAX_DND_DIMS],double max_limit[MAX_DND_DIMS])
{


}
//
//*****************
*/
void
MDData::select_file_reader(const char *file_name)
{
    
// check if the file exist;
    std::ifstream infile;
    infile.open(file_name);
    infile.close();
    if (infile.fail()){
        throw(Exception::FileError("MDData::select_file_reader: Error->can not found or open",file_name));
    }
// check if it is hdf5
    htri_t rez=H5Fis_hdf5(file_name);
    if (rez<=0){
        if (rez==0){
            throw(Exception::FileError("MDData::select_file_reader: Error->the file is not hdf5 file",file_name));
        }else{
            throw(Exception::FileError("MDData::select_file_reader: Error->unspecified hdf5 error ",file_name));
        }
    }else{ 
        // ***> to do:: identify internal hdf5 format; only MATLAB is supported at the moment;
        this->theFile= new MD_File_hdfMatlab(file_name);
    }

}
//
/*
void 
DND::write_mdd(const char *file_name){
     // pick up current default file reader/writer;
    if (this->theFile){
        if(!this->theFile->is_open()){
            delete this->theFile;
            this->theFile=new mdd_hdf(file_name);
        }
    }else{
         this->theFile=new mdd_hdf(file_name);
    }
    this->write_mdd();
}



*/
//*******************************************************************************************************
size_t
MDData::reshape_geometry(const SlicingData &transf)
{
   unsigned int i;

   // all paxis in the transformation matrix have to be defined properly and in accordance with the transformation data. 
   this->reinit_Geometry(transf.getPAxis());

   // set the this object limits as the limits from transf class
   this->setRanges(transf);

   this->dimSizes.assign(this->n_total_dim,0);
   this->dimStride.assign(MAX_NDIMS_POSSIBLE+1,0);

    Dimension *pDim;
    this->dimStride[0] = 1;
    this->data_size    = 1;
    for(i=0;i<this->n_total_dim;i++){
        pDim                 = this->getDimension(i);
        this->dimSizes[i]    = pDim->getNBins();
        this->data_size     *= this->dimSizes[i];
        this->dimStride[i+1] = this->data_size;
    }

    this->nd2 =dimStride[ek];
    this->nd3 =dimStride[el];
    this->nd4 =dimStride[en];
    this->nd5 =dimStride[u1];
    this->nd6 =dimStride[u2];
    this->nd7 =dimStride[u3];
    this->nd8 =dimStride[u4];
    this->nd9 =dimStride[u5];
    this->nd10=dimStride[u6];
    this->nd11=dimStride[u7];

    return data_size;
}
void 
MDData::alloc_mdd_arrays(const SlicingData &transf)
{

// initiate initial dimensions
   if(this->data){
       this->clear_class();
   }
   this->data_size=this->reshape_geometry(transf);



// allocate main data array;
    data = new data_point[data_size];
    if (!data){
        throw(std::bad_alloc("Can not allocate memory to keep Multidimensional dataset"));
    }

    for(unsigned long j=0;j<data_size;j++){
        this->data[j].s   =0;
        this->data[j].err =0;
        this->data[j].npix=0;
    }

}
MDData::MDData(unsigned int nDims):
MDGeometry(nDims),
data_size(0),
data(NULL),
theFile(NULL),
nd2(0),nd3(0),nd4(0),nd5(0),nd6(0),nd7(0),nd8(0),nd9(0),nd10(0),nd11(0)
{
    if(nDims>MAX_NDIMS_POSSIBLE){
        throw(std::invalid_argument("MDData::MDData number of dimensions exceeds the possible value"));
    }
    this->dimSizes.assign(nDims,0);
    this->dimStride.assign(nDims+1,0);
}
//
MDData::~MDData()
{
    this->clear_class();
}
//***************************************************************************************
void
MDData::clear_class(void)
{
    if(data){
        delete [] data;
        data = NULL;
    }
    this->dimSizes.assign(this->n_total_dim,0);

    if(theFile){
        delete theFile;
        theFile=NULL;
    }

}

}
}
