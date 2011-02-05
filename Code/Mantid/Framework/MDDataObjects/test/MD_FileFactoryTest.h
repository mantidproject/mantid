#ifndef MD_FILE_FACTORY_TEST_H
#define MD_FILE_FACTORY_TEST_H

#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MD_FileFormatFactory.h"
#include <Poco/Path.h>
#include "MantidKernel/System.h"
#include <boost/algorithm/string/case_conv.hpp>

// existing file formats
#include "MDDataObjects/MD_File_hdfV1.h"
#include "MDDataObjects/MD_File_hdfMatlab.h"
#include "MDDataObjects/MD_FileHoraceReader.h"
#include "MDDataObjects/MD_FileTestDataGenerator.h"

using namespace Mantid;
using namespace MDDataObjects;
class MD_FileFactoryTest :    public CxxTest::TestSuite
{

public:
	void testFormatImplemented(){
		std::auto_ptr<IMD_FileFormat> testFormat;
		TSM_ASSERT_THROWS_NOTHING("test data format should be initiated without throwing",testFormat=MD_FileFormatFactory::getFileReader("testFile",test_data));
	
		TSM_ASSERT("FileFormat factory returned a pointer to a wrong file reader, should be test data ",dynamic_cast<MD_FileTestDataGenerator*>(testFormat.get())!=0);	
	}
    void testGetUniqueFileName(){
        std::vector<std::string> f_names(2);
        f_names[0] = "tmp_data_0.sqw";
        f_names[1] = "tmp_data_1.sqw";
        // create temporary files
        for(size_t i=0;i<f_names.size();i++){
            std::ofstream tmp_file(f_names[i].c_str());
            tmp_file.close();
        }
        // get the file name which is not among the above
        std::string new_tmp_file = get_unique_tmp_fileName();

        TSM_ASSERT_EQUALS("next temporary file has to be tmp_data_2.sqw but it is not it","tmp_data_2.sqw",new_tmp_file);
       // delete temporary files (not to leave rubbish)
        for(size_t i=0;i<f_names.size();i++){
            std::remove(f_names[i].c_str());
        }
 
    }
	void testReturnsNewHDFV1format(){
		std::auto_ptr<IMD_FileFormat> newFormat;

        std::string new_sqw_file("newDataFile.sqw");

        TS_ASSERT_THROWS_NOTHING(newFormat=MD_FileFormatFactory::getFileReader(best_fit,new_sqw_file.c_str()));
		TSM_ASSERT("FileFormat factory should returned a pointer to new file format ",dynamic_cast<MD_File_hdfV1*>(newFormat.get())!=0);
        // clear the pointer and deleted test file for future tests not to fail or work independently;
        newFormat.reset();
        std::remove(new_sqw_file.c_str());
	}
	void testReturnsMatlabReader(){
		std::auto_ptr<IMD_FileFormat> oldFormat;
		std::string testFile = findTestFileLocation("../../../../Test/VATES/fe_demo.sqw","fe_demo.sqw");
		TS_ASSERT_THROWS_NOTHING(oldFormat=MD_FileFormatFactory::getFileReader(testFile.c_str()));

		TSM_ASSERT("FileFormat factory returned a pointer to a wrong file reader ",dynamic_cast<MD_File_hdfMatlab*>(oldFormat.get())!=0);
	}
	//void t__tReturnsOldMatlabReader(){
	//	std::auto_ptr<IMD_FileFormat> oldFormat;
	//	std::string testFile = findTestFileLocation("../../../../Test/VATES/fe_demo.sqw","fe_demo.sqw");
	//	TS_ASSERT_THROWS_NOTHING(oldFormat=MD_FileFormatFactory::getFileReader(testFile.c_str(),old_4DMatlabReader));

	//	TSM_ASSERT("FileFormat factory returned a pointer to a wrong file reader ",dynamic_cast<MD_File_hdfMatlab4D*>(oldFormat.get())!=0);
	//}
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
