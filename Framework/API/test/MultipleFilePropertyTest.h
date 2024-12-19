// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Timer.h"

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>
#include <Poco/Path.h>

#include <boost/algorithm/string/join.hpp>
#include <unordered_set>

using namespace Mantid;
using namespace Mantid::API;
using Mantid::API::FileProperty;

//////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions.
//////////////////////////////////////////////////////////////////////////////////////////////

namespace // anonymous
{
/**
 * Given a directory name, create the directory and return its absolute path.
 *
 * @param dirPath :: the directory name.
 *
 * @returns the absolute path to the directory.
 */
std::string createAbsoluteDirectory(const std::string &dirPath) {
  Poco::File dir(dirPath);
  dir.createDirectories();
  Poco::Path path(dir.path());
  path = path.makeAbsolute();
  return path.toString();
}

/**
 * Given a set of filenames and a directory path, create each file inside the
 *directory.
 *
 * @param filenames :: the names of the files to create.
 * @param dirPath   :: the directory in which to create the files.
 */
void createFilesInDirectory(const std::unordered_set<std::string> &filenames, const std::string &dirPath) {
  for (const auto &filename : filenames) {
    Poco::File file(dirPath + "/" + filename);
    file.createFile();
  }
}

} // anonymous namespace

class MultipleFilePropertyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor (& destructor) isn't called when running other
  // tests
  static MultipleFilePropertyTest *createSuite() { return new MultipleFilePropertyTest(); }
  static void destroySuite(MultipleFilePropertyTest *suite) { delete suite; }

private:
  std::string m_multiFileLoadingSetting;
  std::string m_oldDataSearchDirectories;
  std::string m_oldDefaultFacility;
  std::string m_oldDefaultInstrument;
  std::string m_dummyFilesDir;
  std::string m_dirWithWhitespace;
  std::unordered_set<std::string> m_tempDirs;
  std::vector<std::string> m_exts;
  std::string m_oldArchiveSearchSetting;

  Mantid::Kernel::ConfigServiceImpl &g_config;

public:
  //////////////////////////////////////////////////////////////////////////////////////////////
  // Setting up the testing class.
  //////////////////////////////////////////////////////////////////////////////////////////////

  /**
   * Constructor containing one-time set up of the tests.
   *
   * Creates the necessary temp directories, fills them with dummy files, saves
   *the
   * user's current data search directories, and replaces them with temp
   *directories.
   */
  MultipleFilePropertyTest()
      : m_multiFileLoadingSetting(), m_oldDataSearchDirectories(), m_oldDefaultFacility(), m_oldDefaultInstrument(),
        m_dummyFilesDir(), m_dirWithWhitespace(), m_tempDirs(), m_exts{".raw", ".nxs"}, m_oldArchiveSearchSetting(),
        g_config(Mantid::Kernel::ConfigService::Instance()) {
    m_dummyFilesDir = createAbsoluteDirectory("_MultipleFilePropertyTestDummyFiles");
    m_dirWithWhitespace = createAbsoluteDirectory("_MultipleFilePropertyTest Folder With Whitespace");

    m_tempDirs.insert(m_dummyFilesDir);
    m_tempDirs.insert(m_dirWithWhitespace);

    std::unordered_set<std::string> dummyFilenames = {
        // Standard raw file runs.
        "TSC00001.raw", "TSC00002.raw", "TSC00003.raw", "TSC00004.raw", "TSC00005.raw",
        // Duplicates, but in NeXuS format.
        "TSC00001.nxs", "TSC00002.nxs", "TSC00003.nxs", "TSC00004.nxs", "TSC00005.nxs",
        // Standard NeXuS runs for another instrument.
        "IRS00001.raw", "IRS00002.raw", "IRS00003.raw", "IRS00004.raw", "IRS00005.raw",
        // Duplicates, but in NeXuS format.
        "IRS00001.nxs", "IRS00002.nxs", "IRS00003.nxs", "IRS00004.nxs", "IRS00005.nxs",
        // "Incorrect" zero padding file.
        "TSC9999999.raw",
        // "Non-run" files.
        "IRS10001_graphite002_info.nxs", "IRS10002_graphite002_info.nxs", "IRS10003_graphite002_info.nxs",
        "IRS10004_graphite002_info.nxs", "IRS10005_graphite002_info.nxs",
        // File with no extension.
        "bl6_flux_at_sample",
        // A single "non-run" file, that we should be able to load.
        "IRS10001-10005_graphite002_info.nxs",
        // A file with a "+" and "," in the name, to see if it can be loaded
        // when multifileloading is turned off via the preferences file.
        "_test_multiFileLoadingSwitchedOff_tempFileWithA+AndA,InTheName.txt",
        // Runs with no instrument name as prefix, commonplace at the ILL
        "111213.nxs", "141516.nxs", "171819.nxs"};

    std::unordered_set<std::string> whiteSpaceDirFilenames = {"file with whitespace.txt"};

    createFilesInDirectory(dummyFilenames, m_dummyFilesDir);
    createFilesInDirectory(whiteSpaceDirFilenames, m_dummyFilesDir);
  }

  /**
   * Destructor containing one-time tear down of the tests.
   *
   * Reset the user's data search directories.
   */
  ~MultipleFilePropertyTest() override {
    // Remove temp dirs.
    for (const auto &tempDir : m_tempDirs) {
      Poco::File dir(tempDir);
      dir.remove(true);
    }
  }

  void setUp() override {
    m_oldDataSearchDirectories = g_config.getString("datasearch.directories");
    m_oldDefaultFacility = g_config.getString("default.facilities");
    m_oldDefaultInstrument = g_config.getString("default.instrument");
    m_oldArchiveSearchSetting = g_config.getString("datasearch.searcharchive");

    g_config.setString("datasearch.directories", m_dummyFilesDir + ";" + m_dirWithWhitespace + ";");
    g_config.setString("default.facility", "ISIS");
    g_config.setString("default.instrument", "TOSCA");
    g_config.setString("datasearch.searcharchive", "Off");

    // Make sure that multi file loading is enabled for each test.
    m_multiFileLoadingSetting = Kernel::ConfigService::Instance().getString("loading.multifile");
    Kernel::ConfigService::Instance().setString("loading.multifile", "On");
  }

  void tearDown() override {
    g_config.setString("datasearch.directories", m_oldDataSearchDirectories);
    g_config.setString("default.facility", m_oldDefaultFacility);
    g_config.setString("default.instrument", m_oldDefaultInstrument);
    g_config.setString("datasearch.searcharchive", m_oldArchiveSearchSetting);

    // Replace user's preference after the test has run.
    Kernel::ConfigService::Instance().setString("loading.multifile", m_multiFileLoadingSetting);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////
  // Testing of MultipleFileProperty objects when multiple file loading has been
  // switched ON.
  //////////////////////////////////////////////////////////////////////////////////////////////

  void test_singeFile_fullPath() {
    MultipleFileProperty p("Filename");
    p.setValue(dummyFile("TSC1.raw"));
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
  }

  void test_singeFile_noInst() {
    MultipleFileProperty p("Filename");
    p.setValue(dummyFile("1.raw"));
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
  }

  void test_singeFile_noExt() {
    MultipleFileProperty p("Filename", m_exts);
    p.setValue(dummyFile("TSC1"));
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
  }

  void test_singeFile_noInstNoExt() {
    MultipleFileProperty p("Filename", m_exts);
    p.setValue(dummyFile("1"));
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
  }

  void test_singeFile_noDir() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
  }

  void test_singeFile_noDirNoInst() {
    MultipleFileProperty p("Filename");
    p.setValue("1.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
  }

  void test_singeFile_noDirNoExt() {
    MultipleFileProperty p("Filename", m_exts);
    p.setValue("TSC1");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
  }

  void test_singeFile_noDirNoInstNoExt() {
    MultipleFileProperty p("Filename", m_exts);
    p.setValue("1");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
  }

  void test_singleFile_shortZeroPadding() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC001.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
  }

  void test_singleFile_longZeroPadding() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC000000000001.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
  }

  void test_singleFile_fileWithIncorrectZeroPaddingStillFound() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC9999999.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC9999999.raw"));
  }

  void test_singleFile_longForm_singleFileLooksLikeARangeWithSuffix() {
    // This test essentially is here to show the reason why we dont support
    // suffixes along with multifile parsing.  Consider the case where:
    //
    // p.setValue("IRS10001-10005_graphite002_info.nxs");
    //
    // Which of the following should be loaded?
    //
    // a) "IRS10001-10005_graphite002_info.nxs"; or
    // b) "IRS10001_graphite002_info.nxs",
    //    "IRS10002_graphite002_info.nxs",
    //    "IRS10003_graphite002_info.nxs",
    //    "IRS10004_graphite002_info.nxs",
    //    "IRS10005_graphite002_info.nxs"
    //
    // If both a) and b) exist (as they do in this test case), then we have no
    // choice but to load a).

    MultipleFileProperty p("Filename");
    p.setValue("IRS10001-10005_graphite002_info.nxs");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("IRS10001-10005_graphite002_info.nxs"));
  }

  void test_singleFile_fileThatHasNoExtension() {
    MultipleFileProperty p("Filename");
    p.setValue("bl6_flux_at_sample");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("bl6_flux_at_sample"));
  }

  void test_multipleFiles_shortForm_commaList() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1,2,3,4,5.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[3][0], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[4][0], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_shortForm_plusList() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1+2+3+4+5.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[0][2], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[0][3], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[0][4], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_shortForm_range() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1:5.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[3][0], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[4][0], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_shortForm_addedRange() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1-5.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[0][2], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[0][3], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[0][4], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_shortForm_steppedRange() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1:5:2.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_shortForm_steppedAddedRange() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1-5:2.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[0][2], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_shortForm_complex() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1,2:5,1+2+3,2-4.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[3][0], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[4][0], dummyFile("TSC00005.raw"));
    TS_ASSERT_EQUALS(fileNames[5][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[5][1], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[5][2], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[6][0], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[6][1], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[6][2], dummyFile("TSC00004.raw"));
  }

  void test_multipleFiles_shortForm_addRanges() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1-2+4-5.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[0][2], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[0][3], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_shortForm_addSingleToRange() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC2+4-5.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[0][2], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_shortForm_rangeToSingle() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1-2+5.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[0][2], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_longForm_commaList() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1.raw,TSC2.raw,TSC3.raw,TSC4.raw,TSC5.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[3][0], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[4][0], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_longForm_plusList() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1.raw+TSC2.raw+TSC3.raw+TSC4.raw+TSC5.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[0][2], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[0][3], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[0][4], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_longForm_nonRunFiles() {
    MultipleFileProperty p("Filename");
    p.setValue("IRS10001_graphite002_info.nxs+IRS10002_graphite002_info.nxs,"
               "IRS10003_graphite002_info.nxs");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("IRS10001_graphite002_info.nxs"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("IRS10002_graphite002_info.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("IRS10003_graphite002_info.nxs"));
  }

  void test_multipleFiles_mixedForm_1() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1,2.raw,TSC3,4,5.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[3][0], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[4][0], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_mixedForm_2() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1,2.raw,TSC3:5.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[3][0], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[4][0], dummyFile("TSC00005.raw"));
  }

  void test_multipleFiles_mixedForm_mixedInstAndExt() {
    MultipleFileProperty p("Filename");
    // This would never load successfully as the Load algo currently forbids
    // mixing loaders (this makes processing other algorithm inputs easier),
    // however, there is no reason why MultipleFileProperty can't at least
    // handle it.
    p.setValue("TSC1-5:1.raw,IRS1-5:1.nxs");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[0][2], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[0][3], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[0][4], dummyFile("TSC00005.raw"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("IRS00001.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][1], dummyFile("IRS00002.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][2], dummyFile("IRS00003.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][3], dummyFile("IRS00004.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][4], dummyFile("IRS00005.nxs"));
  }

  void test_multipleFiles_mixedForm_missingExtensionsMeansFirstDefaultExtIsUsed() {
    // ".raw" appears first in m_exts, so raw files will be found.
    MultipleFileProperty p("Filename", m_exts);
    p.setValue("TSC1-5:1,IRS1-5:1");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[0][2], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[0][3], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[0][4], dummyFile("TSC00005.raw"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("IRS00001.raw"));
    TS_ASSERT_EQUALS(fileNames[1][1], dummyFile("IRS00002.raw"));
    TS_ASSERT_EQUALS(fileNames[1][2], dummyFile("IRS00003.raw"));
    TS_ASSERT_EQUALS(fileNames[1][3], dummyFile("IRS00004.raw"));
    TS_ASSERT_EQUALS(fileNames[1][4], dummyFile("IRS00005.raw"));
  }

  void test_multipleFiles_mixedForm_someMissingExtensionsMeansFirstSpecifiedIsUsed() {
    MultipleFileProperty p("Filename", m_exts);
    p.setValue("IRS1-5:1,TSC1-5:1.nxs");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("IRS00001.nxs"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("IRS00002.nxs"));
    TS_ASSERT_EQUALS(fileNames[0][2], dummyFile("IRS00003.nxs"));
    TS_ASSERT_EQUALS(fileNames[0][3], dummyFile("IRS00004.nxs"));
    TS_ASSERT_EQUALS(fileNames[0][4], dummyFile("IRS00005.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00001.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][1], dummyFile("TSC00002.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][2], dummyFile("TSC00003.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][3], dummyFile("TSC00004.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][4], dummyFile("TSC00005.nxs"));
  }

  void test_multipleFiles_mixedForm_complex() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1,2:5.raw,TSC1+2+3,2-4.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[3][0], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[4][0], dummyFile("TSC00005.raw"));
    TS_ASSERT_EQUALS(fileNames[5][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[5][1], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[5][2], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[6][0], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[6][1], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[6][2], dummyFile("TSC00004.raw"));
  }

  void test_multipleFiles_mixedForm_complexAndNonRunFile() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1,2:5.raw,IRS10001_graphite002_info.nxs,TSC1+2+3,2-4.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[3][0], dummyFile("TSC00004.raw"));
    TS_ASSERT_EQUALS(fileNames[4][0], dummyFile("TSC00005.raw"));
    TS_ASSERT_EQUALS(fileNames[5][0], dummyFile("IRS10001_graphite002_info.nxs"));
    TS_ASSERT_EQUALS(fileNames[6][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[6][1], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[6][2], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[7][0], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[7][1], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[7][2], dummyFile("TSC00004.raw"));
  }

  void test_multipleFiles_mixedForm_addingTwoLists_FAILS() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1,2.raw+TSC3,4.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames.size(), 0);
  }

  void test_fails_addingTwoPlussedLists() {
    MultipleFileProperty p("Filename");
    p.setValue("TSC1+2.raw+TSC3+4.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.raw"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("TSC00002.raw"));
    TS_ASSERT_EQUALS(fileNames[0][2], dummyFile("TSC00003.raw"));
    TS_ASSERT_EQUALS(fileNames[0][3], dummyFile("TSC00004.raw"));
  }

  void test_allowEmptyTokenOptionalLoad() {
    g_config.setString("default.facility", "ILL");
    g_config.setString("default.instrument", "IN16B");
    MultipleFileProperty p("Filename", FileProperty::FileAction::OptionalLoad, {".nxs"}, true);
    p.setValue("111213,0,171819");
    std::vector<std::vector<std::string>> fileNames = p();
    TS_ASSERT_EQUALS(fileNames.size(), 3);
    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("111213.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][0], "000000");
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("171819.nxs"));
    g_config.setString("default.facility", "ISIS");
    g_config.setString("default.instrument", "TOSCA");
  }

  void test_allowEmptyTokenLoad() {
    g_config.setString("default.facility", "ILL");
    g_config.setString("default.instrument", "IN16B");
    MultipleFileProperty p("Filename", FileProperty::FileAction::Load, {".nxs"}, true);
    TS_ASSERT_THROWS_EQUALS(p.setValue("111213,0,171819"), const std::invalid_argument &err, std::string(err.what()),
                            "When setting value of property \"Filename\": "
                            "Could not validate the following file(s): 000000");
    g_config.setString("default.facility", "ISIS");
    g_config.setString("default.instrument", "TOSCA");
  }

  void test_multipleFiles_consistent_spaces() {
    MultipleFileProperty p("Filename");
    p.setValue("1, 2, 3, 4, 5");

    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00002.nxs"));
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("TSC00003.nxs"));
    TS_ASSERT_EQUALS(fileNames[3][0], dummyFile("TSC00004.nxs"));
    TS_ASSERT_EQUALS(fileNames[4][0], dummyFile("TSC00005.nxs"));
  }

  void test_multipleFiles_inconsistent_spaces() {
    MultipleFileProperty p("Filename");
    p.setValue("1,2, 3  ,  4, 5");

    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00002.nxs"));
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("TSC00003.nxs"));
    TS_ASSERT_EQUALS(fileNames[3][0], dummyFile("TSC00004.nxs"));
    TS_ASSERT_EQUALS(fileNames[4][0], dummyFile("TSC00005.nxs"));
  }

  void test_multipleFiles_space_after_first() {
    MultipleFileProperty p("Filename");
    p.setValue("1, 2,3,4,5");

    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00002.nxs"));
    TS_ASSERT_EQUALS(fileNames[2][0], dummyFile("TSC00003.nxs"));
    TS_ASSERT_EQUALS(fileNames[3][0], dummyFile("TSC00004.nxs"));
    TS_ASSERT_EQUALS(fileNames[4][0], dummyFile("TSC00005.nxs"));
  }

  void test_multipleFiles_ranges_with_spaces() {
    MultipleFileProperty p("Filename");
    p.setValue("1-5, 3-4");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0][0], dummyFile("TSC00001.nxs"));
    TS_ASSERT_EQUALS(fileNames[0][1], dummyFile("TSC00002.nxs"));
    TS_ASSERT_EQUALS(fileNames[0][2], dummyFile("TSC00003.nxs"));
    TS_ASSERT_EQUALS(fileNames[0][3], dummyFile("TSC00004.nxs"));
    TS_ASSERT_EQUALS(fileNames[0][4], dummyFile("TSC00005.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][0], dummyFile("TSC00003.nxs"));
    TS_ASSERT_EQUALS(fileNames[1][1], dummyFile("TSC00004.nxs"));
  }

  //////////////////////////////////////////////////////////////////////////////////////////////
  // Testing of MultipleFileProperty objects when multiple file loading has been
  // switched OFF.
  //////////////////////////////////////////////////////////////////////////////////////////////

  void test_multiFileLoadingSwitchedOff_ignoreDelimeters() {
    g_config.setString("loading.multifile", "Off");

    MultipleFileProperty p("Filename");
    p.setValue("_test_multiFileLoadingSwitchedOff_tempFileWithA+AndA,InTheName.txt");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames.size(), 1);
    TS_ASSERT_EQUALS(fileNames[0].size(), 1);
  }

  void test_multiFileLoadingSwitchedOff_normalRunFile() {
    g_config.setString("loading.multifile", "Off");

    MultipleFileProperty p("Filename");
    p.setValue("TSC00001.raw");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames.size(), 1);
    TS_ASSERT_EQUALS(fileNames[0].size(), 1);
  }

  void test_multiFileLoadingSwitchedOff_multiFileLoadingFails() {
    g_config.setString("loading.multifile", "Off");

    MultipleFileProperty p("Filename");
    p.setValue("TSC00001.raw, TSC0001.raw");

    std::vector<std::vector<std::string>> fileNames = p();
    TS_ASSERT_EQUALS(fileNames.size(), 0);
  }

  void test_multiFileLoadingSwitchedOff_fileWithWhitespace() {
    g_config.setString("loading.multifile", "Off");

    MultipleFileProperty p("Filename");
    p.setValue("file with whitespace.txt");
    std::vector<std::vector<std::string>> fileNames = p();

    TS_ASSERT_EQUALS(fileNames.size(), 1);
    TS_ASSERT_EQUALS(fileNames[0].size(), 1);
  }

  void test_multiFileOptionalLoad() {
    MultipleFileProperty p("Filename", FileProperty::OptionalLoad);
    p.setValue("myJunkFile.nxs");
    TS_ASSERT(p.isValid().empty());
  }

  void test_multiFileOptionalLoadEmpty() {
    MultipleFileProperty p("Filename", FileProperty::OptionalLoad);
    p.setValue("");
    TS_ASSERT(p.isValid().empty());
  }

private:
  //////////////////////////////////////////////////////////////////////////////////////////////
  // Private helper functions.
  //////////////////////////////////////////////////////////////////////////////////////////////
  std::string dummyFile(const std::string &filename) { return m_dummyFilesDir + Poco::Path::separator() + filename; }
};
