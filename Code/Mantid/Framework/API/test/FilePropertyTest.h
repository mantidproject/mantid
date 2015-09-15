#ifndef FILEPROPERTYTEST_H_
#define FILEPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FileFinder.h"
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
        // Ensure we have the correct facility set up
        const std::string xmlStr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<facilities>"
      "  <facility name=\"ISIS\" zeropadding=\"5\" FileExtensions=\".nxs,.raw,.sav,.n*,.s*\">"
      "    <archive>"
      "      <archiveSearch plugin=\"ISISDataSearch\" />"
      "    </archive>"
      "    <instrument name=\"LOQ\" shortname=\"LOQ\">"
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
    ConfigService::Instance().setString("default.instrument","LOQ");

    // We need to specify the default facility to make sure that there isn't a default
    // facility set in the Mantid.properties that is not in the Facility XML above.
    ConfigService::Instance().setFacility("ISIS");

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
    doPropertyTraitTests(*fp, true, false,false);
    TS_ASSERT_EQUALS(fp->getDefaultExt(), "");

    ///Test a file in the test directory
    const std::string test_file = "LOQ48127.raw";
    std::string msg = fp->setValue(test_file);
    TS_ASSERT_EQUALS(msg, "");

    // Absolute path
    msg = fp->setValue(fp->value());
    TS_ASSERT_EQUALS(msg, "");

    delete fp;
  }

  void testLoadPropertyWithExtension()
  {
    std::vector<std::string> exts(1, "raw");
    Mantid::API::FileProperty *fp = 
      new Mantid::API::FileProperty("Filename","", Mantid::API::FileProperty::Load, exts);
    doPropertyTraitTests(*fp, true, false,false);

    TS_ASSERT_EQUALS(fp->getDefaultExt(), "raw");

    ///Test a GEM file in the test directory
    std::string msg = fp->setValue("LOQ48127.raw");
    TS_ASSERT_EQUALS(msg, "");

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
    doPropertyTraitTests(*fp, true, false,true);

    std::string msg = fp->setValue("LOQ48127.raw");
    TS_ASSERT_EQUALS(msg, "");
    // I'm using part of the file's path to check that the property really has found the file, with OptionalLoad the property returns valid whether it finds the file or not
    TS_ASSERT(fp->value().find("UnitTest") != std::string::npos);
    // do this in parts making no assumptions about the identity of the slash that separates directories
    TS_ASSERT(fp->value().find("Test") != std::string::npos);

    msg = fp->setValue("LOQ48127.raw");
    TS_ASSERT_EQUALS(msg, "");

    msg = fp->setValue("");
    TS_ASSERT_EQUALS(msg, "");
    TS_ASSERT_EQUALS(fp->value(), "");

    delete fp;
  }

  void testSaveProperty()
  {
    Mantid::API::FileProperty *fp = 
      new Mantid::API::FileProperty("Filename","", Mantid::API::FileProperty::Save);
    doPropertyTraitTests(*fp, false ,true,false);
    //Test for some random file name as this doesn't need to exist here
    std::string msg = fp->setValue("filepropertytest.sav");
    TS_ASSERT_EQUALS(msg, "");
    
    delete fp;
  }
  
  void testOptionalSaveProperty()
   {
     Mantid::API::FileProperty *fp =
       new Mantid::API::FileProperty("Filename","", Mantid::API::FileProperty::OptionalSave);
     doPropertyTraitTests(*fp, false ,true, true);
     //Test for some random file name as this doesn't need to exist here
     std::string msg = fp->setValue("filepropertytest.sav");
     TS_ASSERT_EQUALS(msg, "");

     delete fp;
   }

  void doPropertyTraitTests(const Mantid::API::FileProperty & fileProp,
       const bool loadProp, const bool saveProp, const bool validByDefault)
  {
    // Check type
    TS_ASSERT_EQUALS(fileProp.isLoadProperty(), loadProp);
    TS_ASSERT_EQUALS(fileProp.isSaveProperty(), saveProp);
    if(validByDefault)
    {
      TS_ASSERT_EQUALS(fileProp.isValid(), "");
    }
    else
    {
      TS_ASSERT_DIFFERS(fileProp.isValid(), "");
    }
  }

  void testThatRunNumberReturnsFileWithCorrectPrefix()
  {
    Mantid::API::FileProperty fp("Filename","", Mantid::API::FileProperty::Load,
                                    std::vector<std::string>(1, ".raw"));
    std::string error = fp.setValue("48127");
    TS_ASSERT_EQUALS(error, "");
    TS_ASSERT_DIFFERS(fp.value().find("LOQ48127"),std::string::npos);
    
    // Now test one with an upper case extension
    auto & fileFinder = Mantid::API::FileFinder::Instance();
    const bool startingCaseOption = fileFinder.getCaseSensitive();
    // By default case sensitive is on
    fileFinder.setCaseSensitive(false);

    ConfigService::Instance().setString("default.instrument","LOQ");
    error = fp.setValue("25654");
    TS_ASSERT_EQUALS(error, "");
    TS_ASSERT(fp.value().find("LOQ25654") != std::string::npos);

    fileFinder.setCaseSensitive(startingCaseOption);
  }

  void testOptionalDirectory()
  {
    Mantid::API::FileProperty *fp =
      new Mantid::API::FileProperty("SavePath","", Mantid::API::FileProperty::OptionalDirectory);
    // Check type
    TS_ASSERT_EQUALS(fp->isDirectoryProperty(), true);

    //Test for some random directory that does not need to exist
    std::string msg = fp->setValue("my_nonexistent_folder");
    TS_ASSERT_EQUALS(msg, "");

    delete fp;
  }

  void testDirectoryFailsIfNonExistent()
  {
    Mantid::API::FileProperty *fp =
      new Mantid::API::FileProperty("SavePath","", Mantid::API::FileProperty::Directory);
    //This will fail because this folder does not exist
    std::string TestDir("my_nonexistent_folder");
    std::string msg = fp->setValue(TestDir);
    //It gives an error message starting "Directory "X" not found".
    TS_ASSERT_EQUALS(msg.substr(0,3), "Dir");

    delete fp;
  }

  void testDirectoryPasses()
  {
    std::string TestDir(ConfigService::Instance().getDirectoryOfExecutable() + "MyTestFolder");
    Poco::File dir(TestDir);
    dir.createDirectory();

    Mantid::API::FileProperty *fp =
      new Mantid::API::FileProperty("SavePath","", Mantid::API::FileProperty::Directory);
    TS_ASSERT_EQUALS(fp->isDirectoryProperty(), true);

    //The directory exists, so no failure
    std::string msg = fp->setValue(TestDir);
    TS_ASSERT_EQUALS(msg, "");

    delete fp;
    dir.remove(); //clean up your folder
  }
};

#endif
