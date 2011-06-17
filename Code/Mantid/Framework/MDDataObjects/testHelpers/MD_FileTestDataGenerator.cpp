#include "MDDataObjectsTestHelpers/MD_FileTestDataGenerator.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include <boost/lexical_cast.hpp>


namespace Mantid{
namespace MDDataObjects{
//
MD_FileTestDataGenerator::MD_FileTestDataGenerator(const Geometry::MDGeometryDescription *const pDescr):
IMD_FileFormat("test_memory_file.sqw"),
GeomDescription(pDescr), // if the pDescr is NULL, we would deploy default constructor;
mdImageSize(0),
nDataPoints(0),
pPointDescr(NULL)
{

	this->nDims = (unsigned int)GeomDescription.getNumDims();
	// the geometry description does not have any info about MDDPoint, so we assume here that points 
    // have nDims, 4 byte each, nDim integer indexes (4 bytes each) and 2 fields for signal and error (4 bytes each)
    this->sizeof_pixel=4*(nDims+2+nDims);
 
    // A prospective algorithm which would take indexes and rotation matrix for these indexes should be treated separately
    this->pTestDataSource = std::auto_ptr<MDDataTestHelper::MDDensityHomogeneous>(new MDDataTestHelper::MDPeakData(10,GeomDescription));

    this->nBins.resize(GeomDescription.getNumDims());
    this->nCells =    GeomDescription.getImageSize();
    this->nDataPoints= pDescr->nContributedPixels;
}
//
void 
MD_FileTestDataGenerator::read_basis(Mantid::Geometry::MDGeometryBasis &basisGeometry)
{
   using namespace Mantid::Geometry;
    std::set<Geometry::MDBasisDimension> basisDimensions;

    for(unsigned int i=0;i<nDims;i++){
        std::string dimID = GeomDescription.pDimDescription(i)->Tag;
        if(i<GeomDescription.getNumRecDims()){
            basisDimensions.insert(MDBasisDimension(dimID, true, i));
        }else{
            basisDimensions.insert(MDBasisDimension(dimID, false, i));
        }
    }
   // get_sqw_header should go here and define cell
	boost::shared_ptr<OrientedLattice> spCell= boost::shared_ptr<OrientedLattice>(new OrientedLattice(2.87,2.87,2.87));
   //
    basisGeometry.init(basisDimensions,spCell);
 

  
}
//
void 
MD_FileTestDataGenerator::read_MDGeomDescription(Mantid::Geometry::MDGeometryDescription &dscrptn)
{
 
    dscrptn = this->GeomDescription;

}
   // read DND object data;
void 
MD_FileTestDataGenerator::read_MDImg_data(MDImage & mdd)
{
    size_t i;
    // get size and allocate read buffer;
    size_t nCells = mdd.get_const_MDGeometry().getGeometryExtend();
    double S,Err;
    uint64_t nContributedCells;
    // get access to the MD image array;
    MDDataObjects::MD_image_point *pImg_data =  mdd.get_pData();
    //double step = double(this->nDims)/nCells;

    this->nDataPoints=0;
    for(i=0;i<nCells;i++){
        pTestDataSource->getMDImageCellData(i,S,Err,nContributedCells);
        pImg_data[i].s   = S;
        pImg_data[i].err = Err;
        pImg_data[i].npix= nContributedCells;
        this->nDataPoints+=nContributedCells;
    }
	mdd.setNpix(this->nDataPoints);

}
   
MDPointDescription 
MD_FileTestDataGenerator::read_pointDescriptions(void)const
{
    //TODO: there are currently no place, where this information is provided, except here. This place should be created.
    // nothing at the moment uses the colum indexes but when they would used -- something should be changed here
    unsigned int nResDimTags =2; //number of reciprocal dimension indexes are always 2, regadless of the number of reciprocal dimensions in dataset      
    unsigned int nIndexes    =this->GeomDescription.getNumDims()-this->GeomDescription.getNumRecDims()+nResDimTags;
    // full rec-dim data, signal and error + number of indexes
    unsigned int nHorDataTags=this->GeomDescription.getNumDims()+2+nIndexes;    

    std::vector<std::string> DataID(nHorDataTags);
    // first tags coinside with dimension tags;
    size_t ic(0);
    for(size_t i=0;i<this->GeomDescription.getNumDims();i++){
        DataID[i]=this->GeomDescription.pDimDescription(i)->Tag;
        ic++;
    }
    // two other tags are signal and error
     DataID[ic++]  ="St";
     DataID[ic++]  ="errt";
     // two next data tags always correspond to reciprocal dimension
     DataID[ic++]  ="iRunIDt";
     DataID[ic++]  ="iDetIDt";
     for(size_t i=0;i<nIndexes-2;i++){
         DataID[ic+i] = "Ind"+ boost::lexical_cast<std::string>(i);
     }
    MDPointStructure  aPointDescr;

    //TODO: This information has to correlate with what rebinning expects. Decorrelate!!!
    // let's make signals and errors float;
    aPointDescr.SignalLength = 4;
    // and indexes incompressed;
    aPointDescr.NumPixCompressionBits=0;
    // and indexes int32
    aPointDescr.DimIDlength  = 4;

 

    this->pPointDescr  = new MDPointDescription(aPointDescr,DataID);
    this->sizeof_pixel = this->pPointDescr->sizeofMDDPoint();
    if(this->sizeof_pixel != this->pTestDataSource->sizeofMDDataPoint()){
        throw(std::invalid_argument("number of pixels does not coinside with the values in test dataset"));
    }

    return *pPointDescr;
}
 //read whole pixels information in memory; if impossible returns false (or throws);
bool 
MD_FileTestDataGenerator::read_pix(MDDataPoints & sqw,bool nothrow)
{
    uint64_t nPixels = this->pTestDataSource->getNContribPixels();
    // return if exists or try to allocate if not a pointer to the buffer for the data; the data expressed in n-pixels
	std::vector<char> *dataBuffer = sqw.get_pBuffer();
    //get the buffed size
    size_t SpaceAvailible = sqw.get_pix_bufSize();
    if(nPixels>SpaceAvailible){
		sqw.set_file_based();
	    if(nothrow){
		    return false;
	    }else{
		    throw(std::runtime_error("can not place all pixels in memory"));
	    }
    }
    size_t nCells  = this->GeomDescription.getImageSize();
    size_t pix_size= this->pTestDataSource->sizeofMDDataPoint();
    size_t prev_cells_sizes(0);
    size_t MAX_CELL_SIZE =(~0);
    char *pDataBuffer = &(*dataBuffer)[0];
  
    for(size_t i=0;i<nCells;i++){
        // get suggested cell size in bytes;
        uint64_t cell_size = this->pTestDataSource->coarseCellCapacity(i)*pix_size;
        if(cell_size>MAX_CELL_SIZE){
            throw(std::invalid_argument("can not process such cell on this architecture"));
        }
        // store t
        size_t s_cell_size=size_t(cell_size);
        this->pTestDataSource->getMDDPointData(i,(pDataBuffer+prev_cells_sizes),SpaceAvailible-prev_cells_sizes,s_cell_size);
        prev_cells_sizes+=s_cell_size;
    }


   return true;
}
//
size_t 
MD_FileTestDataGenerator::read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
{
 
    size_t nSeelectedCells; 
    size_t buf_size = pix_buf.size();

    const Geometry::MDGeometry &CurrentGeom = dnd.get_const_MDGeometry();

    size_t pix_size= this->pTestDataSource->sizeofMDDataPoint();
    size_t prev_cells_sizes(0);
    size_t MAX_CELL_SIZE =(~0);
    for(size_t i=starting_cell;i<selected_cells.size();i++){
        size_t cell_ind =selected_cells[i];

        uint64_t cell_size = this->pTestDataSource->coarseCellCapacity(cell_ind)*pix_size;
        if(cell_size>MAX_CELL_SIZE){
            throw(std::invalid_argument("can not process such cell on this architecture"));
        }
        size_t s_cell_size=size_t(cell_size);
        if(cell_size+prev_cells_sizes>pix_buf.size()){
            nSeelectedCells = i+1;
            if(nSeelectedCells==starting_cell+1){
                pix_buf.resize(s_cell_size); // reallocate buffer sufficient to keep one cell
                buf_size    = s_cell_size;
            }else{
                break;  // return on buffer owerflow
            }
        }
        // otherwise, get the next cell
        this->pTestDataSource->getMDDPointData(cell_ind,(&pix_buf[0]+prev_cells_sizes),buf_size-prev_cells_sizes,s_cell_size);
        prev_cells_sizes+=s_cell_size*pix_size;
        nSeelectedCells  = i+1;
    }
    n_pix_in_buffer = prev_cells_sizes/pix_size;
    return nSeelectedCells; // returns next cell availible for reading
}

 /// get number of data pixels(points) contributing into the dataset;
uint64_t 
MD_FileTestDataGenerator::getNPix(void)
{
     return this->nDataPoints;
}

//std::vector<size_t> 
//MD_FileTestDataGenerator::cell_indexes(size_t cell_num,const std::vector<size_t> &dim_strides)
//{
//    size_t cur_index(cell_num);
//    size_t dim_index(0),i,ii;
//    size_t n_steps(dim_strides.size());
//    if(n_steps==0){
//        throw(std::invalid_argument("MD_FileTestDataGenerator::cell_indexes got zero size stride array"));
//    }
//
//    std::vector<size_t> rez(dim_strides.size(),0);
//    for(i=0;i<n_steps;i++){
//        ii = n_steps-1-i;
//        dim_index = cur_index/dim_strides[ii];
//        rez[ii] = dim_index;
//        cur_index -= dim_index*dim_strides[ii];
//    }
//    return rez;
//}
////


MD_FileTestDataGenerator::~MD_FileTestDataGenerator(void)
{
    if(pPointDescr)delete pPointDescr;
    pPointDescr=NULL;
  
}
    // Function performing work previously in GOTO statement.

} // end namespaces
}
