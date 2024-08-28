// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/TestChannel.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>
#include <memory>
#include <string>

#include <Poco/NObserver.h>

using namespace Mantid::Kernel;
using Mantid::TestChannel;

class ConfigServiceTest : public CxxTest::TestSuite {
public:
  void testLogging() {

    // Force the setting to "notice" in case the developer uses a different
    // level.
    Mantid::Kernel::Logger::setLevelForAll(Poco::Message::PRIO_NOTICE);

    // attempt some logging
    Logger log1("logTest");

    TS_ASSERT_THROWS_NOTHING(log1.debug("a debug string"));
    TS_ASSERT_THROWS_NOTHING(log1.information("an information string"));
    TS_ASSERT_THROWS_NOTHING(log1.information("a notice string"));
    TS_ASSERT_THROWS_NOTHING(log1.warning("a warning string"));
    TS_ASSERT_THROWS_NOTHING(log1.error("an error string"));
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string"));

    TS_ASSERT_THROWS_NOTHING(log1.fatal() << "A fatal message from the stream operators " << 4.5 << '\n';
                             log1.error() << "A error message from the stream operators " << -0.2 << '\n';
                             log1.warning() << "A warning message from the stream operators " << 999.99 << '\n';
                             log1.notice() << "A notice message from the stream operators " << 0.0 << '\n';
                             log1.information()
                             << "A information message from the stream operators " << -999.99 << '\n';
                             log1.debug() << "A debug message from the stream operators " << 5684568 << '\n';

    );

    // checking the level - this is set above
    TS_ASSERT(log1.is(Poco::Message::PRIO_DEBUG) == false);       // debug
    TS_ASSERT(log1.is(Poco::Message::PRIO_INFORMATION) == false); // information
    TS_ASSERT(log1.is(Poco::Message::PRIO_NOTICE));               // notice
    TS_ASSERT(log1.is(Poco::Message::PRIO_WARNING));              // warning
    TS_ASSERT(log1.is(Poco::Message::PRIO_ERROR));                // error
    TS_ASSERT(log1.is(Poco::Message::PRIO_FATAL));                // fatal
  }

  void testEnabled() {
    // attempt some logging
    Logger log1("logTestEnabled");
    TS_ASSERT(log1.getEnabled());
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string with enabled=true"));
    TS_ASSERT_THROWS_NOTHING(log1.fatal()
                                 << "A fatal message from the stream operators with enabled=true " << 4.5 << '\n';);

    TS_ASSERT_THROWS_NOTHING(log1.setEnabled(false));
    TS_ASSERT(!log1.getEnabled());
    TS_ASSERT_THROWS_NOTHING(log1.fatal("YOU SHOULD NEVER SEE THIS"));
    TS_ASSERT_THROWS_NOTHING(log1.fatal() << "YOU SHOULD NEVER SEE THIS VIA A STREAM\n";);

    TS_ASSERT_THROWS_NOTHING(log1.setEnabled(true));
    TS_ASSERT(log1.getEnabled());
    TS_ASSERT_THROWS_NOTHING(log1.fatal("you are allowed to see this"));
    TS_ASSERT_THROWS_NOTHING(log1.fatal() << "you are allowed to see this via a stream\n";);
  }

  void testLogLevelOffset() {
    // attempt some logging
    Logger log1("logTestOffset");
    log1.setLevelOffset(0);
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string with offset 0"));
    log1.setLevelOffset(-1);
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string with offset -1 should still be fatal"));
    TS_ASSERT_THROWS_NOTHING(log1.information("a information string with offset -1 should be notice"));
    log1.setLevelOffset(1);
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string with offset 1 should be critical"));
    TS_ASSERT_THROWS_NOTHING(log1.notice("a notice string with offset 1 should be information"));
    TS_ASSERT_THROWS_NOTHING(log1.debug("a debug string with offset 1 should be debug"));
    log1.setLevelOffset(999);
    TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string with offset 999 should  be trace"));
    TS_ASSERT_THROWS_NOTHING(log1.notice("a notice string with offset 999 should be trace"));
    TS_ASSERT_THROWS_NOTHING(log1.debug("a debug string with offset 999 should be trace"));
  }

  void testLogLevelChanges() {
    Logger log1("testLogLevelChangesWithFilteringLevels");
    TS_ASSERT_THROWS_NOTHING(ConfigService::Instance().setLogLevel(4));
    TSM_ASSERT("The log level should be 4 after the filters are set to 4", log1.is(4));

    TS_ASSERT_THROWS_NOTHING(ConfigService::Instance().setLogLevel(3));
    TSM_ASSERT("The log level should be 3 after the filters are set to 3", log1.is(3));

    // return back to previous values
    TS_ASSERT_THROWS_NOTHING(ConfigService::Instance().setLogLevel(4));
  }

  void testLogLevelSetGet() {
    Logger log1("testLogLevelGetSet");

    for (std::string x : Logger::PriorityNames) {
      TS_ASSERT_THROWS_NOTHING(ConfigService::Instance().setLogLevel(x));
      TS_ASSERT_EQUALS(log1.getLevelName(), x);
      TS_ASSERT_EQUALS(ConfigService::Instance().getLogLevel(), x);
    }

    // return back to previous values
    TS_ASSERT_THROWS_NOTHING(ConfigService::Instance().setLogLevel(4));
  }

  void testDefaultFacility() {
    TS_ASSERT_THROWS_NOTHING(ConfigService::Instance().getFacility());
    //
    ConfigService::Instance().setFacility("ISIS");
    const FacilityInfo &fac = ConfigService::Instance().getFacility();
    TS_ASSERT_EQUALS(fac.name(), "ISIS");

    ConfigService::Instance().setFacility("SNS");
    const FacilityInfo &fac1 = ConfigService::Instance().getFacility();
    TS_ASSERT_EQUALS(fac1.name(), "SNS");
  }

  void testChangingDefaultFacilityChangesInst() {
    // When changing default facility using the setFacility method, we should
    // also change the instrument to the default so we don't get weird
    // inst/facility combinations
    auto &config = ConfigService::Instance();

    config.setFacility("ISIS");
    const auto isisFirstInst = config.getString("default.instrument");
    TS_ASSERT(!isisFirstInst.empty());

    config.setFacility("SNS");
    const auto snsFirstInst = config.getString("default.instrument");
    TS_ASSERT(!snsFirstInst.empty());
    TS_ASSERT_DIFFERS(snsFirstInst, isisFirstInst);
  }

  void testFacilityList() {
    std::vector<FacilityInfo *> facilities = ConfigService::Instance().getFacilities();
    std::vector<std::string> names = ConfigService::Instance().getFacilityNames();

    TS_ASSERT_LESS_THAN(0, names.size());
    TS_ASSERT_EQUALS(facilities.size(), names.size());
    auto itFacilities = facilities.begin();
    auto itNames = names.begin();
    for (; itFacilities != facilities.end(); ++itFacilities, ++itNames) {
      TS_ASSERT_EQUALS(*itNames, (**itFacilities).name());
    }
  }

  void testInstrumentSearch() {
    // Set a default facility
    ConfigService::Instance().setFacility("SNS");

    // Try and find some instruments from a facility
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("BASIS").name(), "BASIS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("REF_L").name(), "REF_L");

    // Now find some from other facilities
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("OSIRIS").name(), "OSIRIS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("BIOSANS").name(), "BIOSANS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("NGSANS").name(), "NGSANS");

    // Check we throw the correct error for a nonsense beamline.
    // TS_ASSERT_THROWS(ConfigService::Instance().getInstrument("MyBeamline").name(),
    // const NotFoundError &);

    // Now find by using short name
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("BSS").name(), "BASIS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("MAR").name(), "MARI");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("PG3").name(), "POWGEN");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("OSI").name(), "OSIRIS");
    //    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("HiResSANS").name(),
    //    "GPSANS");

    // Now find some with the wrong case
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("baSis").name(), "BASIS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("TOPaZ").name(), "TOPAZ");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("Seq").name(), "SEQUOIA");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("eqsans").name(), "EQ-SANS");

    // Set the default instrument
    ConfigService::Instance().setString("default.instrument", "OSIRIS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument("").name(), "OSIRIS");
    TS_ASSERT_EQUALS(ConfigService::Instance().getInstrument().name(), "OSIRIS");
  }

  void testSystemValues() {
    // we cannot test the return values here as they will differ based on the
    // environment.
    // therfore we will just check they return a non empty string.
    std::string osName = ConfigService::Instance().getOSName();
    TS_ASSERT_LESS_THAN(0,
                        osName.length()); // check that the string is not empty
    std::string osArch = ConfigService::Instance().getOSArchitecture();
    TS_ASSERT_LESS_THAN(0,
                        osArch.length()); // check that the string is not empty
    std::string osCompName = ConfigService::Instance().getComputerName();
    TS_ASSERT_LESS_THAN(0, osCompName.length()); // check that the string is not empty
    std::string username = ConfigService::Instance().getUsername();
    TS_ASSERT_LESS_THAN(0, username.length());
    TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getOSVersion().length()); // check that the string is not empty
    TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getOSVersionReadable().length());
    TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getCurrentDir().length()); // check that the string is not empty
    //        TS_ASSERT_LESS_THAN(0,
    //        ConfigService::Instance().getHomeDir().length()); //check that the
    //        string is not empty
    TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getTempDir().length()); // check that the string is not empty

    std::string appdataDir = ConfigService::Instance().getAppDataDir();
    TS_ASSERT_LESS_THAN(0, appdataDir.length());
#ifdef _WIN32
    std::string::size_type index = appdataDir.find("\\AppData\\Roaming\\mantidproject\\mantid");
    TSM_ASSERT_LESS_THAN("Could not find correct path in getAppDataDir()", index, appdataDir.size());
#else
    std::string::size_type index = appdataDir.find("/.mantid");
    TSM_ASSERT_LESS_THAN("Could not find correct path in getAppDataDir()", index, appdataDir.size());
#endif
  }

  void testInstrumentDirectory() {

    auto directories = ConfigService::Instance().getInstrumentDirectories();
    TS_ASSERT_LESS_THAN(1, directories.size());
    // the first entry should be the AppDataDir + instrument
    TSM_ASSERT_LESS_THAN("Could not find the appData directory in getInstrumentDirectories()[0]",
                         directories[0].find(ConfigService::Instance().getAppDataDir()), directories[0].size());
    TSM_ASSERT_LESS_THAN("Could not find the 'instrument' directory in "
                         "getInstrumentDirectories()[0]",
                         directories[0].find("instrument"), directories[0].size());

    if (directories.size() == 3) {
      // The middle entry should be /etc/mantid/instrument
      TSM_ASSERT_LESS_THAN("Could not find /etc/mantid/instrument path in "
                           "getInstrumentDirectories()[1]",
                           directories[1].find("etc/mantid/instrument"), directories[1].size());
    }
    // Check that the last directory matches that returned by
    // getInstrumentDirectory
    TS_ASSERT_EQUALS(directories.back(), ConfigService::Instance().getInstrumentDirectory());

    // check all of the directory entries actually exist
    for (auto &directoryPath : directories) {
      Poco::File directory(directoryPath);
      TSM_ASSERT(directoryPath + " does not exist", directory.exists());
    }
  }

  void testSetInstrumentDirectory() {

    auto originalDirectories = ConfigService::Instance().getInstrumentDirectories();
    std::vector<std::string> testDirectories;
    testDirectories.emplace_back("Test Directory 1");
    testDirectories.emplace_back("Test Directory 2");
    ConfigService::Instance().setInstrumentDirectories(testDirectories);
    auto readDirectories = ConfigService::Instance().getInstrumentDirectories();
    TS_ASSERT_EQUALS(readDirectories.size(), testDirectories.size());
    TS_ASSERT_EQUALS(readDirectories[0], testDirectories[0]);
    TS_ASSERT_EQUALS(readDirectories[1], testDirectories[1]);

    // Restore original settings
    ConfigService::Instance().setInstrumentDirectories(originalDirectories);
    readDirectories = ConfigService::Instance().getInstrumentDirectories();
    TS_ASSERT_EQUALS(readDirectories.size(), originalDirectories.size());
    TS_ASSERT_EQUALS(readDirectories[0], originalDirectories[0]);
  }

  void testCustomProperty() {
    std::string countString = ConfigService::Instance().getString("projectRecovery.secondsBetween");
    TS_ASSERT_EQUALS(countString, "60");
  }

  void testCustomPropertyAsValue() {
    // Mantid.legs is defined in the properties script as 6
    int value = ConfigService::Instance().getValue<int>("projectRecovery.secondsBetween").get_value_or(0);
    double dblValue = ConfigService::Instance().getValue<double>("projectRecovery.secondsBetween").get_value_or(0);

    TS_ASSERT_EQUALS(value, 60);
    TS_ASSERT_EQUALS(dblValue, 60.0);
  }

  void testMissingProperty() {
    // Mantid.noses is not defined in the properties script
    std::string noseCountString = ConfigService::Instance().getString("mantid.noses");
    // this should return an empty string

    TS_ASSERT_EQUALS(noseCountString, "");
  }

  void testAppendProperties() {

    // This should clear out all old properties
    const std::string propfilePath = ConfigService::Instance().getDirectoryOfExecutable();
    const std::string propfile = propfilePath + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);
    // this should return an empty string
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.noses"), "");
    // this should pass
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.legs"), "6");
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.thorax"), "1");

    // This should append a new properties file properties
    ConfigService::Instance().updateConfig(propfilePath + "MantidTest.user.properties", true);
    // this should now be valid
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.noses"), "5");
    // this should have been overridden
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.legs"), "76");
    // this should have been left alone
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.thorax"), "1");

    // This should clear out all old properties
    ConfigService::Instance().updateConfig(propfile);
    // this should return an empty string
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.noses"), "");
    // this should pass
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.legs"), "6");
    TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.thorax"), "1");
  }

  void testSaveConfigCleanFile() {
    const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);

    const std::string filename("user.settings");

    // save any previous changed settings to make sure we're on a clean slate
    ConfigService::Instance().saveConfig(filename);

    Poco::File prop_file(filename);
    // Start with a clean state
    if (prop_file.exists())
      prop_file.remove();

    ConfigServiceImpl &settings = ConfigService::Instance();
    TS_ASSERT_THROWS_NOTHING(settings.saveConfig(filename));

    // No changes yet, file exists but is blank
    TS_ASSERT_EQUALS(prop_file.exists(), true);
    std::string contents = readFile(filename);
    TS_ASSERT(contents.empty());

    runSaveTest(filename, "11");
  }

  void testSaveConfigExistingSettings() {

    const std::string filename("user.settings");
    Poco::File prop_file(filename);
    if (prop_file.exists())
      prop_file.remove();

    std::ofstream writer(filename.c_str(), std::ios_base::trunc);
    writer << "mantid.legs = 6";
    writer.close();

    runSaveTest(filename, "13");
  }

  void testLoadChangeLoadSavesOriginalValueIfSettingExists() {
    const std::string filename("user.settingsLoadChangeLoad");
    Poco::File prop_file(filename);
    if (prop_file.exists())
      prop_file.remove();
    const std::string value("15");
    std::ofstream writer(filename.c_str());
    writer << "mantid.legs = " << value << "\n";
    writer.close();

    const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);
    ConfigService::Instance().setString("mantid.legs", value);
    ConfigService::Instance().updateConfig(propfile, false, false);

    ConfigService::Instance().saveConfig(filename);

    const std::string contents = readFile(filename);
    TS_ASSERT_EQUALS(contents, "mantid.legs=6\n");

    prop_file.remove();
  }

  void testLoadChangeClearSavesOriginalPropsFile() {
    // Backup current user settings
    ConfigServiceImpl &settings = ConfigService::Instance();
    const std::string userFileBackup = settings.getUserFilename() + ".unittest";
    try {
      Poco::File userFile(settings.getUserFilename());
      userFile.moveTo(userFileBackup);
    } catch (Poco::Exception &) {
    }

    const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() + "MantidTest.properties";
    settings.updateConfig(propfile);
    settings.setString("mantid.legs", "15");

    settings.reset();

    const std::string contents = readFile(settings.getUserFilename());
    // No mention of mantid.legs but not empty
    TS_ASSERT(!contents.empty());
    TS_ASSERT(contents.find("mantid.legs") == std::string::npos);

    try {
      Poco::File backup(userFileBackup);
      backup.moveTo(settings.getUserFilename());
    } catch (Poco::Exception &) {
    }
  }

  void testRemoveNotPopulated() {
    /* If a property was not originally set in the properties file,
     *  but is later removed, it will still appear in a list or properties
     *  to update.  This test ensures the removed properties are not present.
     */

    // Backup current user settings
    ConfigServiceImpl &settings = ConfigService::Instance();
    const std::string userFileBackup = settings.getUserFilename() + ".unittest";
    try {
      Poco::File userFile(settings.getUserFilename());
      userFile.moveTo(userFileBackup);
    } catch (Poco::Exception &) {
    }

    // pick a property that is not in the config file
    std::string garbage("garbage.truck");
    std::string keeps("garbage.keep"); // control property
    TS_ASSERT(!settings.hasProperty(garbage));
    TS_ASSERT(!settings.hasProperty(keeps));

    // add the property to the config
    settings.setString(garbage, "yes");
    settings.setString(keeps, "yes");
    TS_ASSERT(settings.hasProperty(garbage));
    TS_ASSERT(settings.hasProperty(keeps));

    // now remove the property and save.
    // ensure the removed property is not inside the config file
    settings.remove(garbage);
    settings.saveConfig(settings.getUserFilename());
    const std::string contents = readFile(settings.getUserFilename());
    TS_ASSERT(!contents.empty());
    TS_ASSERT(contents.find(garbage) == std::string::npos);
    TS_ASSERT(contents.find(keeps) != std::string::npos);

    // restore the old file
    try {
      Poco::File backup(userFileBackup);
      backup.moveTo(settings.getUserFilename());
    } catch (Poco::Exception &) {
    }
  }

  void testSaveConfigWithPropertyRemoved() {
    const std::string filename("user.settings.testSaveConfigWithPropertyRemoved");
    Poco::File prop_file(filename);
    if (prop_file.exists())
      prop_file.remove();

    std::ofstream writer(filename.c_str(), std::ios_base::trunc);
    writer << "mantid.legs = 6"
           << "\n";
    writer << "\n";
    writer << "mantid.thorax = 10\n";
    writer << "# This comment line\n";
    writer << " # This is an indented comment line\n";
    writer << "key.withnospace=5\n";
    writer << "key.withnovalue";
    writer.close();

    ConfigService::Instance().updateConfig(filename, false, false);

    std::string rootName = "mantid.thorax";
    ConfigService::Instance().remove(rootName);
    TS_ASSERT_EQUALS(ConfigService::Instance().hasProperty(rootName), false);
    rootName = "key.withnovalue";
    ConfigService::Instance().remove(rootName);
    TS_ASSERT_EQUALS(ConfigService::Instance().hasProperty(rootName), false);

    ConfigService::Instance().saveConfig(filename);

    // Test the entry
    std::ifstream reader(filename.c_str(), std::ios::in);
    if (reader.bad()) {
      TS_FAIL("Unable to open config file for saving");
    }
    std::string line("");
    std::map<int, std::string> prop_lines;
    int line_index(0);
    while (getline(reader, line)) {
      prop_lines.emplace(line_index, line);
      ++line_index;
    }
    reader.close();

    TS_ASSERT_EQUALS(prop_lines.size(), 5);
    TS_ASSERT_EQUALS(prop_lines[0], "mantid.legs=6");
    TS_ASSERT_EQUALS(prop_lines[1], "");
    TS_ASSERT_EQUALS(prop_lines[2], "# This comment line");
    TS_ASSERT_EQUALS(prop_lines[3], " # This is an indented comment line");
    TS_ASSERT_EQUALS(prop_lines[4], "key.withnospace=5");

    // Clean up
    prop_file.remove();
  }

  void testSaveConfigWithLineContinuation() {
    /*const std::string propfile =
    ConfigService::Instance().getDirectoryOfExecutable()
      + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);*/

    const std::string filename("user.settingsLineContinuation");
    Poco::File prop_file(filename);
    if (prop_file.exists())
      prop_file.remove();

    ConfigServiceImpl &settings = ConfigService::Instance();

    std::ofstream writer(filename.c_str(), std::ios_base::trunc);
    writer << "mantid.legs=6\n\n"
              "search.directories=/test1;\\\n"
              "/test2;/test3;\\\n"
              "/test4\n";
    writer.close();

    ConfigService::Instance().updateConfig(filename, true, false);

    TS_ASSERT_THROWS_NOTHING(settings.setString("mantid.legs", "15"));

    TS_ASSERT_THROWS_NOTHING(settings.saveConfig(filename));
    // Should exist
    TS_ASSERT_EQUALS(prop_file.exists(), true);

    // Test the entry
    std::ifstream reader(filename.c_str(), std::ios::in);
    if (reader.bad()) {
      TS_FAIL("Unable to open config file for saving");
    }
    std::string line("");
    std::map<int, std::string> prop_lines;
    int line_index(0);
    while (getline(reader, line)) {
      prop_lines.emplace(line_index, line);
      ++line_index;
    }
    reader.close();

    TS_ASSERT_EQUALS(prop_lines.size(), 3);
    TS_ASSERT_EQUALS(prop_lines[0], "mantid.legs=15");
    TS_ASSERT_EQUALS(prop_lines[1], "");
    TS_ASSERT_EQUALS(prop_lines[2], "search.directories=/test1;/test2;/test3;/test4");

    // Clean up
    // prop_file.remove();
  }

  // Test that the ValueChanged notification is sent
  void testNotifications() {
    Poco::NObserver<ConfigServiceTest, ConfigServiceImpl::ValueChanged> changeObserver(
        *this, &ConfigServiceTest::handleConfigChange);
    m_valueChangedSent = false;

    ConfigServiceImpl &settings = ConfigService::Instance();

    TS_ASSERT_THROWS_NOTHING(settings.addObserver(changeObserver));

    settings.setString("default.facility", "SNS");

    TS_ASSERT(m_valueChangedSent);
    TS_ASSERT_EQUALS(m_key, "default.facility");
    TS_ASSERT_DIFFERS(m_preValue, m_curValue);
    TS_ASSERT_EQUALS(m_curValue, "SNS");
  }

  void testGetKeysWithValidInput() {
    const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);

    // Returns all subkeys with the given root key
    std::vector<std::string> keyVector = ConfigService::Instance().getKeys("workspace.sendto");
    TS_ASSERT_EQUALS(keyVector.size(), 4);
    TS_ASSERT_EQUALS(keyVector[0], "1");
    TS_ASSERT_EQUALS(keyVector[1], "2");
    TS_ASSERT_EQUALS(keyVector[2], "3");
    TS_ASSERT_EQUALS(keyVector[3], "4");
  }

  void testGetKeysWithZeroSubKeys() {
    const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);

    std::vector<std::string> keyVector = ConfigService::Instance().getKeys("mantid.legs");
    TS_ASSERT_EQUALS(keyVector.size(), 0);
    std::vector<std::string> keyVector2 = ConfigService::Instance().getKeys("mantidlegs");
    TS_ASSERT_EQUALS(keyVector2.size(), 0);
  }

  void testGetKeysWithEmptyPrefix() {
    const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);

    // Returns all *root* keys, i.e. unique keys left of the first period
    std::vector<std::string> keyVector = ConfigService::Instance().getKeys("");
    TS_ASSERT_EQUALS(keyVector.size(), 5);
  }

  void testGetAllKeys() {
    const std::string propfilePath = ConfigService::Instance().getDirectoryOfExecutable();
    const std::string propfile = propfilePath + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);

    std::vector<std::string> keys = ConfigService::Instance().keys();

    TS_ASSERT_EQUALS(keys.size(), 9);
  }

  void testRemovingProperty() {
    const std::string propfile = ConfigService::Instance().getDirectoryOfExecutable() + "MantidTest.properties";
    ConfigService::Instance().updateConfig(propfile);

    std::string rootName = "mantid.legs";
    bool mantidLegs = ConfigService::Instance().hasProperty(rootName);
    TS_ASSERT_EQUALS(mantidLegs, true);

    // Remove the value from the key and test again
    ConfigService::Instance().remove(rootName);
    mantidLegs = ConfigService::Instance().hasProperty(rootName);
    TS_ASSERT_EQUALS(mantidLegs, false);
  }

protected:
  bool m_valueChangedSent;
  std::string m_key;
  std::string m_preValue;
  std::string m_curValue;
  void handleConfigChange(const Poco::AutoPtr<Mantid::Kernel::ConfigServiceImpl::ValueChanged> &pNf) {
    m_valueChangedSent = true;
    m_key = pNf->key();
    m_preValue = pNf->preValue();
    m_curValue = pNf->curValue();
  }

private:
  void runSaveTest(const std::string &filename, const std::string &legs) {
    ConfigServiceImpl &settings = ConfigService::Instance();
    // Make a change and save again
    std::string key("mantid.legs");
    std::string value(legs);
    TS_ASSERT_THROWS_NOTHING(settings.setString(key, value));
    TS_ASSERT_THROWS_NOTHING(settings.saveConfig(filename));

    // Should exist
    Poco::File prop_file(filename);
    TS_ASSERT_EQUALS(prop_file.exists(), true);

    // Test the entry
    std::ifstream reader(filename.c_str(), std::ios::in);
    if (reader.bad()) {
      TS_FAIL("Unable to open config file for saving");
    }
    std::string line("");
    while (std::getline(reader, line)) {
      if (line.empty())
        continue;
      else
        break;
    }
    reader.close();

    std::string key_value = key + "=" + value;
    TS_ASSERT_EQUALS(line, key_value);

    // Clean up
    prop_file.remove();
  }

  std::string readFile(const std::string &filename) {
    std::ifstream reader(filename.c_str());
    return std::string((std::istreambuf_iterator<char>(reader)), std::istreambuf_iterator<char>());
  }
};
