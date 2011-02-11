#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDImage.h"
#include "MantidGeometry/MDGeometry/MDCell.h"


namespace Mantid{
    namespace MDDataObjects{
//
    using namespace Mantid::API;
    using namespace Mantid::Kernel;
    using namespace Mantid::Geometry;


// logger for MD workspaces  
Kernel::Logger& MDImage::g_log =Kernel::Logger::get("MDWorkspaces");


void
MDImage::getPointData(std::vector<point3D> &image_points)const{
    std::vector<unsigned int> selection;
    if(this->pMDGeometry->getNumExpandedDims()>3){
        selection.assign(this->pMDGeometry->getNumExpandedDims()-3,0);
    }else{
        selection.resize(0);
    }
    this->getPointData(selection,image_points);

}
//
void
MDImage::getPointData(const std::vector<unsigned int> &selection,std::vector<point3D> &image_points)const
{
    unsigned int selection_size  =  (unsigned int )selection.size();
    if(selection_size >this->pMDGeometry->getNumExpandedDims()){
        throw(std::invalid_argument("MDImaegData::getPointData: selection-> attempting to select more dimensions then there are expanded dimensions"));
    }
    unsigned int i,j,k,iMin,jMin,kMin,iMax,jMax,kMax,isel;
    size_t   base(0);
    const IMDDimension *pDim;

    // calculate shift for all selected dimensions;
    int the_expanded_dim= this->pMDGeometry->getNumExpandedDims()-1;
    for(int iii=selection_size-1;iii>=0;iii--){
		pDim = this->pMDGeometry->get_constDimension(the_expanded_dim).get();
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
        iMax=this->pMDGeometry->get_constDimension(0)->getNBins();
        rez_size = iMax;
    }else{
        iMin=selection[current_selected_dimension];
        iMax=selection[current_selected_dimension]+1;
        rez_size = 1;
        current_selected_dimension++;
    }
    std::vector<double> xx;
	pDim = this->pMDGeometry->get_constDimension(0).get();
    pDim->getAxisPoints(xx);


    if(the_expanded_dim>0){
        jMin=0;
        jMax=this->pMDGeometry->get_constDimension(1)->getNBins();
        rez_size *= jMax;
    }else{
        jMin=selection[current_selected_dimension];
        jMax=selection[current_selected_dimension]+1;
        current_selected_dimension++;
    }
    std::vector<double> yy;
    (this->pMDGeometry->get_constDimension(1))->getAxisPoints(yy);

    if(the_expanded_dim>1){
        kMin=0;
        kMax=this->pMDGeometry->get_constDimension(2)->getNBins();
        rez_size *= kMax;
    }else{
        kMin=selection[current_selected_dimension];
        kMax=selection[current_selected_dimension]+1;
        current_selected_dimension++;
    }
    std::vector<double> zz;
    (this->pMDGeometry->get_constDimension(2))->getAxisPoints(zz);

// build full array of 3D points
	const MD_image_point  *const pData = this->get_const_pData();
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
                image_points[ic]  = pData[index];
                ic++;
            }
        }
    }

}
//
//****************************************
//
MD_image_point *
MDImage::get_pData(void)
{
    if(MD_IMG_array.data){
        return MD_IMG_array.data;
    }else{
		throw(std::runtime_error("Data memory for Multidimensional dataset has not been allocated"));
    }
}
MD_image_point const*
MDImage::get_const_pData(void)const
{
    if(MD_IMG_array.data){
        return MD_IMG_array.data;
    }else{
		throw(std::runtime_error("Data memory for Multidimensional dataset has not been allocated"));
    }
}
//
bool 
MDImage::is_initialized(void)const
{
	if(!pMDGeometry.get()||!this->MD_IMG_array.data)return false;
	return true;
}

//*******************************************************************************************************
void
MDImage::reshape_geometry(const Geometry::MDGeometryDescription &transf)
{

   // all paxis in the transformation matrix have to be defined properly and in accordance with the transformation data.
   // also sets the the dimension limits and object limits as the limits from transf class
   this->pMDGeometry->initialize(transf);
   this->set_imgArray_shape();
 
 
}
// 
void 
MDImage::set_imgArray_shape()
{
	unsigned int nDims = this->pMDGeometry->getNumDims();
	size_t   i;

    this->MD_IMG_array.dimSize.assign(nDims,0);
    this->MD_IMG_array.dimStride.assign(MAX_MD_DIMS_POSSIBLE+1,0);

    const IMDDimension *pDim;
    this->MD_IMG_array.data_size    = 1;
    size_t  stride(1);
    for(i=0;i<this->pMDGeometry->getNumDims();i++){
        pDim                             = (this->pMDGeometry->get_constDimension(i)).get();
        stride                           = pDim->getStride();
        this->MD_IMG_array.dimSize[i]    =  pDim->getNBins();
        this->MD_IMG_array.data_size     *= this->MD_IMG_array.dimSize[i];

        this->MD_IMG_array.dimStride[i]  = stride;

    }
	if(this->MD_IMG_array.data_size!=this->pMDGeometry->getGeometryExtend()){
		g_log.error()<<" size of MD image array = " << MD_IMG_array.data_size<<" and differs from the size, described by MDGeometry = "<<pMDGeometry->getGeometryExtend()<<std::endl;
		throw(std::logic_error(" MD geometry and MD_image_Data are not synchroneous any more. BUGGG!!! "));
	}
	this->MD_IMG_array.npixSum = 0;

    this->nd2 =MD_IMG_array.dimStride[1];
    this->nd3 =MD_IMG_array.dimStride[2];
    this->nd4 =MD_IMG_array.dimStride[3];
    this->nd5 =MD_IMG_array.dimStride[4];
    this->nd6 =MD_IMG_array.dimStride[5];
    this->nd7 =MD_IMG_array.dimStride[6];
    this->nd8 =MD_IMG_array.dimStride[7];
    this->nd9 =MD_IMG_array.dimStride[8];
    this->nd10=MD_IMG_array.dimStride[9];
    this->nd11=MD_IMG_array.dimStride[10];
}
//
void
MDImage::initialize(const MDGeometryDescription &transf,const MDGeometryBasis *const pBasis)
{

	if(!this->pMDGeometry.get()){
		if(!pBasis){
			g_log.error()<<" MDImage::initialize: construction geometry from its description without the basis is imkpossible\n";
			throw(std::invalid_argument("Constructing geometry from geometry description without the Geometry basis is impossible"));
		}else{
			this->pMDGeometry = std::auto_ptr<MDGeometry>(new MDGeometry(*pBasis,transf));
		}
	}
// initiate initial dimensions
	size_t ImgSize = transf.getImageSize();
	// reshape geometry and set the data size requested for the image in MD_img_data structure;
	this->reshape_geometry(transf);
	if(ImgSize!=MD_IMG_array.data_size){
		g_log.error()<<"MDImage::initialize: logical error as array data size="<<MD_IMG_array.data_size<< "and differs from the value requested by transformation"<<ImgSize<<std::endl;
		throw(std::logic_error("MDImage::initialize: MD image geometry and MD image data become  non-synchronous"));
	}

  // do we have enough existing memory for the data?
   if(!this->MD_IMG_array.data || ImgSize>this->MD_IMG_array.data_array_size){
       this->clear_class();
	// allocate main data array;
	   this->alloc_image_data();
   //
	   this->set_imgArray_shape();
   }else{
	    MD_image_point *pData = MD_IMG_array.data;
		for(unsigned long j=0;j<MD_IMG_array.data_size;j++){
			pData[j].s   =0;
			pData[j].err =0;
			pData[j].npix=0;
		}
		MD_IMG_array.npixSum=0;
   }
 

}
// 
void 
MDImage::validateNPix(void)
{
	uint64_t pix_sum(0);
    MD_image_point *pData = MD_IMG_array.data;
	for(size_t j=0;j<MD_IMG_array.data_size;j++){
			pix_sum+=pData[j].npix;
	}
	if(pix_sum!=MD_IMG_array.npixSum){
		uint64_t old_number = MD_IMG_array.npixSum;
		MD_IMG_array.npixSum = pix_sum;
		g_log.warning()<<" the control number of pixels specified in the pixel array = "<<old_number<<" is not equal to the actual number of pixels referred by the cells: "<<pix_sum<<std::endl;
		throw(std::invalid_argument("sum of pixels numbers is not consistent with the control sum; fixed for now but suggests an error in algorithm produced this image"));
	}

}
//
MDImage::MDImage(Mantid::Geometry::MDGeometry* pGeometry): 
pMDGeometry(std::auto_ptr<Mantid::Geometry::MDGeometry>(pGeometry)),
nd2(0),nd3(0),nd4(0),nd5(0),nd6(0),nd7(0),nd8(0),nd9(0),nd10(0),nd11(0)
{
  // empty initialisation; currently not supported as will throw later;
  if(!pGeometry)return;

  int nDims = pMDGeometry->getNumDims();
  if( nDims >MAX_MD_DIMS_POSSIBLE){
    throw(std::invalid_argument("MDData::MDData number of dimensions exceeds the possible value"));
  }
  MDGeometryDescription descr(*pGeometry);
  this->initialize(descr);

}
MDImage::MDImage(const Geometry::MDGeometryDescription &Description, const Geometry::MDGeometryBasis & Basis):
pMDGeometry(NULL),
nd2(0),nd3(0),nd4(0),nd5(0),nd6(0),nd7(0),nd8(0),nd9(0),nd10(0),nd11(0)
{
	this->initialize(Description,&Basis);
}
//
void
MDImage::alloc_image_data()
{
	size_t ImgSize     = this->pMDGeometry->getGeometryExtend();
	try{
     	MD_IMG_array.data = new MD_image_point[ImgSize];
	}catch(std::bad_alloc &err){
		g_log.error()<<" can not allocate memory for multidimensional image of "<<ImgSize<<" points\n";
		throw(std::bad_alloc("Can not allocate memory for Multidimensional image "));
	}
    MD_image_point *pData        = MD_IMG_array.data;
	MD_IMG_array.data_array_size = ImgSize;
	MD_IMG_array.data_size       = ImgSize;



}
//
MDImage::~MDImage()
{
    this->clear_class();
}
//

//***************************************************************************************
void
MDImage::clear_class(void)
{
    if(MD_IMG_array.data){
        delete [] MD_IMG_array.data;
        MD_IMG_array.data = NULL;
    }
	if(this->pMDGeometry.get()){
		this->MD_IMG_array.dimSize.assign(this->pMDGeometry->getNumDims(),0);
		this->MD_IMG_array.dimStride.assign(this->pMDGeometry->getNumDims()+1,0);
		this->MD_IMG_array.min_value.assign(this->pMDGeometry->getNumDims(), FLT_MAX);
		this->MD_IMG_array.max_value.assign(this->pMDGeometry->getNumDims(),-FLT_MAX);
	}
	MD_IMG_array.data_array_size=0;
	MD_IMG_array.data_size     = 0;
	MD_IMG_array.npixSum       = 0;

}

}
}
