#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDData.h"
#include "MDDataObjects/MD_File_hdfMatlab.h"
#include "MDDataObjects/MD_File_hdfV1.h"
#include "MantidGeometry/MDGeometry/MDCell.h"

namespace Mantid{
    namespace MDDataObjects{
//
    using namespace Mantid::API;
    using namespace Mantid::Kernel;
    
    
    unsigned int MDData::getNPoints() const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::IMDDimension& MDData::getDimension(std::string id) const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDPoint * MDData::getPoint(long index) const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDData::getCell(long dim1Increment) const 
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDData::getCell(long dim1Increment, long dim2Increment) const 
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDData::getCell(long dim1Increment, long dim2Increment, long dim3Increment)  const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDData::getCell(long dim1Increment, long dim2Increment, long dim3Increment, long dim4Increment)  const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDData::getCell(...)  const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::IMDDimension& MDData::getXDimension() const
    {
      return MDGeometry::getXDimension();
    }

    Mantid::Geometry::IMDDimension& MDData::getYDimension() const
    {
      return MDGeometry::getYDimension();
    }

    Mantid::Geometry::IMDDimension& MDData::getZDimension() const
    {
      return MDGeometry::getZDimension();
    }

     Mantid::Geometry::IMDDimension& MDData::gettDimension() const
    {
      return MDGeometry::getTDimension();
    }


    Kernel::Logger& MDData::g_log =Kernel::Logger::get("MDWorkspaces");
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
                image_points[ic].X()=xx[i];                
                image_points[ic].Y()=yy[j];                
                image_points[ic].Z()=zz[k];                
                image_points[ic]  = this->data[index];
                    
                ic++;
            }
        }
    }

}

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
MD_image_point * 
MDData::get_pData(void)
{
    if(data){
        return data;
    }else{
        throw(std::bad_alloc("Data memory for Multidimensional dataset has not been allocated"));
    }
}
MD_image_point const* 
MDData::get_const_pData(void)const
{
    if(data){
        return data;
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
MDData::reshape_geometry(const MDGeometryDescription &transf)
{
   unsigned int i;

   // all paxis in the transformation matrix have to be defined properly and in accordance with the transformation data. 
   this->reinit_Geometry(transf);

   // set the this object limits as the limits from transf class
   this->setRanges(transf);

   this->dimSizes.assign(this->n_total_dim,0);
   this->dimStride.assign(MAX_MD_DIMS_POSSIBLE+1,0);

    MDDimension *pDim;
    this->dimStride[0] = 1;
    this->data_size    = 1;
    size_t  stride(1);
    for(i=0;i<this->n_total_dim;i++){
        pDim                 = this->getDimension(i);
        stride               = pDim->getStride();
        this->dimSizes[i]    = pDim->getNBins();
        this->data_size     *= this->dimSizes[i];

        if(stride != this->dimStride[i]){
            throw(std::runtime_error(" logical error -- MD geometry and MD data are not consitent"));
        }
        this->dimStride[i+1] = this->data_size;

    }

    this->nd2 =dimStride[0];
    this->nd3 =dimStride[1];
    this->nd4 =dimStride[2];
    this->nd5 =dimStride[3];
    this->nd6 =dimStride[4];
    this->nd7 =dimStride[5];
    this->nd8 =dimStride[6];
    this->nd9 =dimStride[7];
    this->nd10=dimStride[8];
    this->nd11=dimStride[9];

    return data_size;
}
void 
MDData::alloc_mdd_arrays(const MDGeometryDescription &transf)
{

// initiate initial dimensions
   if(this->data){
       this->clear_class();
   }
   this->data_size=this->reshape_geometry(transf);



// allocate main data array;
    data = new MD_image_point[data_size];
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
    if(nDims>MAX_MD_DIMS_POSSIBLE){
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

std::vector<size_t> 
MDData::getStrides(void)const
{
  unsigned int nDims = this->getNumDims();
  std::vector<size_t> strides(nDims,0);
  for(unsigned int i=0;i<nDims;i++){
      strides[i] = this->getDimension(i)->getStride();
  }
  return strides;

}
//
void 
MDData::identify_SP_points_locations()
{
    // and calculate cells location for pixels;
    this->data[0].chunk_location=0;

    // counter for the number of retatined pixels;
    size_t nPix = this->data[0].npix;
    for(size_t i=1;i<this->data_size;i++){   
// the next cell starts from the the boundary of the previous one plus the number of pixels in the previous cell
        this->data[i].chunk_location=this->data[i-1].chunk_location+this->data[i-1].npix; 
    }
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
