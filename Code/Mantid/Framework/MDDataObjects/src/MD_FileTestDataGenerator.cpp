#include "MDDataObjects/MD_FileTestDataGenerator.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"


namespace Mantid{
namespace MDDataObjects{
//
MD_FileTestDataGenerator::MD_FileTestDataGenerator(const char *file_name):
IMD_FileFormat(file_name),
mdImageSize(0),
nDataPoints(0),
pPointDescr(NULL)
{
    this->nDims = 4;
    this->sizeof_pixel=4*(nDims+2+nDims);

    this->dat_sig_fields= new float[nDims+2];
    this->ind_fields    = new uint32_t[nDims];
    this->nBins.resize(nDims,50);
    this->nCells = 1;
    for(size_t i=0;i<nBins.size();i++){
        nCells *=nBins[i];
    }
}
//
void 
MD_FileTestDataGenerator::read_basis(Mantid::Geometry::MDGeometryBasis &basisGeometry)
{
   using namespace Mantid::Geometry;
    std::set<Geometry::MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("qxt", true, 0));
    basisDimensions.insert(MDBasisDimension("qyt", true, 1));
    basisDimensions.insert(MDBasisDimension("qzt", true, 2));
    basisDimensions.insert(MDBasisDimension("ent", false,3));

    UnitCell cell;
    basisGeometry.init(basisDimensions,cell);
    // get_sqw_header should go here and define cell

  
}
//
void 
MD_FileTestDataGenerator::read_MDGeomDescription(Mantid::Geometry::MDGeometryDescription &dscrptn)
{
  
    unsigned int i;
    // horace tags come from basis, but as they have not been transferred between classes, 
    // they should be written with image too. 
    std::vector<std::string> Horace_tags(4);
    Horace_tags[0]="qxt";
    Horace_tags[1]="qyt";
    Horace_tags[2]="qzt";
    Horace_tags[3]="ent";

    for(i=0;i<this->nDims;i++){
        dscrptn.pDimDescription(i)->data_shift = 0;
        dscrptn.pDimDescription(i)->data_scale= 1; //  Length of projection axes vectors in Ang^-1 or meV [row vector]
        dscrptn.pDimDescription(i)->AxisName = "Axis_"+Horace_tags[i];
        // hardcoded here as we read the dimensions ID (tags and Horace does not do it)
        dscrptn.pDimDescription(i)->Tag    = Horace_tags[i];

        dscrptn.pDimDescription(i)->nBins = this->nBins[i]; // one sets this axis integrated
        dscrptn.pDimDescription(i)->cut_min = -1;
        dscrptn.pDimDescription(i)->cut_max = this->nBins[i]-1;

    }


}
   // read DND object data;
void 
MD_FileTestDataGenerator::read_MDImg_data(MDImage & mdd)
{
    size_t i;
    // get size and allocate read buffer;
    size_t nCells = mdd.getGeometry()->getGeometryExtend();
   

    // get access to the MD image array;
    MDDataObjects::MD_image_point *pImg_data =  mdd.get_pData();
    double step = double(this->nDims)/nCells;
    uint64_t  sanity_check(0);
    for(i=0;i<nCells;i++){
        pImg_data[i].s   = i;
        pImg_data[i].err = 2/double(i+1);
        pImg_data[i].npix= i+1;
        sanity_check+=i+1;
    }
    this->nDataPoints = ((uint64_t)(nCells))*(nCells+1)/2;

}
   
MDPointDescription 
MD_FileTestDataGenerator::read_pointDescriptions(void)const
{
    const char *HoraceDataTags[]={"qxt","qyt","qzt","ent","St","errt","iRunIDt","iDetIDt","iEnt"};
    MDPointStructure  aPointDescr;

    //TODO: This information has to correlate with what rebinning expects. Decorrelate!!!
    // let's make signals and errors float;
    aPointDescr.SignalLength = 4;
    // and indexes incompressed;
    aPointDescr.NumPixCompressionBits=0;
    // and indexes int32
    aPointDescr.DimIDlength  = 4;

    std::vector<std::string> dataID(HoraceDataTags,HoraceDataTags+9);

    this->pPointDescr  = new MDPointDescription(aPointDescr,dataID);
    this->sizeof_pixel = this->pPointDescr->sizeofMDDPoint();

    return *pPointDescr;
}
 //read whole pixels information in memory; usually impossible, then returns false;
bool 
MD_FileTestDataGenerator::read_pix(MDDataPoints & sqw)
{
    sqw.set_file_based();
	return false;
}
//
size_t 
MD_FileTestDataGenerator::read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
{
// pixels data generated assiuming 50^nDim lattice;
    size_t ic(starting_cell),j;
    unsigned int idim;
    unsigned long max_data_size;
    std::vector<size_t> dim_strides(this->nDims);
   

    const Geometry::MDGeometry *pCurrentGeom = dnd.getGeometry();
    
    // data points;
    std::vector<std::vector<float> > dimPoints(this->nDims);
    // get dimensions in the order they are defined in the grid
    std::vector<boost::shared_ptr<Geometry::IMDDimension> > spDims = pCurrentGeom->getDimensions();
  
    // obtain dimensions and dimensions coordinates;
    for(idim=0;idim<this->nDims;idim++){

        const Geometry::IMDDimension *pDim = spDims[idim].get();
        dimPoints[idim].resize(this->nBins[idim]);
        double min  = pDim->getMinimum();
        double step = (pDim->getMaximum()-min)/this->nBins[idim];
        //min+=0.5*step;<- we will step through dimensions in a look well below this
        dim_strides[idim] = pDim->getStride();
        if(min!=0){
            min *= (1+FLT_EPSILON*min/fabs(min));
        }
        for(j=0;j<nBins[idim];j++){
            dimPoints[idim][j]=(float)(min+j*step);
        }
    }
 
    // if data buffer is unsufficient even for one block of pixels, increase it (very inefficient)
    // number of pixels in test data cell equal the cell number (+1 ?) 
    size_t n_pix_in_block = selected_cells[starting_cell]+1;
    if(pix_buf.size()<sizeof_pixel*n_pix_in_block){
        pix_buf.resize(sizeof_pixel*n_pix_in_block);
        max_data_size = n_pix_in_block;
    }else{
        max_data_size = pix_buf.size()/sizeof_pixel;
    }
    n_pix_in_buffer=0;

    // initate packer (should be more generic ways of doing this)
    std::auto_ptr<MDDataPointEqual<float,uint32_t,float> > pPacker= std::auto_ptr<MDDataPointEqual<float,uint32_t,float> >
        (new MDDataPointEqual<float,uint32_t,float>(&pix_buf[0],*pPointDescr));



    // no need in this as will assume number of pixels equal to number of cells
    //const MD_image_point *pData = dnd.get_const_pData();
    size_t nCells = dnd.get_MDImgData().data_size;


    std::vector<float> cell_step(this->nDims);
    // fill pixels according to the algorithm when each cell has n-pixels in it equal to the number of cell+1
    // pix coordinates 
    size_t nSeelectedCells = selected_cells.size();
    for(ic=starting_cell;ic<nSeelectedCells;ic++){
        //size_t n_pix = pData[selected_cells[ic]].npix;
        std::vector<size_t> cell_ind = this->cell_indexes(selected_cells[ic],dim_strides);
        size_t n_pix  = selected_cells[ic]+1;
  
        for(idim=0;idim<this->nDims;idim++){
            const Geometry::IMDDimension *pDim = spDims[idim].get();
            double min  = pDim->getMinimum();
            cell_step[idim] = (float)(pDim->getMaximum()-min)/this->nBins[idim]/n_pix;
         }
   

        // not enough memory for next data chunk;
        if(n_pix_in_buffer+n_pix>max_data_size)return ic;
        // fill pixels placing points along the biggest diagonal of the multidimensional cell;
        for(j=0;j<n_pix;j++){
            for(idim=0;idim<this->nDims;idim++){
                dat_sig_fields[idim] = (float)(dimPoints[idim])[cell_ind[idim]]+ cell_step[idim]*j;
            }
            // signal 
            dat_sig_fields[nDims]  =(float)selected_cells[ic]+1;
            // error
            dat_sig_fields[nDims+1]=2;

     
            pPacker->setData(n_pix_in_buffer,dat_sig_fields,ind_fields);
            n_pix_in_buffer++;
        }
    }
    return nSeelectedCells;
}

 /// get number of data pixels(points) contributing into the dataset;
uint64_t 
MD_FileTestDataGenerator::getNPix(void)
{
    if(this->nDataPoints == 0){
        this->nDataPoints = nCells*(nCells+1)/2;
    }
    return this->nDataPoints;
}

std::vector<size_t> 
MD_FileTestDataGenerator::cell_indexes(size_t cell_num,const std::vector<size_t> &dim_strides)
{
    size_t cur_index(cell_num);
    size_t dim_index(0),i,ii;
    size_t n_steps(dim_strides.size());
    if(n_steps==0){
        throw(std::invalid_argument("MD_FileTestDataGenerator::cell_indexes got zero size stride array"));
    }

    std::vector<size_t> rez(dim_strides.size(),0);
    for(i=0;i<n_steps;i++){
        ii = n_steps-1-i;
        dim_index = cur_index/dim_strides[ii];
        rez[ii] = dim_index;
        cur_index -= dim_index*dim_strides[ii];
    }
    return rez;
}
//


MD_FileTestDataGenerator::~MD_FileTestDataGenerator(void)
{
    if(pPointDescr)delete pPointDescr;
    pPointDescr=NULL;
    delete [] dat_sig_fields;
    delete [] ind_fields;
}
    // Function performing work previously in GOTO statement.

} // end namespaces
}
