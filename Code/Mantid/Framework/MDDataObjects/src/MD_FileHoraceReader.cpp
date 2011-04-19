#include "MDDataObjects/MD_FileHoraceReader.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"


namespace Mantid{
namespace MDDataObjects{
namespace HoraceReader{
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
// here we need float to be 32 bytes long as it is what is embedded in Horace file format;
// if float on a computer is different from 32 bit, the transformations would be meaningles;
typedef float float32;
//
MD_FileHoraceReader::MD_FileHoraceReader(const char *file_name):
IMD_FileFormat(file_name),
positions(),
mdImageSize(0),
nDataPoints(0),
hbs(9*4)
{
    if(sizeof(float32)!=4){
        f_log.error()<<"MD_FileHoraceReader is not defined on a computer with non-32-bit float\n";
        throw(std::bad_cast());
    }

    std::vector<char> data_buffer;

    fileStreamHolder.open(file_name,std::ios::binary);
    if(fileStreamHolder.bad()){
        f_log.error()<<"MD_FileHoraceReader:: error opening existing Horace file "<<File_name<<", which was identified as Horace\n";
        throw(Kernel::Exception::FileError("Error opening existing Horace file",File_name));
    }
    // go to the start of the data fields
    fileStreamHolder.seekg(positions.if_sqw_start);
    data_buffer.resize(3*4);

    fileStreamHolder.read(&data_buffer[0],2*4);
    if(fileStreamHolder.bad()){
        f_log.error()<<"MD_FileHoraceReader:: error reading dnd/sqw and nDims from the file "<<File_name<<std::endl; 
        throw(Kernel::Exception::FileError("Error opening existing Horace file",File_name));
    }
    int isSQW = *((uint32_t*)(&data_buffer[0]));
    if(!isSQW){
        f_log.error()<<" Mantid currently does not support Horace DND files and the file "<<File_name<<" is identified as DND file\n";
        throw(Exception::FileError("File has not been identified as Horace SQW file",File_name));
    }

    this->nDims = *((uint32_t*)(&data_buffer[4]));
    if(nDims != 4){
        f_log.error()<<"MD_FileHoraceReader:: does not support"<<nDims<<" Dimensions, should be 4\n";
        throw(Kernel::Exception::FileError("Wrong data in the Horace file",File_name));
    }

    parse_sqw_main_header();

    // go through all component headers and read them (or calculate their length)
    std::streamoff next_position = positions.component_headers_starts[0];
    size_t nFiles        = positions.component_headers_starts.size();
    for(size_t i=0;i<nFiles;i++){
        positions.component_headers_starts[i] = next_position;
        next_position = parse_component_header(next_position);
    }
    positions.detectors_start = next_position;
    // get detectors
    positions.data_start      = parse_sqw_detpar(positions.detectors_start);
    // calculate all other data fields locations;
    parse_data_locations(positions.data_start);
}


//
void 
MD_FileHoraceReader::read_basis(Mantid::Geometry::MDGeometryBasis &basisGeometry)
{
   using namespace Mantid::Geometry;
    std::set<Geometry::MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("qx", true, 0));
    basisDimensions.insert(MDBasisDimension("qy", true, 1));
    basisDimensions.insert(MDBasisDimension("qz", true, 2));
    basisDimensions.insert(MDBasisDimension("en", false,3));

    std::vector<char> buf(4*(3+3));
    this->fileStreamHolder.seekg(this->positions.geom_start,std::ios::beg);
    this->fileStreamHolder.read(&buf[0],buf.size());

	double a = (double)(*((float *)(&buf[0])));
	double b = (double)(*((float *)(&buf[4])));
	double c = (double)(*((float *)(&buf[8])));
	double aa = (double)(*((float *)(&buf[12])));
	double bb = (double)(*((float *)(&buf[16])));
	double cc = (double)(*((float *)(&buf[20])));


	// here should be the operation to read the cell and goiniometer
	boost::shared_ptr<UnitCell> spCell= boost::shared_ptr<UnitCell>(new UnitCell(a,b,c,aa,bb,cc));

	basisGeometry.init(basisDimensions,spCell);
    // get_sqw_header should go here and define cell
}
//
void 
MD_FileHoraceReader::read_MDGeomDescription(Mantid::Geometry::MDGeometryDescription &dscrptn)
{
    DimensionDescription *pDimDescr;
    // the description has to already have proper shape of dimensions
    if(dscrptn.getNumDims()!=this->nDims||dscrptn.getNumRecDims()!=3){
        f_log.error()<<"read geometry description should receive correct inital object with proper number of orthogonal and reciprocal dimensions\n";
        f_log.error()<<"expeted to obtain"<<this->nDims<<" total and 3 reciprocal dimensions\n";
        f_log.error()<<"obtained:        "<<dscrptn.getNumDims()<<" total and "<<dscrptn.getNumRecDims()<<" reciprocal dimensions\n";
        throw(std::invalid_argument("read_MDGeomDescription for Horace data: input/output object has not been shaped properly"));
    }

    std::vector<char> buf(4*(3+3+4+16+4+2));
    unsigned int i,j,ic,i0;
    // horace tags come from basis, but as they have not been transferred between classes, 
    // they should be written with image too. 
    std::vector<std::string> Horace_tags(4);
    Horace_tags[0]="qx";
    Horace_tags[1]="qy";
    Horace_tags[2]="qz";
    Horace_tags[3]="en";

    this->fileStreamHolder.seekg(this->positions.geom_start,std::ios::beg);

/*
[data.uoffset, count, ok, mess] = fread_catch(fid,[4,1],'float32'); if ~all(ok); return; end;
[n, count, ok, mess] = fread_catch(fid,2,'int32'); if ~all(ok); return; end;
*/
    this->fileStreamHolder.read(&buf[0],buf.size());
    // skip allat and adlngldef
    i0 = 4*(3+3) ; 
    for(i=0;i<this->nDims;i++){
        double val = (double)*((float32*)(&buf[i0+i*4]));
        dscrptn.pDimDescription(i)->data_shift = val;
    }
    //TODO: how to use it in our framework?
    std::vector<double> u_to_Rlu(this->nDims*this->nDims);
    i0 += this->nDims*4;
// [data.u_to_rlu, count, ok, mess] = fread_catch(fid,[4,4],'float32'); if ~all(ok); return; end;
    ic = 0;
    for(i=0;i<this->nDims;i++){
        for(j=0;j<this->nDims;j++){
            u_to_Rlu[ic]=(double)*((float32*)(&buf[i0+4*(i*4+j)]));
            ic++;
        }
    }
    i0 += ic*4;
// [data.ulen, count, ok, mess] = fread_catch(fid,[1,4],'float32'); if ~all(ok); return; end;
//  Length of projection axes vectors in Ang^-1 or meV [row vector]
    for(i=0;i<this->nDims;i++){
        dscrptn.pDimDescription(i)->data_scale= *((float32*)(&buf[i0+i*4]));
    }

    // axis labels size 
    i0 += nDims*4;
    unsigned int nRows = *((uint32_t*)(&buf[i0]));
    unsigned int nCols = *((uint32_t*)(&buf[i0+4]));


    // read axis labelsg
    buf.resize(nRows*nCols);
   // [ulabel, count, ok, mess] = fread_catch(fid,[n(1),n(2)],'*char'); if ~all(ok); return; end;

    this->fileStreamHolder.read(&buf[0],buf.size());

    //data.ulabel=cellstr(ulabel)';
    std::string name;
    char symb;
    name.resize(nCols);
    for(i=0;i<nRows;i++){
        for(j=0;j<nCols;j++){
            symb   =buf[i+j*nRows]; 
            name[j] =symb;  // should be trim here;
        }
        dscrptn.pDimDescription(i)->AxisName = name;
        // hardcoded here as we read the dimensions ID (tags and Horace does not do it)
        dscrptn.pDimDescription(i)->Tag    = Horace_tags[i];
    }

// pax dax fax...
    // the order of the id-s in this array has to correspond to the numbers of axis, specified in Horace
    std::vector<std::string> dimID = dscrptn.getDimensionsTags();

    // resize for iax and npax;
    buf.resize(4*4*3);
    this->fileStreamHolder.read(&buf[0],4);

    unsigned int npax =  *((uint32_t*)(&buf[0]));
    unsigned int niax = 4-npax;

    if(niax>0){
   //    [data.iax, count, ok, mess] = fread_catch(fid,[1,niax],'int32'); if ~all(ok); return; end;
   //   [data.iint, count, ok, mess] = fread_catch(fid,[2,niax],'float32'); if ~all(ok); return; end;
        DimensionDescription *pDimDescr;

        this->fileStreamHolder.read(&buf[0],buf.size());
        int i_axis_index;
        for(i=0;i<niax;i++){
            i_axis_index = *((uint32_t*)(&buf[i*4]));
            pDimDescr  = dscrptn.pDimDescription(dimID[i_axis_index]);

            pDimDescr->nBins = 1; // this sets this axis integrated
            pDimDescr->cut_min = *((float32*)(&buf[4*(niax+i*2)])); // min integration value
            pDimDescr->cut_max = *((float32*)(&buf[4*(niax+i*2+1)])); // max integration value
        }

    }
    // processing projection axis;
    if(npax>0){
      //[data.pax, count, ok, mess] = fread_catch(fid,[1,npax],'int32'); if ~all(ok); return; end;
        this->fileStreamHolder.read(&buf[0],4*npax);
        //[np,count,ok,mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
        //[data.p{i},count,ok,mess] = fread_catch(fid,np,'float32'); if ~all(ok); return; end;

        for(i=0;i<npax;i++){
            // matlab indexes started with 1;
            std::string current_tag = dimID[*((uint32_t*)(&buf[i*4]))-1];

            std::vector<char> axis_buffer(51*4);
            this->fileStreamHolder.read(&axis_buffer[0],4);
            unsigned int  nAxisPoints = *((uint32_t*)(&axis_buffer[0]));
            if(axis_buffer.size()<nAxisPoints*4)axis_buffer.resize(nAxisPoints*4);
            this->fileStreamHolder.read(&axis_buffer[0],4*nAxisPoints);

            pDimDescr = dscrptn.pDimDescription(current_tag);
            // this do not describes axis on an irregular grid. 
            //TODO: implement irregular axis and put check if the grid is regular or not. 
            pDimDescr->nBins   = nAxisPoints-1; // this sets num-bins
            pDimDescr->cut_min = *((float32*)(&axis_buffer[4*(0)])); // min axis value
            pDimDescr->cut_max = *((float32*)(&axis_buffer[4*(nAxisPoints-1)])); // max axis value


        }
    }
    //display axis are not supported, as data in Horace IMG are not arranged according to display axis. (or are they?);
    //TODO: check this.
  //[data.dax, count, ok, mess] = fread_catch(fid,[1,npax],'int32'); if ~all(ok); return; end;


}
   // read DND object data;
void 
MD_FileHoraceReader::read_MDImg_data(MDImage & mdd)
{
    size_t i;
    // get size and allocate read buffer;
    size_t nCells = mdd.getGeometry()->getGeometryExtend();
    std::vector<char> buff(nCells*8);

    // get access to the MD image array;
    MDDataObjects::MD_image_point *pImg_data =  mdd.get_pData();
    if(!pImg_data){
        f_log.error()<<"read_MDImg_data:: MD Image has not been initated properly\n";
        throw(std::invalid_argument(" MD Image has not been initated properly"));
    }
    // read signal and error -> presumably errors follow the signal;
    this->fileStreamHolder.seekg(this->positions.s_start,std::ios::beg);
    this->fileStreamHolder.read(&buff[0],nCells*8);


    for(i=0;i<nCells;i++){
        pImg_data[i].s   = (double)*((float32*)(&buff[i*4]));
        pImg_data[i].err = (double)*((float32*)(&buff[(i+nCells)*4]));

    }
    // read npixels
    this->fileStreamHolder.seekg(this->positions.n_cell_pix_start,std::ios::beg);
    this->fileStreamHolder.read(&buff[0],buff.size());

    hor_points_locations.resize(nCells);
    pImg_data[0].npix = (size_t)*((uint64_t*)(&buff[0*8]));
    hor_points_locations[0] = 0;
    for(i=1;i<nCells;i++){
        pImg_data[i].npix       = (size_t)*((uint64_t*)(&buff[i*8]));
        hor_points_locations[i] = hor_points_locations[i-1]+pImg_data[i-1].npix;
    }
	//
	mdd.setNpix(hor_points_locations[nCells-1]+pImg_data[nCells-1].npix);


}
   
MDPointDescription 
MD_FileHoraceReader::read_pointDescriptions(void)const
{
    const char *HoraceDataTags[]={"qx","qy","qz","en","S","err","iRunID","iDetID","iEn"};
    MDPointStructure  aPointDescr;
	aPointDescr.NumPixCompressionBits=0;
	aPointDescr.SignalLength = 4;
	aPointDescr.DimIDlength  = 4;
    std::vector<std::string> dataID(HoraceDataTags,HoraceDataTags+9);
    MDPointDescription theDescr(aPointDescr,dataID);
	this->hbs = theDescr.sizeofMDDPoint();

	if(hbs != 4*9){
		f_log.error()<<" The length of Horace data pixel ="<<hbs<<" which differs from the expected 36 bytes\n";
		throw(std::invalid_argument("Invalid Horace data pixel length obtained"));
	}
    return theDescr;
}
//
size_t 
MD_FileHoraceReader::read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
{
    size_t i,buffer_availible,cell_index;
    size_t iCellRead(starting_cell);
	size_t max_npix_inCell(0);
	max_npix_inCell=~max_npix_inCell;
	uint64_t nPix;
    // timing
    time_t start,end;
    time(&start);  //***********************************************>>>

    const MD_image_point *pImgData = dnd.get_const_pData();
    // buffer size provided;
    buffer_availible = pix_buf.size()/(hbs);

    // identify data extent fitting the buffer;
    n_pix_in_buffer = 0;
    for(i=starting_cell;i<selected_cells.size();i++){

        cell_index      = selected_cells[i];
		nPix            =pImgData[cell_index].npix;
		if(nPix>max_npix_inCell){
			 f_log.error()<<"The reader can not read this dataset as number of pixels contributed into cell"<<i<<" larger then current architecture allows\n";
			 throw(std::invalid_argument("Data size exceed the possibilities of this computer"));
		}
        n_pix_in_buffer+=(size_t)nPix;

        // end the loop earlier
        if(n_pix_in_buffer>buffer_availible){ 
            if(i==starting_cell){
                pix_buf.resize(n_pix_in_buffer*hbs);
                iCellRead=i;
            }else{
                iCellRead=i-1;
                n_pix_in_buffer-=(size_t)pImgData[cell_index].npix;
            }
            break;
        }
        iCellRead=i;
    }
    time(&end);     //***********************************************<<<<
    f_log.debug()<<" cells preselected in: "<<difftime (end,start)<<" sec\n";;

    // read data cell after cells indexes as provided;
    
    std::streamoff pixels_start;
    size_t         block_size(0);
    size_t         block_start(0);
    size_t         data_buffer_size(0);
//	
    size_t ic      = starting_cell;
    size_t ic_next = ic+1;
    if(ic_next>iCellRead)ic_next = iCellRead;
	uint64_t nBytes;
    time(&start);  //***********************************************>>>


	// read untill data buffer is full
    while(true){

        cell_index      = selected_cells[ic];
        pixels_start  =   this->positions.pix_start+hbs*hor_points_locations[cell_index];

         // optimisaion possible when cells are adjacent
        nBytes    = hbs*pImgData[cell_index].npix;
		if(nBytes> max_npix_inCell){
			 f_log.error()<<"The reader can not read this dataset as number of pixels contributed into cell"<<cell_index<<" larger then current architecture allows\n";
			 throw(std::invalid_argument("Data size exceed the possibilities of this computer"));
		}else{
			block_size = (size_t)nBytes;
		}

        // if the next cell follows the current on HDD, we should read them together aggregating adjacent cells;
        uint64_t next_block = hor_points_locations[cell_index]+pImgData[cell_index].npix;
        size_t next_index = selected_cells[ic_next];
        while(hor_points_locations[next_index]==next_block){
            // block size grows and all other is auxiliary
			    nBytes = hbs*pImgData[next_index].npix;
				if(nBytes> max_npix_inCell){
					 f_log.error()<<"The reader can not read this dataset as number of pixels contributed into cell"<<next_index<<" larger then current architecture allows\n";
					throw(std::invalid_argument("Data size exceed the possibilities of this computer"));
				}

            // block size grows and all other is auxiliary
                block_size    += (size_t)nBytes;
                ic = ic_next;
                ic_next++;
                if(ic_next > iCellRead)break;

                cell_index = selected_cells[ic];
                next_block = hor_points_locations[cell_index]+pImgData[cell_index].npix;
                next_index = selected_cells[ic_next];
            
        }

       
        this->fileStreamHolder.seekg(pixels_start,std::ios::beg);
        this->fileStreamHolder.read(&pix_buf[block_start],block_size);
        block_start+=block_size;

        // for single cell it is important to add but not to assign the next value, for more then one -- no difference
        ic++;
        ic_next++;
        if(ic>iCellRead)break;
        if(ic_next>iCellRead)ic_next=iCellRead;
    }
    time(&end);     //***********************************************<<<<
    f_log.debug()<<" cells read in: "<<difftime (end,start)<<" sec\n";;
    //
    time(&start);  //*******
	data_buffer_size = block_start; // latest block size has been already added
	compact_hor_data(&pix_buf[0],data_buffer_size);  
    time(&end);   
    f_log.debug()<<" cells transformed in: "<<difftime (end,start)<<" sec\n";;
    // returns next cell to read if any or size of the selection
    return ic;
}
//
void 
MD_FileHoraceReader::compact_hor_data(char *buffer,size_t &buf_size)
{
    size_t i;
    // data size should be in blocks of 9*4
    size_t data_size = buf_size/(hbs);
    if(data_size*hbs!= buf_size){
        f_log.error()<<" Block of Horace data does not arrived for compression in block of 9*4\n";
        throw(std::invalid_argument(" Block of Horace data does not arrived for compression in block of 9*4"));
    }

    float      Dim_sig[6];
    uint32_t   index[3];
	MDPointDescription pixInfo;
	pixInfo.PixInfo().DimIDlength =4;
	pixInfo.PixInfo().SignalLength=4;

 	MDDataPointEqual<float,uint32_t,float> defPoint(buffer,pixInfo);
    // output buffer size now decreases;
    buf_size = data_size*defPoint.sizeofMDDataPoint();
    for(i=0;i<data_size;i++){
        Dim_sig[0] =(float)*((float *)(buffer+i*hbs));
        Dim_sig[1] =(float)*((float *)(buffer+i*hbs+4));
        Dim_sig[2] =(float)*((float *)(buffer+i*hbs+8));
        Dim_sig[3] =(float)*((float *)(buffer+i*hbs+12));

        index[0]  =(uint32_t)*((uint32_t *)(buffer+i*hbs+16)); 
        index[1]  =(uint32_t)*((uint32_t *)(buffer+i*hbs+20)); 
        index[2]  =(uint32_t)*((uint32_t *)(buffer+i*hbs+24)); 

        Dim_sig[4] =(float)*((float*)(buffer+i*hbs+28));
        Dim_sig[5] =(float)*((float*)(buffer+i*hbs+32));
        if(Dim_sig[4] == -1e+30){
            Dim_sig[4] = std::numeric_limits<float>::quiet_NaN();
            Dim_sig[5] = Dim_sig[4];
        }


        defPoint.setData(i,Dim_sig,index);
    }

}
//
 //read whole pixels information in memory; usually impossible, then returns false;
bool 
MD_FileHoraceReader::read_pix(MDDataPoints &sqw, bool nothrow)
{
	// number of pixels in dataset
    uint64_t n_pix_inDataset  = this->getNPix();
  	// size of one data point
	unsigned int pix_size = hbs;

    // It is often impossible to allocate the array needed to place all pixels in the pixel buffer. In this case this function has to fail as it is 
    // not possible to read all pixels into memory;
	char * pix_array;
	size_t max_npix_in_buf(0);
	try{
		if(n_pix_inDataset>~max_npix_in_buf){
			f_log.information()<<" pixel array of "<<n_pix_inDataset<<" pixels can not be placed in memory on current architectrue\n";
			if(nothrow){
				return false;
			}else{
				throw(std::bad_alloc("too many pixels to place in memory for given architecture "));
			}
		}else{
			max_npix_in_buf= (size_t)n_pix_inDataset;
		}
		std::vector<char> *pa = sqw.get_pBuffer(max_npix_in_buf);
		pix_array             = &(*pa)[0];
	}catch(std::bad_alloc &err){
		f_log.information()<<" can not allocate memory for "<<n_pix_inDataset<<" pixels\n";
	    sqw.set_file_based();
		if(nothrow){
			return false;
		}else{
			throw(err);
		}
	}
// sufficient memory is availible
	size_t data_buffer_size = pix_size*max_npix_in_buf;
// read data
    this->fileStreamHolder.seekg(this->positions.pix_start,std::ios::beg);
    this->fileStreamHolder.read(pix_array,data_buffer_size);
//
	compact_hor_data(pix_array,data_buffer_size);
	
	return true;
}
 /// get number of data pixels(points) contributing into the dataset;
uint64_t
MD_FileHoraceReader::getNPix(void)
{
    return this->nDataPoints;
}

// auxiliary functions
void
MD_FileHoraceReader::parse_sqw_main_header()
{ // we do not need this header  at the moment -> just need to calculated its length;

  std::vector<char> data_buffer(4 * 3);
  this->fileStreamHolder.read(&data_buffer[0], 4);                validateFileStreamHolder(fileStreamHolder);

  unsigned int file_name_length = *((uint32_t*) (&data_buffer[0]));
  //skip main header file name
  fileStreamHolder.seekg(file_name_length, std::ios_base::cur);   validateFileStreamHolder(fileStreamHolder);

  this->fileStreamHolder.read(&data_buffer[0], 4);                validateFileStreamHolder(fileStreamHolder);
  unsigned int file_path_length = *((uint32_t*) (&data_buffer[0]));


  //skip main header file path
  fileStreamHolder.seekg(file_path_length, std::ios_base::cur);    validateFileStreamHolder(fileStreamHolder);

  this->fileStreamHolder.read(&data_buffer[0], 4);
  unsigned int file_title = *((uint32_t*) (&data_buffer[0]));      validateFileStreamHolder(fileStreamHolder);

  //skip main header file path
  fileStreamHolder.seekg(file_title, std::ios_base::cur);          validateFileStreamHolder(fileStreamHolder);

  // indentify number of file headers, contributed into the dataset
  this->fileStreamHolder.read(&data_buffer[0], 4);                 validateFileStreamHolder(fileStreamHolder);
  unsigned int nFiles = *((uint32_t*) (&data_buffer[0]));


  /// allocate space for the component headers positions;
  positions.component_headers_starts.assign(nFiles, 0);

  std::streamoff last_location = fileStreamHolder.tellg();
  if (nFiles > 0)
  {
    positions.component_headers_starts[0] = last_location;
  }
  //return last_location;
  return;

}

// 
std::streamoff 
MD_FileHoraceReader::parse_component_header(std::streamoff start_location)
{ // we do not need this header  at the moment -> just calculating its length; or may be we do soon?
    std::vector<char> data_buffer(8);
/*
[n, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
[data.filename, count, ok, mess] = fread_catch(fid,[1,n],'*char'); if ~all(ok); return; end;

[n, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
[data.filepath, count, ok, mess] = fread_catch(fid,[1,n],'*char'); if ~all(ok); return; end;

[data.efix,   count, ok, mess] = fread_catch(fid,1,    'float32'); if ~all(ok); return; end;
[data.emode,  count, ok, mess] = fread_catch(fid,1,    'int32');   if ~all(ok); return; end;
[data.alatt,  count, ok, mess] = fread_catch(fid,[1,3],'float32'); if ~all(ok); return; end;
[data.angdeg, count, ok, mess] = fread_catch(fid,[1,3],'float32'); if ~all(ok); return; end;
[data.cu,     count, ok, mess] = fread_catch(fid,[1,3],'float32'); if ~all(ok); return; end;
[data.cv,     count, ok, mess] = fread_catch(fid,[1,3],'float32'); if ~all(ok); return; end;
[data.psi,    count, ok, mess] = fread_catch(fid,1,    'float32'); if ~all(ok); return; end;
[data.omega,  count, ok, mess] = fread_catch(fid,1,    'float32'); if ~all(ok); return; end;
[data.dpsi,   count, ok, mess] = fread_catch(fid,1,    'float32'); if ~all(ok); return; end;
[data.gl,     count, ok, mess] = fread_catch(fid,1,    'float32'); if ~all(ok); return; end;
[data.gs,     count, ok, mess] = fread_catch(fid,1,    'float32'); if ~all(ok); return; end;

[ne, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
[data.en,count,ok,mess] = fread_catch(fid, [ne,1], 'float32'); if ~all(ok); return; end;

[data.uoffset, count, ok, mess] = fread_catch(fid,[4,1],'float32'); if ~all(ok); return; end;
[data.u_to_rlu,count, ok, mess] = fread_catch(fid,[4,4],'float32'); if ~all(ok); return; end;
[data.ulen,    count, ok, mess] = fread_catch(fid,[1,4],'float32'); if ~all(ok); return; end;

[n, count, ok, mess] = fread_catch(fid,2,'int32'); if ~all(ok); return; end;
[ulabel, count, ok, mess] = fread_catch(fid,[n(1),n(2)],'*char'); if ~all(ok); return; end;
data.ulabel=cellstr(ulabel)';
*/

    std::streamoff end_location = start_location;
    std::streamoff shift = start_location-this->fileStreamHolder.tellg();
    // move to spefied location, which should be usually 0;
    fileStreamHolder.seekg(shift,std::ios_base::cur);



    this->fileStreamHolder.read(&data_buffer[0],4);
    validateFileStreamHolder(fileStreamHolder);

    unsigned int file_name_length= *((uint32_t*)(&data_buffer[0]));
    //skip component header file name
    fileStreamHolder.seekg(file_name_length,std::ios_base::cur);
    validateFileStreamHolder(fileStreamHolder);

    this->fileStreamHolder.read(&data_buffer[0],4);
    unsigned int file_path_length= *((uint32_t*)(&data_buffer[0]));
    validateFileStreamHolder(fileStreamHolder);

    //skip component header file path
    fileStreamHolder.seekg(file_path_length,std::ios_base::cur);
    validateFileStreamHolder(fileStreamHolder);

    // move to by specifified nuber of bytes, see Matlab header above;
    fileStreamHolder.seekg(4*(7+3*4),std::ios_base::cur);
    validateFileStreamHolder(fileStreamHolder);

    // read number of energy bins;
    this->fileStreamHolder.read(&data_buffer[0],4);
    unsigned int nEn_bins = *((uint32_t*)(&data_buffer[0]));
    // skip energy values;
    fileStreamHolder.seekg(4*(nEn_bins),std::ios_base::cur);
    validateFileStreamHolder(fileStreamHolder);
    // skip offsets and conversions;
    fileStreamHolder.seekg(4*(4+4*4+4),std::ios_base::cur);
    validateFileStreamHolder(fileStreamHolder);

    // get labels matix size;
    this->fileStreamHolder.read(&data_buffer[0],8);
    validateFileStreamHolder(fileStreamHolder);
    unsigned int nRows = *((uint32_t*)(&data_buffer[0]));
    unsigned int nCols = *((uint32_t*)(&data_buffer[4]));

    // skip labels
    fileStreamHolder.seekg(nRows*nCols,std::ios_base::cur);
    validateFileStreamHolder(fileStreamHolder);

    end_location = (unsigned int)fileStreamHolder.tellg();
    return end_location;


}
//
std::streamoff
MD_FileHoraceReader::parse_sqw_detpar(std::streamoff start_location)
{ //
        std::vector<char> data_buffer(8);
/*
[n, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
[det.filename, count, ok, mess] = fread_catch(fid,[1,n],'*char'); if ~all(ok); return; end;

[n, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
[det.filepath, count, ok, mess] = fread_catch(fid,[1,n],'*char'); if ~all(ok); return; end;

[ndet, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
[det.group, count,ok,mess] = fread_catch(fid, [1,ndet], 'float32'); if ~all(ok); return; end;
[det.x2,    count,ok,mess] = fread_catch(fid, [1,ndet], 'float32'); if ~all(ok); return; end;
[det.phi,   count,ok,mess] = fread_catch(fid, [1,ndet], 'float32'); if ~all(ok); return; end;
[det.azim,  count,ok,mess] = fread_catch(fid, [1,ndet], 'float32'); if ~all(ok); return; end;
[det.width, count,ok,mess] = fread_catch(fid, [1,ndet], 'float32'); if ~all(ok); return; end;
[det.height,count,ok,mess] = fread_catch(fid, [1,ndet], 'float32'); if ~all(ok); return; end;
*/

    std::streamoff end_location = start_location;
    std::streamoff shift = start_location-this->fileStreamHolder.tellg();
    // move to specified location, which should be usually 0;
    fileStreamHolder.seekg(shift,std::ios_base::cur);              validateFileStreamHolder(fileStreamHolder);


    this->fileStreamHolder.read(&data_buffer[0],4);                 validateFileStreamHolder(fileStreamHolder);

    unsigned int file_name_length= *((uint32_t*)(&data_buffer[0]));
    //skip component header file name
    fileStreamHolder.seekg(file_name_length,std::ios_base::cur);     validateFileStreamHolder(fileStreamHolder);

    this->fileStreamHolder.read(&data_buffer[0],4);
    unsigned int file_path_length= *((uint32_t*)(&data_buffer[0]));  validateFileStreamHolder(fileStreamHolder);
    //skip component header file path
    fileStreamHolder.seekg(file_path_length,std::ios_base::cur);     validateFileStreamHolder(fileStreamHolder);

    this->fileStreamHolder.read(&data_buffer[0],4);
    unsigned int num_detectors = *((uint32_t*)(&data_buffer[0]));
//skip detector information
    fileStreamHolder.seekg(num_detectors*6*4,std::ios_base::cur);  validateFileStreamHolder(fileStreamHolder);

    end_location = fileStreamHolder.tellg();
    return end_location;


}
void
MD_FileHoraceReader::parse_data_locations(std::streamoff data_start)
{
    std::vector<char> data_buffer(12);
    unsigned int i;

    //std::streamoff end_location = data_start;
    std::streamoff shift = data_start-this->fileStreamHolder.tellg();
    // move to specified location, which should be usually 0;
    fileStreamHolder.seekg(shift,std::ios_base::cur);
  validateFileStreamHolder(fileStreamHolder);
/*   
    [n, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
    [dummy_filename, count, ok, mess] = fread_catch(fid,[1,n],'*char'); if ~all(ok); return; end;

    [n, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
    [dummy_filepath, count, ok, mess] = fread_catch(fid,[1,n],'*char'); if ~all(ok); return; end;

    [n, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
    [data.title, count, ok, mess] = fread_catch(fid,[1,n],'*char'); if ~all(ok); return; end;
*/
    this->fileStreamHolder.read(&data_buffer[0],4);
  validateFileStreamHolder(fileStreamHolder);

    unsigned int file_name_length= *((uint32_t*)(&data_buffer[0]));
    //skip dummy file name
    fileStreamHolder.seekg(file_name_length,std::ios_base::cur);
  validateFileStreamHolder(fileStreamHolder);

    this->fileStreamHolder.read(&data_buffer[0],4);
    unsigned int file_path_length= *((uint32_t*)(&data_buffer[0]));
  validateFileStreamHolder(fileStreamHolder);
    //skip dummy file path
    fileStreamHolder.seekg(file_path_length,std::ios_base::cur);
  validateFileStreamHolder(fileStreamHolder);

    this->fileStreamHolder.read(&data_buffer[0],4);
    unsigned int data_title_length = *((uint32_t*)(&data_buffer[0]));
  validateFileStreamHolder(fileStreamHolder);
    //skip data title
    fileStreamHolder.seekg(data_title_length,std::ios_base::cur);
  validateFileStreamHolder(fileStreamHolder);

    this->positions.geom_start = fileStreamHolder.tellg();
    // skip geometry information
/*
    [data.alatt, count, ok, mess] = fread_catch(fid,[1,3],'float32'); if ~all(ok); return; end;
    [data.angdeg, count, ok, mess] = fread_catch(fid,[1,3],'float32'); if ~all(ok); return; end;

    [data.uoffset, count, ok, mess] = fread_catch(fid,[4,1],'float32'); if ~all(ok); return; end;
    [data.u_to_rlu, count, ok, mess] = fread_catch(fid,[4,4],'float32'); if ~all(ok); return; end;
    [data.ulen, count, ok, mess] = fread_catch(fid,[1,4],'float32'); if ~all(ok); return; end;
*/
    fileStreamHolder.seekg(4*(3+3+4+16+4),std::ios_base::cur);
/*
    [n, count, ok, mess] = fread_catch(fid,2,'int32'); if ~all(ok); return; end;
    [ulabel, count, ok, mess] = fread_catch(fid,[n(1),n(2)],'*char'); if ~all(ok); return; end;
    data.ulabel=cellstr(ulabel)';
*/
    // get label information and skip labels;
    this->fileStreamHolder.read(&data_buffer[0],8);
  validateFileStreamHolder(fileStreamHolder);
    unsigned int n_labels      = *((uint32_t*)(&data_buffer[0])); 
    unsigned int labels_length = *((uint32_t*)(&data_buffer[4])); 
    fileStreamHolder.seekg(n_labels*labels_length,std::ios_base::cur);

    this->positions.npax_start = fileStreamHolder.tellg();
/*
    [npax, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
    niax=4-npax;
    if niax~=0
        [data.iax, count, ok, mess] = fread_catch(fid,[1,niax],'int32'); if ~all(ok); return; end;
        [data.iint, count, ok, mess] = fread_catch(fid,[2,niax],'float32'); if ~all(ok); return; end;
    else
        data.iax=zeros(1,0);    % create empty index of integration array in standard form
        data.iint=zeros(2,0);
    end
*/
    this->fileStreamHolder.read(&data_buffer[0],4);
  validateFileStreamHolder(fileStreamHolder);;
    unsigned int npax = *((uint32_t*)(&data_buffer[0])); 
    unsigned int niax = npax-4;
    if(niax!=0){
        fileStreamHolder.seekg(3*niax*4,std::ios_base::cur);
      validateFileStreamHolder(fileStreamHolder);
    }
    if(npax!=0){
        this->nBins.resize(npax);
        /*
        [data.pax, count, ok, mess] = fread_catch(fid,[1,npax],'int32'); if ~all(ok); return; end;
        psize=zeros(1,npax);    % will contain number of bins along each dimension of plot axes
        for i=1:npax
            [np,count,ok,mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
            [data.p{i},count,ok,mess] = fread_catch(fid,np,'float32'); if ~all(ok); return; end;
            psize(i)=np-1;
        end
        */
        // skip projection axis
        fileStreamHolder.seekg(npax*4,std::ios_base::cur);
      validateFileStreamHolder(fileStreamHolder);
        this->mdImageSize = 1;
        unsigned int nAxisPoints;
        for(i=0;i<npax;i++){
            this->fileStreamHolder.read(&data_buffer[0],4);
          validateFileStreamHolder(fileStreamHolder);
            nAxisPoints = *((uint32_t*)(&data_buffer[0])); 
            nBins[i] = nAxisPoints-1;
            this->mdImageSize *= nBins[i] ;
            fileStreamHolder.seekg(nAxisPoints*4,std::ios_base::cur);
            validateFileStreamHolder(fileStreamHolder);
        }
        /*
        [data.dax, count, ok, mess] = fread_catch(fid,[1,npax],'int32'); if ~all(ok); return; end;
        if length(psize)==1
            psize=[psize,1];    % make size of a column vector
        end
        */
        // skip display indexes;
        fileStreamHolder.seekg(npax*4,std::ios_base::cur);
      validateFileStreamHolder(fileStreamHolder);
    }
    // signal start:
    this->positions.s_start = fileStreamHolder.tellg();
    // and skip to errors
    fileStreamHolder.seekg(this->mdImageSize*4,std::ios_base::cur);
    validateFileStreamHolder(fileStreamHolder);
    // error start:
    this->positions.err_start= fileStreamHolder.tellg();
    fileStreamHolder.seekg(this->mdImageSize*4,std::ios_base::cur);
    validateFileStreamHolder(fileStreamHolder);
    // dnd data file.  we do not suppor this?
    if(fileStreamHolder.eof()){
        f_log.error()<<" DND horace data file supplied. This file reader needs SQW Horace type data file\n";
        throw(std::invalid_argument("DND Horace datasets are not supported by Mantid"));
    }
/*
    % All of the above fields will be present in a valid sqw file. The following need not exist, but to be a valid sqw file,
    % for any one field to be present all earlier fields must have been written. 
    position.npix=[];
    position.pix=[];
    npixtot=[];
*/

    this->positions.n_cell_pix_start=fileStreamHolder.tellg();
    // skip to the end of pixels;
    fileStreamHolder.seekg(this->mdImageSize*8,std::ios_base::cur);
  validateFileStreamHolder(fileStreamHolder);
    if(fileStreamHolder.eof()){
        f_log.error()<<" DND b+ horace data file supplied. This file reader needs full SQW Horace type data file\n";
        throw(std::invalid_argument("DND b+ Horace datasets are not supported by Mantid"));
    }
    this->positions.min_max_start = fileStreamHolder.tellg();
    // skip min-max start
    //[data.urange,count,ok,mess] = fread_catch(fid,[2,4],'float32'); if ~all(ok); return; end;
    fileStreamHolder.seekg(8*4,std::ios_base::cur);
  validateFileStreamHolder(fileStreamHolder);
    if(fileStreamHolder.eof()){
        f_log.error()<<" SQW a- horace data file supplied. This file reader needs full SQW Horace type data file\n";
        throw(std::invalid_argument("SQW a- Horace datasets are not supported by Mantid"));
    }
    // skip redundant field and read nPix (number of data points)
    this->fileStreamHolder.read(&data_buffer[0],12);
  validateFileStreamHolder(fileStreamHolder);
    this->nDataPoints =(size_t)( *((uint64_t*)(&data_buffer[4])));
    this->positions.pix_start = this->fileStreamHolder.tellg();
}

MD_FileHoraceReader::~MD_FileHoraceReader(void)
{
    fileStreamHolder.close();
}
    // Function performing work previously in GOTO statement.
void inline 
MD_FileHoraceReader::validateFileStreamHolder(std::ifstream& fileStreamHolder)
{
    if (fileStreamHolder.bad()){
        f_log.error() << " Error reading main sqw file header for file " << this->File_name << "\n";
        throw(Kernel::Exception::FileError("Error reading main sqw file header ", this->File_name));
    }
}
} // end namespaces
}
}
