#include "MDDataObjects/MD_FileFormatFactory.h"
// existing file formats
#include "MDDataObjects/MD_File_hdfV1.h"
#include "MDDataObjects/MD_File_hdfMatlab.h"
#include "MDDataObjects/MD_File_hdfMatlab4D.h"
#include "MDDataObjects/MD_FileHoraceReader.h"
#include "MDDataObjects/MD_FileTestDataGenerator.h"

#include "MantidKernel/System.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace Mantid
{
namespace MDDataObjects
{
	using namespace Kernel;

MD_FileFormatFactory* MD_FileFormatFactory::pFactory=NULL;

Kernel::Logger& MD_FileFormatFactory::f_log=Kernel::Logger::get("IMD_fileOperations");
//--------------------------------------------------------------------------------------
MD_FileFormatFactory::MD_FileFormatFactory()
{
}
//
MD_FileFormatFactory::~MD_FileFormatFactory()
{
	if(pFactory)delete pFactory;
	pFactory = NULL;
}
//
std::auto_ptr<IMD_FileFormat> 
MD_FileFormatFactory::getFileReader(const char *fileName,user_request rec)
{
	if(!pFactory)pFactory = new MD_FileFormatFactory();
	return std::auto_ptr<IMD_FileFormat>(pFactory->select_file_reader(fileName,rec));
}
//
IMD_FileFormat *
MD_FileFormatFactory::select_file_reader(const char *file_name,user_request rec)
{
	if(rec == test_data){
		f_log.error()<<"MD_FileFactory: Enabled test file format for the file: "<<file_name<<std::endl;
        return (new MD_FileTestDataGenerator(file_name));
	}

// check if the file exist;
    std::ifstream infile;
    infile.open(file_name);
    infile.close();
    if (infile.fail()){
		std::ofstream outfile;
		outfile.open(file_name);
		if(outfile.fail()){
			f_log.error()<<"MD_FileFactory: can not find or create file: "<<file_name<<std::endl;
			throw(Exception::FileError("MDData::select_file_reader: Error->can not found or open",file_name));
		}else{
			outfile.close();
			std::remove(file_name);
			return (new MD_File_hdfV1(file_name));
		}
    }
// check existing file is hdf5 file;
    htri_t rez=H5Fis_hdf5(file_name);
    if (rez<=0){
        if (rez==0){
			if(isHoraceFile(file_name)){
				return (new HoraceReader::MD_FileHoraceReader(file_name));
			}
			f_log.error()<<" HDF5 error dealing with file"<<file_name<<std::endl;
            throw(Exception::FileError("MDData::select_file_reader: Error->the file is not hdf5 file",file_name));
        }else{
			f_log.error()<<" HDF5 error dealing with file"<<file_name<<std::endl;
            throw(Exception::FileError("MDData::select_file_reader: Error->unspecified hdf5 error ",file_name));
        }
    }else{
        // ***> to do:: identify internal hdf5 format; only MATLAB is supported at the moment;
      if(rec == old_4DMatlabReader){
          return (new MD_File_hdfMatlab4D(file_name));
      }else{
         return (new MD_File_hdfMatlab(file_name));
      }
    }
}
// 
bool 
MD_FileFormatFactory::isHoraceFile(const char *fileName)
{
	std::ifstream aFile(fileName);
	if(aFile.bad()){
		f_log.error()<< "attempt to open existing file"<<fileName<<" for reading have failed\n";
		std::string errorName(fileName);
		throw(Kernel::Exception::FileError(" can not open existing file to check if it Horace written",errorName));
	}
	char DataBuffer[4+6+8];
	aFile.read(DataBuffer,4+6+8);
	if(aFile.bad()){
		f_log.debug()<< "Can not read first 18 bytes of data from existing binary file: "<<fileName<<" It is probably not a Horace file\n";
		return false; // probably not Horace file
	}


	int n_symbols= *(reinterpret_cast<uint32_t*>(DataBuffer));
	if(n_symbols!=6){
		f_log.debug()<<" first number of the file header is not 6, It is probably not a Horace file\n";
		return false;
	}
	// 6 symbols starting from 4-th 
	std::string buf(DataBuffer+4,6);
	if(buf != "horace"){
		f_log.debug()<<" the program name is not a Horace, definitely not a Horace file\n";
		return false;
	}

	double version = *(reinterpret_cast<double *>(DataBuffer+10));

	if(abs(version-2)>std::numeric_limits<float>::epsilon()){
		f_log.debug()<<" Only version 2 of Horace format file is currently supported and we got the version :"<<version<<std::endl;
		return false;
	}
	// should be happening automatically anyway;
	aFile.close();
	return true;
}
} // end namespaces
} 
