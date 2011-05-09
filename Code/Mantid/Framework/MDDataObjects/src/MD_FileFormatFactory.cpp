#include "MDDataObjects/MD_FileFormatFactory.h"
// existing file formats
#include "MDDataObjects/MD_FileHoraceReader.h"
#include "MDDataObjects/MD_FileTestDataGenerator.h"

#include "MantidKernel/System.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <Poco/File.h>

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
std::auto_ptr<IMD_FileFormat> 
MD_FileFormatFactory::getFileReader(user_request rec,const char *fileName)
{
    if(!pFactory)pFactory = new MD_FileFormatFactory();

    // get test data;
    if(rec == test_data){
        if(fileName){
           return std::auto_ptr<IMD_FileFormat>(pFactory->select_file_reader(fileName,rec));
        }else{
           return std::auto_ptr<IMD_FileFormat>(pFactory->select_file_reader("data4x3_50x50x50x50.sqw",rec));
        }
    }
    // get actual file writer; if input file name exists, open exisgting and try to initiate file reader (writer);
    if(fileName){
          return getFileReader(fileName,rec);
     }
    // if there are no file name given, open a unique temporary file;

    std::string unique_tmp_name = get_unique_tmp_fileName();

    return getFileReader(unique_tmp_name.c_str(),rec);
}
// 
std::string get_unique_tmp_fileName(void){

    std::string defaultFileName("tmp_data_");
    
    int ic(0);
    std::stringstream name_num;
    name_num<<ic;
    std::string file_name = defaultFileName+name_num.str()+".sqw";
    Poco::File tmpFile(file_name);
    while(tmpFile.exists()){
        name_num.seekp(std::ios::beg);
        ic++;
        name_num<<ic;
        file_name = defaultFileName+name_num.str()+".sqw";
        tmpFile = file_name;
    
    } 
    return  file_name;
   
}
//
IMD_FileFormat *
  MD_FileFormatFactory::select_file_reader(const char *file_name,user_request rec)
{
  if(rec == test_data){
    f_log.information()<<"MD_FileFactory: Enabled test file format for the file: "<<file_name<<std::endl;
    return (new MD_FileTestDataGenerator(file_name));
  }

  // check if the file exist;
  std::ifstream infile;
  infile.open(file_name);
  infile.close();
  if (infile.fail())
  {  // new real file can be the new file format only;
    std::ofstream outfile;
    outfile.open(file_name);
    if(outfile.fail()){
      f_log.error()<<"MD_FileFactory: can not open or create file: "<<file_name<<std::endl;
      throw(Exception::FileError("MDData::select_file_reader: Error->can not found or open",file_name));
    }else{
      outfile.close();
    }
  }
  if(isHoraceFile(file_name))
  {
    return (new HoraceReader::MD_FileHoraceReader(file_name));
  }
  else
  {
    throw(Exception::FileError("There is no reader suitable for this file.",file_name));
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
	// version can not be too big
	int version =(int)*(reinterpret_cast<double *>(DataBuffer+10));

	if(version!=2){
		f_log.debug()<<" Only version 2 of Horace format file is currently supported and we got the version :"<<version<<std::endl;
		return false;
	}
	// should be happening automatically anyway;
	aFile.close();
	return true;
}
} // end namespaces
} 
