#ifndef MD_FILE_FACTORY_TEST_H
#define MD_FILE_FACTORY_TEST_H

#include <cxxtest/TestSuite.h>
#include <MDDataObjects/MD_FileFormatFactory.h>
#include "Poco/Path.h"

#include "MantidKernel/System.h"

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
		std::string testFile = findTestFileLocation();
		TS_ASSERT_THROWS_NOTHING(oldFormat=MD_FileFormatFactory::getFileReader(testFile.c_str()));

		TSM_ASSERT("FileFormat factory returned a pointer to a wrong file reader ",dynamic_cast<MD_File_hdfMatlab*>(oldFormat.get())!=0);
	}
	void testReturnsOldMatlabReader(){
		std::auto_ptr<IMD_FileFormat> oldFormat;
		std::string testFile = findTestFileLocation();
		TS_ASSERT_THROWS_NOTHING(oldFormat=MD_FileFormatFactory::getFileReader(testFile.c_str(),old_4DMatlabReader));

		TSM_ASSERT("FileFormat factory returned a pointer to a wrong file reader ",dynamic_cast<MD_File_hdfMatlab4D*>(oldFormat.get())!=0);
	}
private:
  std::string findTestFileLocation(void){

    std::string path = Mantid::Kernel::getDirectoryOfExecutable();

    char pps[2];
    pps[0]=Poco::Path::separator();
    pps[1]=0; 
    std::string sps(pps);

    std::string root_path;
    size_t   nPos = path.find("Mantid"+sps+"Code");
    if(nPos==std::string::npos){
      std::cout <<" can not identify application location\n";
      root_path="../../../../Test/VATES/fe_demo.sqw";
    }else{
      root_path=path.substr(0,nPos)+"Mantid/Test/VATES/fe_demo.sqw";
    }
    std::cout << " test file location: "<< root_path<< std::endl;
    return root_path;
  }
};


#endif
