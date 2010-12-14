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
File_name(file_name),
positions(),
mdImageSize(0),
nDataPoints(0)
{
	if(sizeof(float32)!=4){
		f_log.error()<<"MD_FileHoraceReader is not defined on a computer with non-32-bit float\n";
		throw(std::bad_cast("MD_FileHoraceReader can not cast non-32 bif float properly "));
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
	unsigned int nFiles        = positions.component_headers_starts.size();
	for(unsigned int i=0;i<nFiles;i++){
		positions.component_headers_starts[i] = next_position;
		next_position = parse_component_header(next_position);
	}
	positions.detectors_start = next_position;
	// get detectors
	positions.data_start      = parse_sqw_detpar(positions.detectors_start);
	// calculate all other data fields locations;
	parse_data_locations(positions.data_start);
}


std::string MD_FileHoraceReader::getFileName() const
{
  return this->File_name;
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

    UnitCell cell;
	basisGeometry.init(basisDimensions,cell);
	// get_sqw_header should go here and define cell
}
//
void 
MD_FileHoraceReader::read_MDGeomDescription(Mantid::Geometry::MDGeometryDescription &dscrptn)
{
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
		dscrptn.dimDescription(i).data_shift = val;
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
		dscrptn.dimDescription(i).data_scale= *((float32*)(&buf[i0+i*4]));
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
		dscrptn.dimDescription(i).AxisName = name;
		// hardcoded here as we read the dimensions ID (tags and Horace does not do it)
		dscrptn.dimDescription(i).Tag    = Horace_tags[i];
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


		this->fileStreamHolder.read(&buf[0],buf.size());
		int i_axis_index;
		for(i=0;i<niax;i++){
			i_axis_index = *((uint32_t*)(&buf[i*4]));

			dscrptn.dimDescription(dimID[i_axis_index]).nBins = 1; // this sets this axis integrated
			dscrptn.dimDescription(dimID[i_axis_index]).cut_min = *((float32*)(&buf[4*(niax+i*2)])); // min integration value
			dscrptn.dimDescription(dimID[i_axis_index]).cut_max = *((float32*)(&buf[4*(niax+i*2+1)])); // max integration value
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

			// this do not describes axis on an irregular grid. 
			//TODO: implement irregular axis and put check if the grid is regular or not. 
			dscrptn.dimDescription(current_tag).nBins   = nAxisPoints-1; // this sets num-bins
			dscrptn.dimDescription(current_tag).cut_min = *((float32*)(&axis_buffer[4*(0)])); // min axis value
			dscrptn.dimDescription(current_tag).cut_max = *((float32*)(&axis_buffer[4*(nAxisPoints-1)])); // max axis value


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

	for(i=0;i<nCells;i++){
		pImg_data[i].npix = (size_t)*((uint64_t*)(&buff[i*8]));
	}


}
   
MDPointDescription 
MD_FileHoraceReader::read_pointDescriptions(void)const
{
	MDPointDescription defaultDescr;
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
MD_FileHoraceReader::read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
{
	size_t i,buffer_availible,cell_index;
	size_t iCellRead(starting_cell);

	const MD_image_point *pImgData = dnd.get_const_pData();
	// buffer size provided;
	buffer_availible = pix_buf.size()/(hbs);

	// identify data extent fitting the buffer;
	n_pix_in_buffer = 0;
	for(i=starting_cell;i<selected_cells.size();i++){

		cell_index      = selected_cells[i];
		n_pix_in_buffer+=pImgData[cell_index].npix;

		// end the loop earlier
		if(n_pix_in_buffer>buffer_availible){ 
			if(i==starting_cell){
				pix_buf.resize(n_pix_in_buffer);
				iCellRead++;
			}else{
				iCellRead=i-1;
				n_pix_in_buffer-=pImgData[cell_index].npix;
			}
			break;
		}
		iCellRead=i;
	}
	// read data cell after cells indexes as provided;
	
	std::streamoff pixels_start;
	size_t         block_size(0);
	size_t         block_start(0);
//	
	size_t ic      = starting_cell;
	size_t ic_next = ic+1;

	while(ic<iCellRead){

		cell_index      = selected_cells[ic];
		pixels_start  =   this->positions.pix_start+hbs*pImgData[cell_index].chunk_location; 

		// optimisaion possible when cells are adjacent
		block_size    = hbs*pImgData[cell_index].npix;

		// if the next cell follows the current on HDD, we should read them together aggregating adjacent cells;
		size_t next_block = pImgData[cell_index].chunk_location+pImgData[cell_index].npix;
		size_t next_index = selected_cells[ic_next];
		while(pImgData[next_index].chunk_location==next_block){
			// block size grows and all other is auxiliary
				block_size    += hbs*pImgData[next_index].npix;
				ic = ic_next;
				ic_next++;
				if(ic_next == iCellRead)break;

				cell_index = selected_cells[ic];
				next_block = pImgData[cell_index].chunk_location+pImgData[cell_index].npix;
				next_index = selected_cells[ic_next];
			
		}


		this->fileStreamHolder.seekg(pixels_start,std::ios::beg);
		this->fileStreamHolder.read(&pix_buf[block_start],block_size);
		// compacting horace data in memory
		this->compact_hor_data(&pix_buf[block_start],block_size);
		block_start+=block_size;

		ic = ic_next;
	    ic_next++;
	}

	return iCellRead;
}
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

	float Dim_sig[6];
	int   index[3];

	MDDataPoint<float,uint16_t,float> defPoint(buffer);
	// output buffer size now decreases;
	buf_size = data_size*defPoint.sizeofMDDataPoint();
	for(i=0;i<data_size;i++){
		Dim_sig[0] =(float)*((float *)(buffer+i*hbs));
		Dim_sig[1] =(float)*((float *)(buffer+i*hbs+4));
		Dim_sig[2] =(float)*((float *)(buffer+i*hbs+8));
		Dim_sig[3] =(float)*((float *)(buffer+i*hbs+12));

		Dim_sig[4] =(float)*((float*)(buffer+i*hbs+16));
		Dim_sig[5] =(float)*((float*)(buffer+i*hbs+20));

		index[0]  =(int)*((uint32_t *)(buffer+i*hbs+24)); 
		index[1]  =(int)*((uint32_t *)(buffer+i*hbs+28)); 
		index[2]  =(int)*((uint32_t *)(buffer+i*hbs+32)); 

		defPoint.setData(i,Dim_sig,index);
	}

}

 /// get number of data pixels(points) contributing into the dataset;
size_t 
MD_FileHoraceReader::getNPix(void)
{
	return this->nDataPoints;
}
// auxiliary functions
void
MD_FileHoraceReader::parse_sqw_main_header()
{ // we do not need this header  at the moment -> just need to calculated its length;

		std::vector<char> data_buffer(4*3);
		this->fileStreamHolder.read(&data_buffer[0],4);		           if(fileStreamHolder.bad())goto Error;

		unsigned int file_name_length= *((uint32_t*)(&data_buffer[0]));
		//skip main header file name
		fileStreamHolder.seekg(file_name_length,std::ios_base::cur);   if(fileStreamHolder.bad())goto Error;

		this->fileStreamHolder.read(&data_buffer[0],4);
		unsigned int file_path_length= *((uint32_t*)(&data_buffer[0])); if(fileStreamHolder.bad())goto Error;
		//skip main header file path
		fileStreamHolder.seekg(file_path_length,std::ios_base::cur);    if(fileStreamHolder.bad())goto Error;

		this->fileStreamHolder.read(&data_buffer[0],4);
		unsigned int file_title = *((uint32_t*)(&data_buffer[0]));      if(fileStreamHolder.bad())goto Error;
		//skip main header file path
		fileStreamHolder.seekg(file_title,std::ios_base::cur);          if(fileStreamHolder.bad())goto Error;

		// indentify number of file headers, contributed into the dataset
		this->fileStreamHolder.read(&data_buffer[0],4);
		unsigned int nFiles= *((uint32_t*)(&data_buffer[0])); if(fileStreamHolder.bad())goto Error;
       /// allocate space for the component headers positions;
		positions.component_headers_starts.assign(nFiles,0);

		std::streamoff last_location = fileStreamHolder.tellg();
		if(nFiles>0){
			positions.component_headers_starts[0] = last_location;
		}
	//return last_location;
		return;
Error:
		f_log.error()<<" Error reading main sqw file header for file " <<this->File_name<<"\n";
		throw(Kernel::Exception::FileError("Error reading main sqw file header ",this->File_name));
		
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
	fileStreamHolder.seekg(shift,std::ios_base::cur);               if(fileStreamHolder.bad())goto Error;


	this->fileStreamHolder.read(&data_buffer[0],4);		             if(fileStreamHolder.bad())goto Error;

	unsigned int file_name_length= *((uint32_t*)(&data_buffer[0]));
	//skip component header file name
	fileStreamHolder.seekg(file_name_length,std::ios_base::cur);   if(fileStreamHolder.bad())goto Error;

	this->fileStreamHolder.read(&data_buffer[0],4);
	unsigned int file_path_length= *((uint32_t*)(&data_buffer[0])); if(fileStreamHolder.bad())goto Error;
	//skip component header file path
	fileStreamHolder.seekg(file_path_length,std::ios_base::cur);    if(fileStreamHolder.bad())goto Error;

	// move to by specifified nuber of bytes, see Matlab header above;
	fileStreamHolder.seekg(4*(7+3*4),std::ios_base::cur);    if(fileStreamHolder.bad())goto Error;
	// read number of energy bins;
	this->fileStreamHolder.read(&data_buffer[0],4);
	unsigned int nEn_bins = *((uint32_t*)(&data_buffer[0]));
	// skip energy values;
	fileStreamHolder.seekg(4*(nEn_bins),std::ios_base::cur);    if(fileStreamHolder.bad())goto Error;
	// skip offsets and conversions;
	fileStreamHolder.seekg(4*(4+4*4+4),std::ios_base::cur);    if(fileStreamHolder.bad())goto Error;

	// get labels matix size;
	this->fileStreamHolder.read(&data_buffer[0],8);		       if(fileStreamHolder.bad())goto Error;
	unsigned int nRows = *((uint32_t*)(&data_buffer[0]));
	unsigned int nCols = *((uint32_t*)(&data_buffer[4]));

	// skip labels
	fileStreamHolder.seekg(nRows*nCols,std::ios_base::cur);    if(fileStreamHolder.bad())goto Error;

	end_location = (unsigned int)fileStreamHolder.tellg();
	return end_location;
Error:
		f_log.error()<<" Error reading sqw component file header for file " <<this->File_name<<"\n";
		throw(Kernel::Exception::FileError("Error reading sqw component file header ",this->File_name));

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
	fileStreamHolder.seekg(shift,std::ios_base::cur);   if(fileStreamHolder.bad())goto Error;


	this->fileStreamHolder.read(&data_buffer[0],4);		            if(fileStreamHolder.bad())goto Error;

	unsigned int file_name_length= *((uint32_t*)(&data_buffer[0]));
	//skip component header file name
	fileStreamHolder.seekg(file_name_length,std::ios_base::cur);   if(fileStreamHolder.bad())goto Error;

	this->fileStreamHolder.read(&data_buffer[0],4);
	unsigned int file_path_length= *((uint32_t*)(&data_buffer[0])); if(fileStreamHolder.bad())goto Error;
	//skip component header file path
	fileStreamHolder.seekg(file_path_length,std::ios_base::cur);    if(fileStreamHolder.bad())goto Error;

	this->fileStreamHolder.read(&data_buffer[0],4);
	unsigned int num_detectors = *((uint32_t*)(&data_buffer[0]));
//skip detector information
	fileStreamHolder.seekg(num_detectors*6*4,std::ios_base::cur);    if(fileStreamHolder.bad())goto Error;

	end_location = fileStreamHolder.tellg();
	return end_location;
Error:
		f_log.error()<<" Error reading detectors for file " <<this->File_name<<"\n";
		throw(Kernel::Exception::FileError("Error reading detectors for file ",this->File_name));

}
void
MD_FileHoraceReader::parse_data_locations(std::streamoff data_start)
{
	std::vector<char> data_buffer(12);
	unsigned int i;

	std::streamoff end_location = data_start;
	std::streamoff shift = data_start-this->fileStreamHolder.tellg();
	// move to specified location, which should be usually 0;
	fileStreamHolder.seekg(shift,std::ios_base::cur);       if(fileStreamHolder.bad())goto Error;
/*   
	[n, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
    [dummy_filename, count, ok, mess] = fread_catch(fid,[1,n],'*char'); if ~all(ok); return; end;

    [n, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
    [dummy_filepath, count, ok, mess] = fread_catch(fid,[1,n],'*char'); if ~all(ok); return; end;

    [n, count, ok, mess] = fread_catch(fid,1,'int32'); if ~all(ok); return; end;
    [data.title, count, ok, mess] = fread_catch(fid,[1,n],'*char'); if ~all(ok); return; end;
*/
	this->fileStreamHolder.read(&data_buffer[0],4);		            if(fileStreamHolder.bad())goto Error;

	unsigned int file_name_length= *((uint32_t*)(&data_buffer[0]));
	//skip dummy file name
	fileStreamHolder.seekg(file_name_length,std::ios_base::cur);   if(fileStreamHolder.bad())goto Error;

	this->fileStreamHolder.read(&data_buffer[0],4);
	unsigned int file_path_length= *((uint32_t*)(&data_buffer[0])); if(fileStreamHolder.bad())goto Error;
	//skip dummy file path
	fileStreamHolder.seekg(file_path_length,std::ios_base::cur);    if(fileStreamHolder.bad())goto Error;

	this->fileStreamHolder.read(&data_buffer[0],4);
	unsigned int data_title_length = *((uint32_t*)(&data_buffer[0])); if(fileStreamHolder.bad())goto Error;
	//skip data title
	fileStreamHolder.seekg(data_title_length,std::ios_base::cur);    if(fileStreamHolder.bad())goto Error;

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
	this->fileStreamHolder.read(&data_buffer[0],8);  if(fileStreamHolder.bad())goto Error;
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
	this->fileStreamHolder.read(&data_buffer[0],4);           if(fileStreamHolder.bad())goto Error;
    unsigned int npax = *((uint32_t*)(&data_buffer[0])); 
	unsigned int niax = npax-4;
	if(niax!=0){
		fileStreamHolder.seekg(3*niax*4,std::ios_base::cur);  if(fileStreamHolder.bad())goto Error;
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
		fileStreamHolder.seekg(npax*4,std::ios_base::cur);            if(fileStreamHolder.bad())goto Error;
		this->mdImageSize = 1;
		unsigned int nAxisPoints;
		for(i=0;i<npax;i++){
			this->fileStreamHolder.read(&data_buffer[0],4);           if(fileStreamHolder.bad())goto Error;
			nAxisPoints = *((uint32_t*)(&data_buffer[0])); 
			nBins[i] = nAxisPoints-1;
			this->mdImageSize *= nBins[i] ;
		    fileStreamHolder.seekg(nAxisPoints*4,std::ios_base::cur);     if(fileStreamHolder.bad())goto Error;
		}
		/*
		[data.dax, count, ok, mess] = fread_catch(fid,[1,npax],'int32'); if ~all(ok); return; end;
		if length(psize)==1
			psize=[psize,1];    % make size of a column vector
		end
		*/
		// skip display indexes;
		fileStreamHolder.seekg(npax*4,std::ios_base::cur);            if(fileStreamHolder.bad())goto Error;
	}
	// signal start:
	this->positions.s_start = fileStreamHolder.tellg();
	// and skip to errors
    fileStreamHolder.seekg(this->mdImageSize*4,std::ios_base::cur);            if(fileStreamHolder.bad())goto Error;
	// error start:
	this->positions.err_start= fileStreamHolder.tellg();
    fileStreamHolder.seekg(this->mdImageSize*4,std::ios_base::cur);            if(fileStreamHolder.bad())goto Error;
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
	fileStreamHolder.seekg(this->mdImageSize*8,std::ios_base::cur);            if(fileStreamHolder.bad())goto Error;
	if(fileStreamHolder.eof()){
		f_log.error()<<" DND b+ horace data file supplied. This file reader needs full SQW Horace type data file\n";
		throw(std::invalid_argument("DND b+ Horace datasets are not supported by Mantid"));
	}
	this->positions.min_max_start = fileStreamHolder.tellg();
	// skip min-max start
    //[data.urange,count,ok,mess] = fread_catch(fid,[2,4],'float32'); if ~all(ok); return; end;
	fileStreamHolder.seekg(8*4,std::ios_base::cur);            if(fileStreamHolder.bad())goto Error;
	if(fileStreamHolder.eof()){
		f_log.error()<<" SQW a- horace data file supplied. This file reader needs full SQW Horace type data file\n";
		throw(std::invalid_argument("SQW a- Horace datasets are not supported by Mantid"));
	}
	// skip redundant field and read nPix (number of data points)
	this->fileStreamHolder.read(&data_buffer[0],12);              if(fileStreamHolder.bad())goto Error;
	this->nDataPoints =(size_t)( *((uint64_t*)(&data_buffer[4])));
	this->positions.pix_start = this->fileStreamHolder.tellg();

	return ;
Error:
		f_log.error()<<" Error identifying data locations for file " <<this->File_name<<"\n";
		throw(Kernel::Exception::FileError("Error identifying data locations ",this->File_name));


}

MD_FileHoraceReader::~MD_FileHoraceReader(void)
{
	fileStreamHolder.close();
}
} // end namespaces
}
}