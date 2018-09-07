#ifndef FILEPROPERTYTEST_H_
#define FILEPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/File.h>
#include <cstdlib>
#include <fstream>

using Mantid::API::FileFinder;
using Mantid::API::FileProperty;
using Mantid::Kernel::ConfigService;

class FilePropertyTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    // Ensure we have the correct facility set up
    const std::string xmlStr =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<facilities>"
        "  <facility name=\"ISIS\" zeropadding=\"5\" "
        "FileExtensions=\".nxs,.raw,.sav,.n*,.s*\">"
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
    ConfigService::Instance().setString("default.instrument", "LOQ");

    // We need to specify the default facility to make sure that there isn't a
    // default
    // facility set in the Mantid.properties that is not in the Facility XML
    // above.
    ConfigService::Instance().setFacility("ISIS");

    Poco::File(facilityFilePath).remove();
  }

  void testSearchDirs() {
    TS_ASSERT_DIFFERS(ConfigService::Instance().getDataSearchDirs().size(), 0);
  }

  void testLoadPropertyWithEmptyValueIsInvalid() {
    FileProperty fp("Filename", "", FileProperty::Load);
    doPropertyTraitTests(fp, true, false, false);
    std::string msg = fp.setValue("");
    TS_ASSERT_DIFFERS("", msg);
  }

  void testLoadPropertyNoExtension() {
    FileProperty fp("Filename", "", FileProperty::Load);
    doPropertyTraitTests(fp, true, false, false);
    doExtensionCheck(fp, "", "LOQ48127.raw", "48098.Q");
  }

  void testLoadPropertyWithExtensionAsVector() {
    std::vector<std::string> exts(1, ".raw");
    FileProperty fp("Filename", "", FileProperty::Load, exts);
    doPropertyTraitTests(fp, true, false, false);
    doExtensionCheck(fp, ".raw", "LOQ48127.raw", "48098.Q");
  }

  void testLoadPropertyWithExtensionAsString() {
    std::string ext(".raw");
    FileProperty fp("Filename", "", FileProperty::Load, ext);
    doPropertyTraitTests(fp, true, false, false);
    doExtensionCheck(fp, ".raw", "LOQ48127.raw", "48098.Q");
  }

  void testLoadPropertyWithExtensionAsInitializerList() {
    FileProperty fp("Filename", "", FileProperty::Load, {".raw"});
    doPropertyTraitTests(fp, true, false, false);
    doExtensionCheck(fp, ".raw", "LOQ48127.raw", "48098.Q");
  }

  void testOptionalLoadProperty() {
    std::vector<std::string> exts(1, "raw");
    FileProperty fp("Filename", "", FileProperty::OptionalLoad, exts);
    doPropertyTraitTests(fp, true, false, true);

    std::string msg = fp.setValue("LOQ48127.raw");
    TS_ASSERT_EQUALS(msg, "");
    // I'm using part of the file's path to check that the property really has
    // found the file, with OptionalLoad the property returns valid whether it
    // finds the file or not
    TS_ASSERT(fp.value().find("UnitTest") != std::string::npos);
    // do this in parts making no assumptions about the identity of the slash
    // that separates directories
    TS_ASSERT(fp.value().find("Test") != std::string::npos);

    msg = fp.setValue("LOQ48127.raw");
    TS_ASSERT_EQUALS(msg, "");

    msg = fp.setValue("");
    TS_ASSERT_EQUALS(msg, "");
    TS_ASSERT_EQUALS(fp.value(), "");
  }

  void testSaveProperty() {
    FileProperty fp("Filename", "", FileProperty::Save);
    doPropertyTraitTests(fp, false, true, false);
    // Test for some random file name as this doesn't need to exist here
    std::string msg = fp.setValue("filepropertytest.sav");
    TS_ASSERT_EQUALS(msg, "");
  }

  void testOptionalSaveProperty() {
    FileProperty fp("Filename", "", FileProperty::OptionalSave);
    doPropertyTraitTests(fp, false, true, true);
    // Test for some random file name as this doesn't need to exist here
    std::string msg = fp.setValue("filepropertytest.sav");
    TS_ASSERT_EQUALS(msg, "");
  }

  void testThatRunNumberReturnsFileWithCorrectPrefix() {
    FileProperty fp("Filename", "", FileProperty::Load,
                    std::vector<std::string>(1, ".raw"));
    std::string error = fp.setValue("48127");
    TS_ASSERT_EQUALS(error, "");
    TS_ASSERT_DIFFERS(fp.value().find("LOQ48127"), std::string::npos);

    // Now test one with an upper case extension
    auto &fileFinder = Mantid::API::FileFinder::Instance();
    const bool startingCaseOption = fileFinder.getCaseSensitive();
    // By default case sensitive is on
    fileFinder.setCaseSensitive(false);

    ConfigService::Instance().setString("default.instrument", "LOQ");
    error = fp.setValue("25654");
    TS_ASSERT_EQUALS(error, "");
    TS_ASSERT_DIFFERS(fp.value().find("LOQ25654"), std::string::npos);

    fileFinder.setCaseSensitive(startingCaseOption);
  }

  void testOptionalDirectory() {
    FileProperty fp("SavePath", "", FileProperty::OptionalDirectory);
    // Check type
    TS_ASSERT_EQUALS(fp.isDirectoryProperty(), true);

    // Test for some random directory that does not need to exist
    std::string msg = fp.setValue("my_nonexistent_folder");
    TS_ASSERT_EQUALS(msg, "");
  }

  void testDirectoryFailsIfNonExistent() {
    FileProperty fp("SavePath", "", FileProperty::Directory);
    // This will fail because this folder does not exist
    std::string TestDir("my_nonexistent_folder");
    std::string msg = fp.setValue(TestDir);
    // It gives an error message starting "Directory "X" not found".
    auto pos = msg.find("Directory");
    TS_ASSERT(pos != std::string::npos);
    // "not found" comes after "Directory"
    TS_ASSERT(msg.find("not found", pos) !=
              std::string::npos); //.substr(0, 3), "Dir");
  }

  void testDirectoryPasses() {
    std::string TestDir(ConfigService::Instance().getDirectoryOfExecutable() +
                        "MyTestFolder");
    Poco::File dir(TestDir);
    dir.createDirectory();

    FileProperty fp("SavePath", "", FileProperty::Directory);
    TS_ASSERT_EQUALS(fp.isDirectoryProperty(), true);

    // The directory exists, so no failure
    std::string msg = fp.setValue(TestDir);
    TS_ASSERT_EQUALS(msg, "");

    dir.remove(); // clean up your folder
  }

  void testExpandUserVariables() {
    FileProperty fp("Dir", "", FileProperty::Directory);
    std::string msg = fp.setValue("~");

    char *homepath = std::getenv("HOME");
    if (!homepath) {
      homepath = std::getenv("USERPROFILE");
    }

    if (homepath && Poco::File(homepath).exists()) {
      // User home variable is set and points to a valid directory
      // We should have no errors
      TS_ASSERT(msg.empty());
    } else {
      // No user variables were set, so we should have an error
      TS_ASSERT(!msg.empty());
    }
  }

private:
  void doPropertyTraitTests(const FileProperty &fileProp, const bool loadProp,
                            const bool saveProp, const bool validByDefault) {
    // Check type
    TS_ASSERT_EQUALS(fileProp.isLoadProperty(), loadProp);
    TS_ASSERT_EQUALS(fileProp.isSaveProperty(), saveProp);
    if (validByDefault) {
      TS_ASSERT_EQUALS(fileProp.isValid(), "");
    } else {
      TS_ASSERT_DIFFERS(fileProp.isValid(), "");
    }
  }

  void doExtensionCheck(FileProperty &fileProp, const std::string &defaultExt,
                        const std::string &match, const std::string &nomatch) {
    TS_ASSERT_EQUALS(fileProp.getDefaultExt(), defaultExt);
    // Filename as given
    std::string msg = fileProp.setValue(match);
    TS_ASSERT_EQUALS(msg, "");

    // full path
    msg = fileProp.setValue(fileProp.value());
    TS_ASSERT_EQUALS(msg, "");

    // Extension is not mandatory so different also passes
    msg = fileProp.setValue(nomatch);
    TS_ASSERT_EQUALS(msg, "");
  }
};

#endif
