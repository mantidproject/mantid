#ifndef FILEPROPERTYTEST_H_
#define FILEPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/Path.h>
#include <Poco/File.h>
#include <fstream>

using namespace Mantid::Kernel;

class FilePropertyTest : public CxxTest::TestSuite
{
public:
  
  void setUp()
  {
    ConfigService::Instance().updateConfig("Mantid.properties");
        // Ensure we have the correct facility set up
        const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"ISIS\" zeropadding=\"5\" FileExtensions=\".nxs,.raw,.sav,.n*,.s*\">"
      "    <archive>"
      "      <archiveSearch plugin=\"ISISDataSearch\" />"
      "    </archive>"
      "    <instrument name=\"GEM\" shortname=\"GEM\">"
      "      <technique>technique</technique>"
      "    </instrument>"
      "    <instrument name=\"ALF\" shortname=\"ALF\">"
      "      <technique>technique</technique>"
      "    </instrument>"

      "  </facility>"
      "</facilities>";

    const std::string facilityFilePath("FilePropertyTest_Facilities.xml");
    std::ofstream facilityFile(facilityFilePath.c_str());
    facilityFile << xmlStr;
    facilityFile.close();

    ConfigService::Instance().updateFacilities(facilityFilePath);
    ConfigService::Instance().setString("default.instrument","GEM");
    ConfigService::Instance().setString("default.facility","ISIS");
    
    Poco::File(facilityFilePath).remove();
  }

  void testSearchDirs()
  {
    TS_ASSERT_DIFFERS(ConfigService::Instance().getDataSearchDirs().size(), 0);
  }

  void testLoadPropertyNoExtension()
  {
    Mantid::API::FileProperty *fp = 
      new Mantid::API::FileProperty("Filename","", Mantid::API::FileProperty::Load);

    // Check type
    TS_ASSERT_EQUALS(fp->isLoadProperty(), true)
    TS_ASSERT_EQUALS(fp->getDefaultExt(), "")

    ///Test a GEM file in the test directory
    const std::string test_file = "GEM38370.raw";
    std::string msg = fp->setValue(test_file);
    TS_ASSERT_EQUALS(msg, "")

    // Absolute path
    Poco::Path test_dir = Poco::Path("../../../../Test/AutoTestData/").absolute();
    msg = fp->setValue(test_dir.resolve(Poco::Path(test_file)).toString());
    TS_ASSERT_EQUALS(msg, "")    

    delete fp;
  }

  void testLoadPropertyWithExtension()
  {
    std::vector<std::string> exts(1, "raw");
    Mantid::API::FileProperty *fp = 
      new Mantid::API::FileProperty("Filename","", Mantid::API::FileProperty::Load, exts);
    // Check type
    TS_ASSERT_EQUALS(fp->isLoadProperty(), true)
    TS_ASSERT_EQUALS(fp->isOptional(), false)
    TS_ASSERT_EQUALS(fp->getDefaultExt(), "raw")

    ///Test a GEM file in the test directory
    std::string msg = fp->setValue("GEM38370.raw");
    TS_ASSERT_EQUALS(msg, "")    
    msg = fp->setValue("ALF15739.raw");
    TS_ASSERT_EQUALS(msg, "")    

    //Check different extension
    msg = fp->setValue("48098.Q");
    TS_ASSERT_EQUALS(msg, "");
    
    delete fp;
    fp = new Mantid::API::FileProperty("Filename","", Mantid::API::FileProperty::Load, exts);
    //Check empty value
    msg = fp->setValue("");
    TS_ASSERT_EQUALS(fp->value(), "");
    TS_ASSERT_EQUALS(msg, "No file specified.");

    delete fp;
  }

  void testOptionalLoadProperty()
  {
    std::vector<std::string> exts(1, "raw");
    Mantid::API::FileProperty *fp = 
      new Mantid::API::FileProperty("Filename","", Mantid::API::FileProperty::OptionalLoad, exts);
    // Check type
    TS_ASSERT_EQUALS(fp->isLoadProperty(), true)
    TS_ASSERT_EQUALS(fp->isOptional(), true)

    std::string msg = fp->setValue("GEM38370.raw");
    TS_ASSERT_EQUALS(msg, "")    
    // I'm using part of the file's path to check that the property really has found the file, with OptionalLoad the property returns valid whether it finds the file or not
    TS_ASSERT(fp->value().find("Data") != std::string::npos)
    // do this in parts making no assumptions about the identity of the slash that separates directories
    TS_ASSERT(fp->value().find("Test") != std::string::npos)

    msg = fp->setValue("GEM38371.raw");
    TS_ASSERT_EQUALS(msg, "")

    msg = fp->setValue("");
    TS_ASSERT_EQUALS(msg, "")
    TS_ASSERT_EQUALS(fp->value(), "");

    delete fp;
  }

  void testSaveProperty()
  {
    Mantid::API::FileProperty *fp = 
      new Mantid::API::FileProperty("Filename","", Mantid::API::FileProperty::Save);
    // Check type
    TS_ASSERT_EQUALS(fp->isLoadProperty(), false)

    //Test for some random file name as this doesn't need to exist here
    std::string msg = fp->setValue("filepropertytest.sav");
    TS_ASSERT_EQUALS(msg, "")
    
    delete fp;
  }
  
  void testThatRunNumberReturnsFileWithCorrectPrefix()
  {
    Poco::Path test_dir = Poco::Path("../../../../Test/AutoTestData/").absolute();
    Poco::Path test_file = test_dir.resolve("GEM38370.raw");
    

    Mantid::API::FileProperty *fp = 
      new Mantid::API::FileProperty("Filename","", Mantid::API::FileProperty::Load, 
				    std::vector<std::string>(1, ".raw"));
    std::string error = fp->setValue("38370");
    TS_ASSERT_EQUALS(error, "");
    TS_ASSERT_EQUALS(test_file.toString(), fp->value());
    
    // Now test one with an upper case extension
    ConfigService::Instance().setString("default.instrument","ALF");
    error = fp->setValue("15739");
    TS_ASSERT_EQUALS(error, "");
    TS_ASSERT(fp->value().find("ALF15739") != std::string::npos);
  }

};

#endif
