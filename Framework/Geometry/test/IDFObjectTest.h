// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/ScopedFileHelper.h"
#include "MantidGeometry/Instrument/IDFObject.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/DateTimeFormatter.h>
#include <Poco/DigestStream.h>
#include <Poco/SHA1Engine.h>
#include <Poco/String.h>
#include <Poco/Thread.h>
#include <cxxtest/TestSuite.h>

#include <boost/regex.hpp>
#include <filesystem>

using Mantid::Geometry::IDFObject;
using Mantid::Kernel::ConfigService;

class IDFObjectTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IDFObjectTest *createSuite() { return new IDFObjectTest(); }
  static void destroySuite(IDFObjectTest *suite) { delete suite; }

  void testExpectedExtensionIsXML() { TS_ASSERT_EQUALS(".xml", IDFObject::expectedExtension()); }

  void testExists() {
    const std::string filename =
        ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/IDF_for_UNIT_TESTING.xml";
    IDFObject obj(filename);
    TS_ASSERT(obj.exists());
  }

  void testDoesntExistIfEmpty() {
    IDFObject obj("");
    TS_ASSERT(!obj.exists());
  }

  void testDoesntExist() {
    const std::string filename = "made_up_file.xml";
    IDFObject obj(filename);
    TS_ASSERT(!obj.exists());
  }

  void testGetParentDirectory() {
    const std::filesystem::path expectedDir =
        std::filesystem::path(ConfigService::Instance().getInstrumentDirectory()) / "unit_testing";
    std::string filename = (expectedDir / "IDF_for_UNIT_TESTING.xml").string();
    IDFObject obj(filename);
    TS_ASSERT_EQUALS(expectedDir.string(), obj.getParentDirectory().string());
  }

  void testGetFullPath() {
    const std::string filename =
        ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/IDF_for_UNIT_TESTING.xml";
    IDFObject obj(filename);
    TS_ASSERT_EQUALS(std::filesystem::path(filename).string(), obj.getFileFullPath().string());
  }

  void testGetExtension() {
    const std::string filename =
        ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/IDF_for_UNIT_TESTING.xml";
    IDFObject obj(filename);
    TS_ASSERT_EQUALS(".xml", obj.getExtension());
  }

  void testGetFileNameOnly() {
    const std::string filenameonly = "IDF_for_UNIT_TESTING.xml";
    const std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/" + filenameonly;
    IDFObject obj(filename);
    TS_ASSERT_EQUALS(filenameonly, obj.getFileNameOnly());
  }

  void testGetMangledName() {
    const std::string filename =
        ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/IDF_for_UNIT_TESTING.xml";

    std::filesystem::path path(filename);

    using Poco::DigestEngine;
    using Poco::DigestOutputStream;
    using Poco::SHA1Engine;

    std::ifstream filein(filename.c_str(), std::ios::in | std::ios::binary);
    if (!filein) {
      TS_FAIL("Cannot open file: " + filename);
      return;
    }

    std::string contents;
    filein.seekg(0, std::ios::end);
    contents.resize(filein.tellg());
    filein.seekg(0, std::ios::beg);
    filein.read(&contents[0], contents.size());
    filein.close();

    // convert to unix line endings
    static boost::regex eol("\\R");                       // \R is Perl syntax for matching any EOL sequence
    contents = boost::regex_replace(contents, eol, "\n"); // converts all to LF

    // and trim
    contents = Poco::trim(contents);

    SHA1Engine sha1;
    DigestOutputStream outstr(sha1);
    outstr << contents;
    outstr.flush(); // to pass everything to the digest engine

    auto const head = path.filename().string();
    auto const tail = DigestEngine::digestToHex(sha1.digest());

    IDFObject obj(filename);

    TS_ASSERT_EQUALS(head + tail, obj.getMangledName());
  }

  void testGetFileFullPathStr() {
    const std::string filename =
        ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/IDF_for_UNIT_TESTING.xml";
    IDFObject obj(filename);
    TS_ASSERT_EQUALS(std::filesystem::path(filename).string(), obj.getFileFullPathStr());
  }
};
