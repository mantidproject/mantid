#ifndef MD_FILE_FACTORY_TEST_H
#define MD_FILE_FACTORY_TEST_H

#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MD_FileFormatFactory.h"
#include "Poco/Path.h"
#include "MantidKernel/System.h"
#include <boost/algorithm/string/case_conv.hpp>

// existing file formats
#include "MDDataObjects/MD_File_hdfV1.h"
#include "MDDataObjects/MD_File_hdfMatlab.h"
#include "MDDataObjects/MD_File_hdfMatlab4D.h"
#include "MDDataObjects/MD_FileHoraceReader.h"

using namespace Mantid;
using namespace MDDataObjects;
class MD_FileFactoryTest :    public CxxTest::TestSuite
{

public:
	void testFormatNotImplemented(){
		// not implemented at the moment
		TS_ASSERT_THROWS(MD_FileFormatFactory::getFileReader("testFile",test_data),Kernel::Exception::NotImplementedError);
	}
	void testReturnsNewHDFV1format(){
		std::auto_ptr<IMD_FileFormat> newFormat;
		// new file format has not been implemented so throws rubbish
		try
		{
		  MD_FileFormatFactory::getFileReader("testFile");
		  TSM_ASSERT("MD_FileFormatFactory::getFileReader() should have thrown.", false);
		}
		catch (...)
		{
		  //Some kind of exception thrown. good.
		}

		//TS_ASSERT_THROWS_NOTHING(newFormat=MD_FileFormatFactory::getFileReader("testFile"));
		//TSM_ASSERT("FileFormat factory returned a pointer to a wrong file format ",dynamic_cast<MD_File_hdfV1*>(newFormat.get())!=0);
	}
	void testReturnsMatlabReader(){
		std::auto_ptr<IMD_FileFormat> oldFormat;
		std::string testFile = findTestFileLocation("../../../../Test/VATES/fe_demo.sqw","fe_demo.sqw");
		TS_ASSERT_THROWS_NOTHING(oldFormat=MD_FileFormatFactory::getFileReader(testFile.c_str()));

		TSM_ASSERT("FileFormat factory returned a pointer to a wrong file reader ",dynamic_cast<MD_File_hdfMatlab*>(oldFormat.get())!=0);
	}
	void testReturnsOldMatlabReader(){
		std::auto_ptr<IMD_FileFormat> oldFormat;
		std::string testFile = findTestFileLocation("../../../../Test/VATES/fe_demo.sqw","fe_demo.sqw");
		TS_ASSERT_THROWS_NOTHING(oldFormat=MD_FileFormatFactory::getFileReader(testFile.c_str(),old_4DMatlabReader));

		TSM_ASSERT("FileFormat factory returned a pointer to a wrong file reader ",dynamic_cast<MD_File_hdfMatlab4D*>(oldFormat.get())!=0);
	}
	void testHoraceFileFound(){
		std::auto_ptr<IMD_FileFormat> horaceFormat;
		std::string testFile = findTestFileLocation("../../../../Test/VATES/fe_demo_bin.sqw","fe_demo_bin.sqw");

		TS_ASSERT_THROWS_NOTHING(horaceFormat= MD_FileFormatFactory::getFileReader(testFile.c_str()));

		TSM_ASSERT("FileFormat factory have not returned a pointer to a Horace file reader ",dynamic_cast<HoraceReader::MD_FileHoraceReader*>(horaceFormat.get())!=0);
	}
private:
  std::string findTestFileLocation(const char *filePath,const std::string &fileName){

    std::string search_path = Mantid::Kernel::getDirectoryOfExecutable();
	std::string real_path   = search_path;
	boost::to_upper(search_path);

	char pps[2];
    pps[0]=Poco::Path::separator();
    pps[1]=0; 
    std::string sps(pps);

    std::string root_path;
    size_t   nPos = search_path.find("MANTID"+sps+"CODE");

	if(nPos==std::string::npos){
      std::cout <<" can not identify application location\n";
	  root_path.assign(filePath);
    }else{
      root_path=real_path.substr(0,nPos)+"Mantid/Test/VATES/"+fileName;
    }
    std::cout << "\n\n test file location: "<< root_path<< std::endl;
    return root_path;
  }
};


#endif
