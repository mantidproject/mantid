#include "MDDataObjects/MD_FileFormatFactory.h"


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
		f_log.error()<<"MD_FileFactory: test file format has not been implemented yet for file: "<<file_name<<std::endl;
		throw(Exception::NotImplementedError("test file format has not been implemented yet"));
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

} // end namespaces
} 