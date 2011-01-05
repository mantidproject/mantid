#include "MDDataObjects/MD_FileTestDataGenerator.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"


namespace Mantid{
namespace MDDataObjects{
//
MD_FileTestDataGenerator::MD_FileTestDataGenerator(const char *file_name):
IMD_FileFormat(file_name),
mdImageSize(0),
nDataPoints(0)
{

}


//
void 
MD_FileTestDataGenerator::read_basis(Mantid::Geometry::MDGeometryBasis &basisGeometry)
{
   using namespace Mantid::Geometry;
    std::set<Geometry::MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("qtx", true, 0));
    basisDimensions.insert(MDBasisDimension("qty", true, 1));
    basisDimensions.insert(MDBasisDimension("qtz", true, 2));
    basisDimensions.insert(MDBasisDimension("ent", false,3));

    UnitCell cell;
    basisGeometry.init(basisDimensions,cell);
    // get_sqw_header should go here and define cell
}
//
void 
MD_FileTestDataGenerator::read_MDGeomDescription(Mantid::Geometry::MDGeometryDescription &dscrptn)
{
  
    unsigned int i,j,ic,i0;
    // horace tags come from basis, but as they have not been transferred between classes, 
    // they should be written with image too. 
    std::vector<std::string> Horace_tags(4);
    Horace_tags[0]="qtx";
    Horace_tags[1]="qty";
    Horace_tags[2]="qtz";
    Horace_tags[3]="ent";

    for(i=0;i<this->nDims;i++){
        dscrptn.pDimDescription(i)->data_shift = 0;
        dscrptn.pDimDescription(i)->data_scale= 1; //  Length of projection axes vectors in Ang^-1 or meV [row vector]
        dscrptn.pDimDescription(i)->AxisName = "Axis"+Horace_tags[i];
        // hardcoded here as we read the dimensions ID (tags and Horace does not do it)
        dscrptn.pDimDescription(i)->Tag    = Horace_tags[i];

        dscrptn.pDimDescription(i)->nBins = 1; // this sets this axis integrated
        dscrptn.pDimDescription(i)->cut_min = -1;
        dscrptn.pDimDescription(i)->cut_max =  1;

    }


}
   // read DND object data;
void 
MD_FileTestDataGenerator::read_MDImg_data(MDImage & mdd)
{
    size_t i;
    // get size and allocate read buffer;
    size_t nCells = mdd.getGeometry()->getGeometryExtend();
    std::vector<char> buff(nCells*8);

    // get access to the MD image array;
    MDDataObjects::MD_image_point *pImg_data =  mdd.get_pData();
 
    for(i=0;i<nCells;i++){
        pImg_data[i].s   = 2*i;
        pImg_data[i].err = 1000/i;
        pImg_data[i].npix= i;
    }
 

}
   
MDPointDescription 
MD_FileTestDataGenerator::read_pointDescriptions(void)const
{
    const char *HoraceDataTags[]={"qtx","qty","qtz","ent","St","errt","iRunIDt","iDetIDt","iEnt"};
    MDPointStructure  aPointDescr;
    std::vector<std::string> dataID(HoraceDataTags,HoraceDataTags+9);
    MDPointDescription defaultDescr(aPointDescr,dataID);
    return defaultDescr;
}
 //read whole pixels information in memory; usually impossible, then returns false;
//bool 
//MD_FileHoraceReader::read_pix(MDDataPoints & sqw)
//{
//	return false;
//}
//
size_t 
MD_FileTestDataGenerator::read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
{
//    size_t i,buffer_availible,cell_index;
//    size_t iCellRead(starting_cell);
//    // timing
//    time_t start,end;
//    time(&start);  //***********************************************>>>
//
//    const MD_image_point *pImgData = dnd.get_const_pData();
//    // buffer size provided;
////    buffer_availible = pix_buf.size()/(hbs);
//
//    // identify data extent fitting the buffer;
//    n_pix_in_buffer = 0;
//    for(i=starting_cell;i<selected_cells.size();i++){
//
//        cell_index      = selected_cells[i];
//        n_pix_in_buffer+=pImgData[cell_index].npix;
//
//        // end the loop earlier
//        if(n_pix_in_buffer>buffer_availible){ 
//            if(i==starting_cell){
////                pix_buf.resize(n_pix_in_buffer*hbs);
//                iCellRead=i;
//            }else{
//                iCellRead=i-1;
//                n_pix_in_buffer-=pImgData[cell_index].npix;
//            }
//            break;
//        }
//        iCellRead=i;
//    }
//    time(&end);     //***********************************************<<<<
//    f_log.debug()<<" cells preselected in: "<<difftime (end,start)<<" sec\n";;
//
//    // read data cell after cells indexes as provided;
//    
//    std::streamoff pixels_start;
//    size_t         block_size(0);
//    size_t         block_start(0);
//    size_t         data_buffer_size(0);
////	
    size_t ic=0;
//    size_t ic      = starting_cell;
//    size_t ic_next = ic+1;
//    if(ic_next>iCellRead)ic_next = iCellRead;
//    time(&start);  //***********************************************>>>
//
//
//	// read untill data buffer is full
//    while(true){
//
//        cell_index      = selected_cells[ic];
////        pixels_start  =   hbs*pImgData[cell_index].chunk_location; 
//
//        // optimisaion possible when cells are adjacent
////        block_size    = hbs*pImgData[cell_index].npix;
//
//        // if the next cell follows the current on HDD, we should read them together aggregating adjacent cells;
//        size_t next_block = pImgData[cell_index].chunk_location+pImgData[cell_index].npix;
//        size_t next_index = selected_cells[ic_next];
//        while(pImgData[next_index].chunk_location==next_block){
//            // block size grows and all other is auxiliary
////                block_size    += hbs*pImgData[next_index].npix;
//                ic = ic_next;
//                ic_next++;
//                if(ic_next > iCellRead)break;
//
//                cell_index = selected_cells[ic];
//                next_block = pImgData[cell_index].chunk_location+pImgData[cell_index].npix;
//                next_index = selected_cells[ic_next];
//            
//        }
//
//       
//  //      this->fileStreamHolder.seekg(pixels_start,std::ios::beg);
//    //    this->fileStreamHolder.read(&pix_buf[block_start],block_size);
//        block_start+=block_size;
//
//        // for single cell it is important to add but not to assign the next value, for more then one -- no difference
//        ic++;
//        ic_next++;
//        if(ic>iCellRead)break;
//        if(ic_next>iCellRead)ic_next=iCellRead;
//    }
//    time(&end);     //***********************************************<<<<
//    f_log.debug()<<" cells read in: "<<difftime (end,start)<<" sec\n";;
//    //
//    time(&start);  //*******
//
//	data_buffer_size = block_start; // latest block size has been already added
//	compact_hor_data(&pix_buf[0],data_buffer_size);
//    time(&end);   
 //   f_log.debug()<<" cells transformed in: "<<difftime (end,start)<<" sec\n";;
    // returns next cell to read if any or size of the selection
    return ic;
}

 /// get number of data pixels(points) contributing into the dataset;
unsigned long 
MD_FileTestDataGenerator::getNPix(void)
{
    return this->nDataPoints;
}





MD_FileTestDataGenerator::~MD_FileTestDataGenerator(void)
{
}
    // Function performing work previously in GOTO statement.

} // end namespaces
}
