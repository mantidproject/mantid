#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDImageData.h"
#include "MDDataObjects/MD_File_hdfMatlab4D.h"
#include "MDDataObjects/MD_File_hdfMatlab.h"
#include "MDDataObjects/MD_File_hdfV1.h"
#include "MantidGeometry/MDGeometry/MDCell.h"


namespace Mantid{
    namespace MDDataObjects{
//
    using namespace Mantid::API;
    using namespace Mantid::Kernel;
// logger for MD workspaces  
    Kernel::Logger& MDImageData::g_log =Kernel::Logger::get("MDWorkspaces");




void
MDImageData::getPointData(std::vector<point3D> &image_points)const{
    std::vector<unsigned int> selection;
    if(this->m_pMDGeometry->getNumExpandedDims()>3){
        selection.assign(this->m_pMDGeometry->getNumExpandedDims()-3,0);
    }else{
        selection.resize(0);
    }
    this->getPointData(selection,image_points);

}
//
void
MDImageData::getPointData(const std::vector<unsigned int> &selection,std::vector<point3D> &image_points)const
{
    unsigned int selection_size  =  (unsigned int )selection.size();
    if(selection_size >this->m_pMDGeometry->getNumExpandedDims()){
        throw(std::invalid_argument("MDImaegData::getPointData: selection-> attempting to select more dimensions then there are expanded dimensions"));
    }
    unsigned int i,j,k,iMin,jMin,kMin,iMax,jMax,kMax,isel;
    size_t   base(0);
    MDDimension *pDim;

    // calculate shift for all selected dimensions;
    int the_expanded_dim= this->m_pMDGeometry->getNumExpandedDims()-1;
    for(int iii=selection_size-1;iii>=0;iii--){
        pDim = this->m_pMDGeometry->getDimension(the_expanded_dim);
        if(selection[iii]>=pDim->getNBins()){
            isel=pDim->getNBins()-1;
        }else{
            isel=selection[iii];
        }
        if(the_expanded_dim>2){  // all lower dimensions shifs will be processed in the loop;
            base+=pDim->getStride()*isel;
        }
        the_expanded_dim--;
    }

    // check how the selection relates to 3 dimensions we are working with;
    unsigned int current_selected_dimension(0);
    size_t   rez_size(0);
    if(the_expanded_dim>=0){
        iMin=0;
        iMax=this->m_pMDGeometry->getDimension(0)->getNBins();
        rez_size = iMax;
    }else{
        iMin=selection[current_selected_dimension];
        iMax=selection[current_selected_dimension]+1;
        rez_size = 1;
        current_selected_dimension++;
    }
    std::vector<double> xx;
    this->m_pMDGeometry->getDimension(0)->getAxisPoints(xx);


    if(the_expanded_dim>0){
        jMin=0;
        jMax=this->m_pMDGeometry->getDimension(1)->getNBins();
        rez_size *= jMax;
    }else{
        jMin=selection[current_selected_dimension];
        jMax=selection[current_selected_dimension]+1;
        current_selected_dimension++;
    }
    std::vector<double> yy;
    this->m_pMDGeometry->getDimension(1)->getAxisPoints(yy);

    if(the_expanded_dim>1){
        kMin=0;
        kMax=this->m_pMDGeometry->getDimension(2)->getNBins();
        rez_size *= kMax;
    }else{
        kMin=selection[current_selected_dimension];
        kMax=selection[current_selected_dimension]+1;
        current_selected_dimension++;
    }
    std::vector<double> zz;
    this->m_pMDGeometry->getDimension(2)->getAxisPoints(zz);

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
                image_points[ic].X()=xx[i];
                image_points[ic].Y()=yy[j];
                image_points[ic].Z()=zz[k];
                image_points[ic]  = this->pData[index];

                ic++;
            }
        }
    }

}
//



//****************************************


//
MD_image_point *
MDImageData::get_pData(void)
{
    if(pData){
        return pData;
    }else{
        throw(std::bad_alloc("Data memory for Multidimensional dataset has not been allocated"));
    }
}
MD_image_point const*
MDImageData::get_const_pData(void)const
{
    if(pData){
        return pData;
    }else{
        throw(std::bad_alloc("Data memory for Multidimensional dataset has not been allocated"));
    }
}
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
MDImageData::reshape_geometry(const MDGeometryDescription &transf)
{
   unsigned int i;

   // all paxis in the transformation matrix have to be defined properly and in accordance with the transformation data.
   // also sets the the dimension limits and object limits as the limits from transf class
   this->m_pMDGeometry->reinit_Geometry(transf);

   
   this->MDStruct.dimSize.assign(this->m_pMDGeometry->getNumDims(),0);
   this->MDStruct.dimStride.assign(MAX_MD_DIMS_POSSIBLE+1,0);

    MDDimension *pDim;
    this->MDStruct.dimStride[0] = 0;
    this->MDStruct.data_size    = 1;
    size_t  stride(1);
    for(i=0;i<this->m_pMDGeometry->getNumDims();i++){
      pDim                 = this->m_pMDGeometry->getDimension(i);
        stride               = pDim->getStride();
        this->MDStruct.dimSize[i]    =  pDim->getNBins();
        this->MDStruct.data_size     *= this->MDStruct.dimSize[i];

        this->MDStruct.dimStride[i+1] = this->MDStruct.data_size;

    }

    this->nd2 =MDStruct.dimStride[0];
    this->nd3 =MDStruct.dimStride[1];
    this->nd4 =MDStruct.dimStride[2];
    this->nd5 =MDStruct.dimStride[3];
    this->nd6 =MDStruct.dimStride[4];
    this->nd7 =MDStruct.dimStride[5];
    this->nd8 =MDStruct.dimStride[6];
    this->nd9 =MDStruct.dimStride[7];
    this->nd10=MDStruct.dimStride[8];
    this->nd11=MDStruct.dimStride[9];

    return MDStruct.data_size;
}
void
MDImageData::alloc_mdd_arrays(const MDGeometryDescription &transf)
{

// initiate initial dimensions
   if(this->pData){
       this->clear_class();
   }
   this->MDStruct.data_size=this->reshape_geometry(transf);



// allocate main data array;
    pData = new MD_image_point[MDStruct.data_size];
    if (!pData){
        throw(std::bad_alloc("Can not allocate memory to keep Multidimensional dataset"));
    }
    MDStruct.data  = pData;

    for(unsigned long j=0;j<MDStruct.data_size;j++){
        this->pData[j].s   =0;
        this->pData[j].err =0;
        this->pData[j].npix=0;
    }
    this->MDStruct.min_value.assign(this->m_pMDGeometry->getNumDims(), FLT_MAX);
    this->MDStruct.max_value.assign(this->m_pMDGeometry->getNumDims(),-FLT_MAX);
    

}

MDImageData::MDImageData():
pData(NULL)
{
  this->m_pMDGeometry = boost::shared_ptr<MDGeometry>(new MDGeometry()); //HACK
  MDGeometryDescription tt(*(m_pMDGeometry));
  this->alloc_mdd_arrays(tt);
}

//
MDImageData::MDImageData(boost::shared_ptr<Mantid::Geometry::MDGeometry> spMDGeometry):
m_pMDGeometry(spMDGeometry),
pData(NULL),
nd2(0),nd3(0),nd4(0),nd5(0),nd6(0),nd7(0),nd8(0),nd9(0),nd10(0),nd11(0)
{
  int nDims = m_pMDGeometry->getNumDims();
  if( nDims >MAX_MD_DIMS_POSSIBLE){
    throw(std::invalid_argument("MDData::MDData number of dimensions exceeds the possible value"));
  }

  this->MDStruct.data_size = 0;
  this->MDStruct.dimSize.assign(nDims,0);
  this->MDStruct.dimStride.assign(nDims+1,0);
  this->MDStruct.min_value.assign(nDims, FLT_MAX);
  this->MDStruct.max_value.assign(nDims,-FLT_MAX);
}
//
MDImageData::~MDImageData()
{
    this->clear_class();
}

std::vector<size_t>
MDImageData::getStrides(void)const
{
  unsigned int nDims = this->m_pMDGeometry->getNumDims();
  std::vector<size_t> strides(nDims,0);
  for(unsigned int i=0;i<nDims;i++){
    strides[i] = m_pMDGeometry->getDimension(i)->getStride();
  }
  return strides;

}
//
void
MDImageData::identify_SP_points_locations()
{
    // and calculate cells location for pixels;
    this->pData[0].chunk_location=0;

    // counter for the number of retatined pixels;
    size_t nPix = this->pData[0].npix;
    for(size_t i=1;i<this->MDStruct.data_size;i++){
// the next cell starts from the the boundary of the previous one plus the number of pixels in the previous cell
        this->pData[i].chunk_location=this->pData[i-1].chunk_location+this->pData[i-1].npix;
    }
}

//***************************************************************************************
void
MDImageData::clear_class(void)
{
    if(pData){
        delete [] pData;
        pData = NULL;
        //MDStruct.data = NULL;
    }
    this->MDStruct.dimSize.assign(this->m_pMDGeometry->getNumDims(),0);
    this->MDStruct.dimStride.assign(this->m_pMDGeometry->getNumDims()+1,0);
    this->MDStruct.min_value.assign(this->m_pMDGeometry->getNumDims(), FLT_MAX);
    this->MDStruct.max_value.assign(this->m_pMDGeometry->getNumDims(),-FLT_MAX);


}

}
}
