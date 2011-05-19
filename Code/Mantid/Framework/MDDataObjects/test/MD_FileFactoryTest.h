#ifndef MD_FILE_FACTORY_TEST_H
#define MD_FILE_FACTORY_TEST_H

#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MD_FileFormatFactory.h"
#include "MantidAPI/FileFinder.h"
#include <Poco/Path.h>
#include <boost/algorithm/string/case_conv.hpp>

// existing file formats
#include "MDDataObjects/MD_FileHoraceReader.h"
#include "MDDataObjects/MD_FileTestDataGenerator.h"

using namespace Mantid;
using namespace MDDataObjects;
class MD_FileFactoryTest :    public CxxTest::TestSuite
{

public:
  void testFormatImplemented(){
    std::auto_ptr<IMD_FileFormat> testFormat;
    TSM_ASSERT_THROWS_NOTHING("test default data format should be initiated without throwing",testFormat=MD_FileFormatFactory::getFileReader("testFile",test_data));

    TSM_ASSERT("FileFormat factory returned a pointer to a wrong file reader, should be test data ",dynamic_cast<MD_FileTestDataGenerator*>(testFormat.get())!=0);	
  }
  void testTestFileFormatDataProvided(){
    std::auto_ptr<IMD_FileFormat> testFormat;
	std::auto_ptr<Geometry::MDGeometryDescription> pGeomDescr = std::auto_ptr<Geometry::MDGeometryDescription>
		                                                        (new Geometry::MDGeometryDescription(6,3));
	TSM_ASSERT_THROWS_NOTHING("test default data format should be initiated without throwing",testFormat=MD_FileFormatFactory::getFileReader("testFile",test_data,pGeomDescr.get()));

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

  //void t__tReturnsOldMatlabReader(){
  //	std::auto_ptr<IMD_FileFormat> oldFormat;
  //	std::string testFile = findTestFileLocation("../../../../Test/VATES/fe_demo.sqw","fe_demo.sqw");
  //	TS_ASSERT_THROWS_NOTHING(oldFormat=MD_FileFormatFactory::getFileReader(testFile.c_str(),old_4DMatlabReader));

  //	TSM_ASSERT("FileFormat factory returned a pointer to a wrong file reader ",dynamic_cast<MD_File_hdfMatlab4D*>(oldFormat.get())!=0);
  //}
  void testHoraceFileFound(){
    std::auto_ptr<IMD_FileFormat> horaceFormat;
    std::string testFile = API::FileFinder::Instance().getFullPath("test_horace_reader.sqw");

    TS_ASSERT_THROWS_NOTHING(horaceFormat= MD_FileFormatFactory::getFileReader(testFile.c_str()));

    TSM_ASSERT("FileFormat factory have not returned a pointer to a Horace file reader ",dynamic_cast<HoraceReader::MD_FileHoraceReader*>(horaceFormat.get())!=0);
  }
  ~MD_FileFactoryTest(){
  }

};


#endif
