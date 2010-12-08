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
	if(sizeof(float)!=4){
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


//
void 
MD_FileHoraceReader::read_basis(Mantid::Geometry::MDGeometryBasis &)
{
}
//
void 
MD_FileHoraceReader::read_MDGeomDescription(Mantid::Geometry::MDGeometryDescription &)
{
}
   // read DND object data;
void 
MD_FileHoraceReader::read_MDImg_data(MDImage & mdd)
{
}
   
MDPointDescription 
MD_FileHoraceReader::read_pointDescriptions(void)const
{
	MDPointDescription defaultDescr;
	return defaultDescr;
}
 //read whole pixels information in memory; usually impossible, then returns false;
bool 
MD_FileHoraceReader::read_pix(MDDataPoints & sqw)
{
	return false;
}
//
size_t 
MD_FileHoraceReader::read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
{
	return 0;
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

	this->positions.crystal_start = fileStreamHolder.tellg();
	// skip crystal information
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