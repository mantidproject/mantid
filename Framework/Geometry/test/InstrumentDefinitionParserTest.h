// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/ScopedFileHelper.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/WarningSuppressions.h"
#include <cxxtest/TestSuite.h>

#include <boost/algorithm/string/replace.hpp>
#include <gmock/gmock.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace testing;
using ScopedFileHelper::ScopedFile;

class InstrumentDefinitionParserTest : public CxxTest::TestSuite {
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
private:
  /// Mock Type to act as IDF files.
  class MockIDFObject : public Mantid::Geometry::IDFObject {
  public:
    MockIDFObject(const std::string &fileName) : Mantid::Geometry::IDFObject(fileName) {}
    MOCK_CONST_METHOD0(exists, bool());
  };

  /// Mock Type to act as IDF files.
  class MockIDFObjectWithParentDirectory : public Mantid::Geometry::IDFObject {
  public:
    MockIDFObjectWithParentDirectory(const std::string &fileName) : Mantid::Geometry::IDFObject(fileName) {}
    MOCK_CONST_METHOD0(exists, bool());
    MOCK_CONST_METHOD0(getParentDirectory, const Poco::Path());
  };
  GNU_DIAG_ON_SUGGEST_OVERRIDE
  /**
  Helper type to pass around related IDF environment information in a
  collection.
  */
  struct IDFEnvironment {
    IDFEnvironment(const ScopedFile &idf, const ScopedFile &vtp, const std::string &xmlText,
                   const std::string &instName)
        : _idf(idf), _vtp(vtp), _xmlText(xmlText), _instName(instName) {};

    ScopedFile _idf;
    ScopedFile _vtp;
    std::string _xmlText;
    std::string _instName;
  };

  /**
  Helper method to create a pair of corresponding resource managed, IDF and VTP
  files.
  */
  IDFEnvironment create_idf_and_vtp_pair(bool put_vtp_next_to_IDF = true) {
    const std::string instrument_name = "MinimalForTesting";
    const std::string idf_filename = instrument_name + "_Definition.xml";
    const std::string idf_file_contents = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                          "<instrument name=\"MinimalForTesting\" valid-from   =\"1900-01-31 "
                                          "23:59:59\" valid-to=\"2100-01-31 23:59:59\" "
                                          "last-modified=\"2012-10-05 11:00:00\">"
                                          "<defaults/>"
                                          "<component type=\"cylinder-right\" idlist=\"cylinder-right\">"
                                          "<location/>"
                                          "</component>"
                                          "<type name=\"cylinder-right\" is=\"detector\">"
                                          "<cylinder id=\"some-shape\">"
                                          "  <centre-of-bottom-base r=\"0.0\" t=\"0.0\" p=\"0.0\" />"
                                          "  <axis x=\"0.0\" y=\"0.0\" z=\"1.0\" />"
                                          "  <radius val=\"0.01\" />"
                                          "  <height val=\"0.03\" />"
                                          "</cylinder>"
                                          "</type>"
                                          "<idlist idname=\"cylinder-right\">"
                                          "<id val=\"1\" />"
                                          "</idlist>"
                                          "</instrument>";

    // expected name
    const std::string vtp_filename =
        instrument_name + ChecksumHelper::sha1FromString(Strings::strip(idf_file_contents)) + ".vtp";
    const std::string vtp_file_contents = "<VTKFile byte_order=\"LittleEndian\" type=\"PolyData\" "
                                          "version=\"1.0\"><PolyData/></VTKFile>";

    const std::string instrument_dir = ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/";
    std::string vtp_dir = ConfigService::Instance().getVTPFileDirectory();
    if (!put_vtp_next_to_IDF) {
      vtp_dir = ConfigService::Instance().getTempDir();
    }
    ScopedFile idf(idf_file_contents, idf_filename, instrument_dir);
    ScopedFile vtp(vtp_file_contents, vtp_filename, vtp_dir);

    return IDFEnvironment(idf, vtp, idf_file_contents, instrument_name);
  }

  // Helper method to create the IDF File.
  ScopedFile createIDFFileObject(const std::string &idf_filename, const std::string &idf_file_contents) {
    const std::string instrument_dir = ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/";

    return ScopedFile(idf_file_contents, idf_filename, instrument_dir);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentDefinitionParserTest *createSuite() { return new InstrumentDefinitionParserTest(); }
  static void destroySuite(InstrumentDefinitionParserTest *suite) { delete suite; }

  void test_extract_ref_info() {
    std::string filename =
        ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/IDF_for_UNIT_TESTING.xml";
    std::string xmlText = Strings::loadFile(filename);
    std::shared_ptr<const Instrument> i;

    // Parse the XML
    InstrumentDefinitionParser parser(filename, "For Unit Testing", xmlText);
    TS_ASSERT_THROWS_NOTHING(i = parser.parseXML(nullptr););

    // Extract the reference frame object
    std::shared_ptr<const ReferenceFrame> frame = i->getReferenceFrame();

    // Test that values have been populated with expected values (those from
    // file).
    TS_ASSERT_EQUALS(Right, frame->getHandedness());
    TS_ASSERT_EQUALS(Y, frame->pointingUp());
    TS_ASSERT_EQUALS(Z, frame->pointingAlongBeam());
    TS_ASSERT(frame->origin().empty());
  }

  void test_extract_ref_info_theta_sign() {
    std::string filename =
        ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/IDF_for_UNIT_TESTING6.xml";
    std::string xmlText = Strings::loadFile(filename);
    std::shared_ptr<const Instrument> i;

    // Parse the XML
    InstrumentDefinitionParser parser(filename, "For Unit Testing", xmlText);
    TS_ASSERT_THROWS_NOTHING(i = parser.parseXML(nullptr););

    // Extract the reference frame object
    std::shared_ptr<const ReferenceFrame> frame = i->getReferenceFrame();

    // Test that values have been populated with expected values (those from
    // file).
    TS_ASSERT_EQUALS(V3D(1., 0., 0.), frame->vecThetaSign());
  }

  void test_parse_IDF_for_unit_testing() // IDF stands for Instrument Definition File
  {
    std::string filenameNoExt =
        ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/IDF_for_UNIT_TESTING";
    std::string filename = filenameNoExt + ".xml";
    std::string xmlText = Strings::loadFile(filename);
    std::shared_ptr<const Instrument> i;

    InstrumentDefinitionParser parser(filename, "For Unit Testing", xmlText);

    // Parse the XML (remove old vtp file if it exists)
    std::string vtpFilename = parser.createVTPFileName();
    try {
      Poco::File vtpFile(vtpFilename);
      vtpFile.remove();
    } catch (Poco::FileNotFoundException &) {
    }

    TS_ASSERT_THROWS_NOTHING(i = parser.parseXML(nullptr););
    try {
      Poco::File vtpFile(vtpFilename);
      vtpFile.remove();
    } catch (Poco::FileNotFoundException &) {
      TS_FAIL("Cannot find expected .vtp file next to " + filename);
    }

    std::shared_ptr<const IObjComponent> source = std::dynamic_pointer_cast<const IObjComponent>(i->getSource());
    TS_ASSERT_EQUALS(source->getName(), "undulator");
    TS_ASSERT_DELTA(source->getPos().Z(), -17.0, 0.01);

    std::shared_ptr<const IComponent> samplepos = std::dynamic_pointer_cast<const IComponent>(i->getSample());
    TS_ASSERT_EQUALS(samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA(samplepos->getPos().Y(), 0.0, 0.01);

    std::shared_ptr<const IDetector> ptrDet1 = i->getDetector(1);
    TS_ASSERT_EQUALS(ptrDet1->getID(), 1);
    TS_ASSERT_DELTA(ptrDet1->getPos().X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Y(), 10.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet1->getPos().Z(), 0.0, 0.0001);
    double d = ptrDet1->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d, 10.0, 0.0001);
    double cmpDistance = ptrDet1->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance, 10.0, 0.0001);

    std::shared_ptr<const IDetector> ptrDet2 = i->getDetector(2);
    TS_ASSERT_EQUALS(ptrDet2->getID(), 2);
    TS_ASSERT_DELTA(ptrDet2->getPos().X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet2->getPos().Y(), -10.0, 0.0001);
    TS_ASSERT_DELTA(ptrDet2->getPos().Z(), 0.0, 0.0001);
    d = ptrDet2->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d, 10.0, 0.0001);
    cmpDistance = ptrDet2->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance, 10.0, 0.0001);

    // test if detectors face sample
    TS_ASSERT(!ptrDet1->isValid(V3D(0.02, 0.0, 0.0) + ptrDet1->getPos()));
    TS_ASSERT(!ptrDet1->isValid(V3D(-0.02, 0.0, 0.0) + ptrDet1->getPos()));
    TS_ASSERT(ptrDet1->isValid(V3D(0.0, 0.02, 0.0) + ptrDet1->getPos()));
    TS_ASSERT(!ptrDet1->isValid(V3D(0.0, -0.02, 0.0) + ptrDet1->getPos()));
    TS_ASSERT(!ptrDet1->isValid(V3D(0.0, 0.0, 0.02) + ptrDet1->getPos()));
    TS_ASSERT(!ptrDet1->isValid(V3D(0.0, 0.0, -0.02) + ptrDet1->getPos()));

    TS_ASSERT(!ptrDet2->isValid(V3D(0.02, 0.0, 0.0) + ptrDet2->getPos()));
    TS_ASSERT(!ptrDet2->isValid(V3D(-0.02, 0.0, 0.0) + ptrDet2->getPos()));
    TS_ASSERT(!ptrDet2->isValid(V3D(0.0, 0.02, 0.0) + ptrDet2->getPos()));
    TS_ASSERT(ptrDet2->isValid(V3D(0.0, -0.02, 0.0) + ptrDet2->getPos()));
    TS_ASSERT(!ptrDet2->isValid(V3D(0.0, 0.0, 0.02) + ptrDet2->getPos()));
    TS_ASSERT(!ptrDet2->isValid(V3D(0.0, 0.0, -0.02) + ptrDet2->getPos()));

    std::shared_ptr<const IDetector> ptrDet3 = i->getDetector(3);
    TS_ASSERT(!ptrDet3->isValid(V3D(0.02, 0.0, 0.0) + ptrDet3->getPos()));
    TS_ASSERT(!ptrDet3->isValid(V3D(-0.02, 0.0, 0.0) + ptrDet3->getPos()));
    TS_ASSERT(!ptrDet3->isValid(V3D(0.0, 0.02, 0.0) + ptrDet3->getPos()));
    TS_ASSERT(!ptrDet3->isValid(V3D(0.0, -0.02, 0.0) + ptrDet3->getPos()));
    TS_ASSERT(ptrDet3->isValid(V3D(0.0, 0.0, 0.02) + ptrDet3->getPos()));
    TS_ASSERT(!ptrDet3->isValid(V3D(0.0, 0.0, -0.02) + ptrDet3->getPos()));

    std::shared_ptr<const IDetector> ptrDet4 = i->getDetector(4);
    TS_ASSERT(!ptrDet4->isValid(V3D(0.02, 0.0, 0.0) + ptrDet4->getPos()));
    TS_ASSERT(!ptrDet4->isValid(V3D(-0.02, 0.0, 0.0) + ptrDet4->getPos()));
    TS_ASSERT(!ptrDet4->isValid(V3D(0.0, 0.02, 0.0) + ptrDet4->getPos()));
    TS_ASSERT(!ptrDet4->isValid(V3D(0.0, -0.02, 0.0) + ptrDet4->getPos()));
    TS_ASSERT(ptrDet4->isValid(V3D(0.0, 0.0, 0.02) + ptrDet4->getPos()));
    TS_ASSERT(!ptrDet4->isValid(V3D(0.0, 0.0, -0.02) + ptrDet4->getPos()));

    // test of facing as a sub-element of location
    std::shared_ptr<const IDetector> ptrDet5 = i->getDetector(5);
    TS_ASSERT(!ptrDet5->isValid(V3D(0.02, 0.0, 0.0) + ptrDet5->getPos()));
    TS_ASSERT(ptrDet5->isValid(V3D(-0.02, 0.0, 0.0) + ptrDet5->getPos()));
    TS_ASSERT(!ptrDet5->isValid(V3D(0.0, 0.02, 0.0) + ptrDet5->getPos()));
    TS_ASSERT(!ptrDet5->isValid(V3D(0.0, -0.02, 0.0) + ptrDet5->getPos()));
    TS_ASSERT(!ptrDet5->isValid(V3D(0.0, 0.0, 0.02) + ptrDet5->getPos()));
    TS_ASSERT(!ptrDet5->isValid(V3D(0.0, 0.0, -0.02) + ptrDet5->getPos()));

    // test of infinite-cone.
    std::shared_ptr<const IDetector> ptrDet6 = i->getDetector(6);
    TS_ASSERT(!ptrDet6->isValid(V3D(0.02, 0.0, 0.0) + ptrDet6->getPos()));
    TS_ASSERT(!ptrDet6->isValid(V3D(-0.02, 0.0, 0.0) + ptrDet6->getPos()));
    TS_ASSERT(!ptrDet6->isValid(V3D(0.0, 0.02, 0.0) + ptrDet6->getPos()));
    TS_ASSERT(!ptrDet6->isValid(V3D(0.0, -0.02, 0.0) + ptrDet6->getPos()));
    TS_ASSERT(!ptrDet6->isValid(V3D(0.0, 0.0, 0.02) + ptrDet6->getPos()));
    TS_ASSERT(ptrDet6->isValid(V3D(0.0, 0.0, -0.02) + ptrDet6->getPos()));
    TS_ASSERT(ptrDet6->isValid(V3D(0.0, 0.0, -1.02) + ptrDet6->getPos()));

    // test of (finite) cone.
    std::shared_ptr<const IDetector> ptrDet7 = i->getDetector(7);
    TS_ASSERT(!ptrDet7->isValid(V3D(0.02, 0.0, 0.0) + ptrDet7->getPos()));
    TS_ASSERT(!ptrDet7->isValid(V3D(-0.02, 0.0, 0.0) + ptrDet7->getPos()));
    TS_ASSERT(!ptrDet7->isValid(V3D(0.0, 0.02, 0.0) + ptrDet7->getPos()));
    TS_ASSERT(!ptrDet7->isValid(V3D(0.0, -0.02, 0.0) + ptrDet7->getPos()));
    TS_ASSERT(!ptrDet7->isValid(V3D(0.0, 0.0, 0.02) + ptrDet7->getPos()));
    TS_ASSERT(ptrDet7->isValid(V3D(0.0, 0.0, -0.02) + ptrDet7->getPos()));
    TS_ASSERT(!ptrDet7->isValid(V3D(0.0, 0.0, -1.02) + ptrDet7->getPos()));

    // test of hexahedron.
    std::shared_ptr<const IDetector> ptrDet8 = i->getDetector(8);
    TS_ASSERT(ptrDet8->isValid(V3D(0.4, 0.4, 0.0) + ptrDet8->getPos()));
    TS_ASSERT(ptrDet8->isValid(V3D(0.8, 0.8, 0.0) + ptrDet8->getPos()));
    TS_ASSERT(ptrDet8->isValid(V3D(0.4, 0.4, 2.0) + ptrDet8->getPos()));
    TS_ASSERT(!ptrDet8->isValid(V3D(0.8, 0.8, 2.0) + ptrDet8->getPos()));
    TS_ASSERT(!ptrDet8->isValid(V3D(0.0, 0.0, -0.02) + ptrDet8->getPos()));
    TS_ASSERT(!ptrDet8->isValid(V3D(0.0, 0.0, 2.02) + ptrDet8->getPos()));
    TS_ASSERT(ptrDet8->isValid(V3D(0.5, 0.5, 0.1) + ptrDet8->getPos()));

    // test of tapered-guide.
    std::shared_ptr<const IDetector> ptrDet9 = i->getDetector(9);
    TS_ASSERT(ptrDet9->isValid(V3D(2.0, -2.0, 1.0) + ptrDet9->getPos()));
    TS_ASSERT(ptrDet9->isValid(V3D(2.0, 2.0, 1.0) + ptrDet9->getPos()));
    TS_ASSERT(ptrDet9->isValid(V3D(-2.0, 2.0, 1.0) + ptrDet9->getPos()));
    TS_ASSERT(ptrDet9->isValid(V3D(-2.0, -2.0, 1.0) + ptrDet9->getPos()));
    TS_ASSERT(ptrDet9->isValid(V3D(1.0, -1.0, -1.0) + ptrDet9->getPos()));
    TS_ASSERT(ptrDet9->isValid(V3D(1.0, 1.0, -1.0) + ptrDet9->getPos()));
    TS_ASSERT(ptrDet9->isValid(V3D(-1.0, 1.0, -1.0) + ptrDet9->getPos()));
    TS_ASSERT(ptrDet9->isValid(V3D(-1.0, -1.0, -1.0) + ptrDet9->getPos()));
    TS_ASSERT(ptrDet9->isValid(V3D(1.5, -1.5, 0.0) + ptrDet9->getPos()));
    TS_ASSERT(ptrDet9->isValid(V3D(1.5, 1.5, 0.0) + ptrDet9->getPos()));
    TS_ASSERT(ptrDet9->isValid(V3D(-1.5, 1.5, 0.0) + ptrDet9->getPos()));
    TS_ASSERT(ptrDet9->isValid(V3D(-1.5, -1.5, 0.0) + ptrDet9->getPos()));
    TS_ASSERT(!ptrDet9->isValid(V3D(2.0, -2.0, 0.0) + ptrDet9->getPos()));
    TS_ASSERT(!ptrDet9->isValid(V3D(2.0, 2.0, 0.0) + ptrDet9->getPos()));
    TS_ASSERT(!ptrDet9->isValid(V3D(-2.0, 2.0, 0.0) + ptrDet9->getPos()));
    TS_ASSERT(!ptrDet9->isValid(V3D(-2.0, -2.0, 0.0) + ptrDet9->getPos()));

    // test for "cuboid-rotating-test".
    std::shared_ptr<const IDetector> ptrDet10 = i->getDetector(10);
    TS_ASSERT(ptrDet10->isValid(V3D(0.0, 0.0, 0.1) + ptrDet10->getPos()));
    TS_ASSERT(ptrDet10->isValid(V3D(0.0, 0.0, -0.1) + ptrDet10->getPos()));
    TS_ASSERT(ptrDet10->isValid(V3D(0.0, 0.02, 0.1) + ptrDet10->getPos()));
    TS_ASSERT(ptrDet10->isValid(V3D(0.0, 0.02, -0.1) + ptrDet10->getPos()));
    TS_ASSERT(!ptrDet10->isValid(V3D(0.0, 0.05, 0.0) + ptrDet10->getPos()));
    TS_ASSERT(!ptrDet10->isValid(V3D(0.0, -0.05, 0.0) + ptrDet10->getPos()));
    TS_ASSERT(!ptrDet10->isValid(V3D(0.0, -0.01, 0.05) + ptrDet10->getPos()));
    TS_ASSERT(!ptrDet10->isValid(V3D(0.0, -0.01, -0.05) + ptrDet10->getPos()));
    std::shared_ptr<const IDetector> ptrDet11 = i->getDetector(11);
    TS_ASSERT(ptrDet11->isValid(V3D(-0.07, 0.0, -0.07) + ptrDet11->getPos()));
    TS_ASSERT(ptrDet11->isValid(V3D(0.07, 0.0, 0.07) + ptrDet11->getPos()));
    TS_ASSERT(ptrDet11->isValid(V3D(0.07, 0.01, 0.07) + ptrDet11->getPos()));
    TS_ASSERT(ptrDet11->isValid(V3D(-0.07, 0.01, -0.07) + ptrDet11->getPos()));
    TS_ASSERT(!ptrDet11->isValid(V3D(0.0, 0.05, 0.0) + ptrDet11->getPos()));
    TS_ASSERT(!ptrDet11->isValid(V3D(0.0, -0.05, 0.0) + ptrDet11->getPos()));
    TS_ASSERT(!ptrDet11->isValid(V3D(0.0, -0.01, 0.05) + ptrDet11->getPos()));
    TS_ASSERT(!ptrDet11->isValid(V3D(0.0, -0.01, -0.05) + ptrDet11->getPos()));
    std::shared_ptr<const IDetector> ptrDet1000 = i->getDetector(1000);
    TS_ASSERT(ptrDet1000->isValid(V3D(0.0, 0.0, 0.1) + ptrDet1000->getPos()));
    TS_ASSERT(ptrDet1000->isValid(V3D(0.0, 0.0, -0.1) + ptrDet1000->getPos()));
    TS_ASSERT(ptrDet1000->isValid(V3D(0.0, 0.02, 0.1) + ptrDet1000->getPos()));
    TS_ASSERT(ptrDet1000->isValid(V3D(0.0, 0.02, -0.1) + ptrDet1000->getPos()));
    TS_ASSERT(!ptrDet1000->isValid(V3D(0.0, 0.05, 0.0) + ptrDet1000->getPos()));
    TS_ASSERT(!ptrDet1000->isValid(V3D(0.0, -0.05, 0.0) + ptrDet1000->getPos()));
    TS_ASSERT(!ptrDet1000->isValid(V3D(0.0, -0.01, 0.05) + ptrDet1000->getPos()));
    TS_ASSERT(!ptrDet1000->isValid(V3D(0.0, -0.01, -0.05) + ptrDet1000->getPos()));
    std::shared_ptr<const IDetector> ptrDet1001 = i->getDetector(1001);
    TS_ASSERT(ptrDet1001->isValid(V3D(-0.07, 0.0, -0.07) + ptrDet1001->getPos()));
    TS_ASSERT(ptrDet1001->isValid(V3D(0.07, 0.0, 0.07) + ptrDet1001->getPos()));
    TS_ASSERT(ptrDet1001->isValid(V3D(0.07, 0.01, 0.07) + ptrDet1001->getPos()));
    TS_ASSERT(ptrDet1001->isValid(V3D(-0.07, 0.01, -0.07) + ptrDet1001->getPos()));
    TS_ASSERT(!ptrDet1001->isValid(V3D(0.0, 0.05, 0.0) + ptrDet1001->getPos()));
    TS_ASSERT(!ptrDet1001->isValid(V3D(0.0, -0.05, 0.0) + ptrDet1001->getPos()));
    TS_ASSERT(!ptrDet1001->isValid(V3D(0.0, -0.01, 0.05) + ptrDet1001->getPos()));
    TS_ASSERT(!ptrDet1001->isValid(V3D(0.0, -0.01, -0.05) + ptrDet1001->getPos()));

    // test for "cuboid-alternate-test".
    std::shared_ptr<const IDetector> ptrDet18 = i->getDetector(18);

    TS_ASSERT(ptrDet18->isValid(V3D(1.05, 1.10, 1.20) + ptrDet18->getPos()));
    TS_ASSERT(ptrDet18->isValid(V3D(1.05, 1.10, 0.80) + ptrDet18->getPos()));
    TS_ASSERT(ptrDet18->isValid(V3D(1.05, 0.90, 1.20) + ptrDet18->getPos()));
    TS_ASSERT(ptrDet18->isValid(V3D(1.05, 0.90, 0.80) + ptrDet18->getPos()));
    TS_ASSERT(ptrDet18->isValid(V3D(0.95, 1.10, 1.20) + ptrDet18->getPos()));
    TS_ASSERT(ptrDet18->isValid(V3D(0.95, 1.10, 0.80) + ptrDet18->getPos()));
    TS_ASSERT(ptrDet18->isValid(V3D(0.95, 0.90, 1.20) + ptrDet18->getPos()));
    TS_ASSERT(ptrDet18->isValid(V3D(0.95, 0.90, 0.80) + ptrDet18->getPos()));

    TS_ASSERT(!ptrDet18->isValid(V3D(1.06, 1.11, 1.21) + ptrDet18->getPos()));
    TS_ASSERT(!ptrDet18->isValid(V3D(1.06, 1.11, 0.79) + ptrDet18->getPos()));
    TS_ASSERT(!ptrDet18->isValid(V3D(1.06, 0.89, 1.21) + ptrDet18->getPos()));
    TS_ASSERT(!ptrDet18->isValid(V3D(1.06, 0.89, 0.79) + ptrDet18->getPos()));
    TS_ASSERT(!ptrDet18->isValid(V3D(0.94, 1.11, 1.21) + ptrDet18->getPos()));
    TS_ASSERT(!ptrDet18->isValid(V3D(0.94, 1.11, 0.79) + ptrDet18->getPos()));
    TS_ASSERT(!ptrDet18->isValid(V3D(0.94, 0.89, 1.21) + ptrDet18->getPos()));
    TS_ASSERT(!ptrDet18->isValid(V3D(0.94, 0.89, 0.79) + ptrDet18->getPos()));

    // test for "infinite-cylinder-test".
    std::shared_ptr<const IDetector> ptrDet12 = i->getDetector(12);
    TS_ASSERT(ptrDet12->isValid(V3D(0.0, 0.0, 0.1) + ptrDet12->getPos()));
    TS_ASSERT(ptrDet12->isValid(V3D(0.0, 0.0, -0.1) + ptrDet12->getPos()));
    TS_ASSERT(ptrDet12->isValid(V3D(0.0, 0.1, 0.0) + ptrDet12->getPos()));
    TS_ASSERT(ptrDet12->isValid(V3D(0.0, -0.1, 0.0) + ptrDet12->getPos()));
    TS_ASSERT(ptrDet12->isValid(V3D(0.1, 0.0, 0.0) + ptrDet12->getPos()));
    TS_ASSERT(ptrDet12->isValid(V3D(-0.1, 0.0, 0.0) + ptrDet12->getPos()));
    TS_ASSERT(ptrDet12->isValid(V3D(0.0, 0.0, 0.0) + ptrDet12->getPos()));
    TS_ASSERT(!ptrDet12->isValid(V3D(2.0, 0.0, 0.0) + ptrDet12->getPos()));

    // test for "finite-cylinder-test".
    std::shared_ptr<const IDetector> ptrDet13 = i->getDetector(13);
    TS_ASSERT(ptrDet13->isValid(V3D(0.0, 0.0, 0.1) + ptrDet13->getPos()));
    TS_ASSERT(!ptrDet13->isValid(V3D(0.0, 0.0, -0.1) + ptrDet13->getPos()));
    TS_ASSERT(ptrDet13->isValid(V3D(0.0, 0.1, 0.0) + ptrDet13->getPos()));
    TS_ASSERT(ptrDet13->isValid(V3D(0.0, -0.1, 0.0) + ptrDet13->getPos()));
    TS_ASSERT(ptrDet13->isValid(V3D(0.1, 0.0, 0.0) + ptrDet13->getPos()));
    TS_ASSERT(ptrDet13->isValid(V3D(-0.1, 0.0, 0.0) + ptrDet13->getPos()));
    TS_ASSERT(ptrDet13->isValid(V3D(0.0, 0.0, 0.0) + ptrDet13->getPos()));
    TS_ASSERT(!ptrDet13->isValid(V3D(2.0, 0.0, 0.0) + ptrDet13->getPos()));

    // test for "complement-test".
    std::shared_ptr<const IDetector> ptrDet14 = i->getDetector(14);
    TS_ASSERT(!ptrDet14->isValid(V3D(0.0, 0.0, 0.0) + ptrDet14->getPos()));
    TS_ASSERT(!ptrDet14->isValid(V3D(0.0, 0.0, -0.04) + ptrDet14->getPos()));
    TS_ASSERT(ptrDet14->isValid(V3D(0.0, 0.0, -0.06) + ptrDet14->getPos()));
    TS_ASSERT(!ptrDet14->isValid(V3D(0.0, 0.04, 0.0) + ptrDet14->getPos()));
    TS_ASSERT(ptrDet14->isValid(V3D(0.0, 0.06, 0.0) + ptrDet14->getPos()));
    TS_ASSERT(!ptrDet14->isValid(V3D(0.06, 0.0, 0.0) + ptrDet14->getPos()));
    TS_ASSERT(!ptrDet14->isValid(V3D(0.51, 0.0, 0.0) + ptrDet14->getPos()));
    TS_ASSERT(!ptrDet14->isValid(V3D(0.0, 0.51, 0.0) + ptrDet14->getPos()));
    TS_ASSERT(!ptrDet14->isValid(V3D(0.0, 0.0, 0.51) + ptrDet14->getPos()));

    // test for "rotation-of-element-test".
    std::shared_ptr<const IDetector> ptrDet15 = i->getDetector(15);
    TS_ASSERT(!ptrDet15->isValid(V3D(0.0, 0.09, 0.01) + ptrDet15->getPos()));
    TS_ASSERT(!ptrDet15->isValid(V3D(0.0, -0.09, 0.01) + ptrDet15->getPos()));
    TS_ASSERT(ptrDet15->isValid(V3D(0.09, 0.0, 0.01) + ptrDet15->getPos()));
    TS_ASSERT(ptrDet15->isValid(V3D(-0.09, 0.0, 0.01) + ptrDet15->getPos()));
    std::shared_ptr<const IDetector> ptrDet16 = i->getDetector(16);
    TS_ASSERT(ptrDet16->isValid(V3D(0.0, 0.0, 0.09) + ptrDet16->getPos()));
    TS_ASSERT(ptrDet16->isValid(V3D(0.0, 0.0, -0.09) + ptrDet16->getPos()));
    TS_ASSERT(!ptrDet16->isValid(V3D(0.0, 0.09, 0.0) + ptrDet16->getPos()));
    TS_ASSERT(!ptrDet16->isValid(V3D(0.0, 0.09, 0.0) + ptrDet16->getPos()));
    std::shared_ptr<const IDetector> ptrDet17 = i->getDetector(17);
    TS_ASSERT(ptrDet17->isValid(V3D(0.0, 0.09, 0.01) + ptrDet17->getPos()));
    TS_ASSERT(ptrDet17->isValid(V3D(0.0, -0.09, 0.01) + ptrDet17->getPos()));
    TS_ASSERT(!ptrDet17->isValid(V3D(0.09, 0.0, 0.01) + ptrDet17->getPos()));
    TS_ASSERT(!ptrDet17->isValid(V3D(-0.09, 0.0, 0.01) + ptrDet17->getPos()));

    // test of source shape
    TS_ASSERT(source->isValid(V3D(0.0, 0.0, 0.005) + source->getPos()));
    TS_ASSERT(!source->isValid(V3D(0.0, 0.0, -0.005) + source->getPos()));
    TS_ASSERT(!source->isValid(V3D(0.0, 0.0, 0.02) + source->getPos()));

    // test <locations>
    std::shared_ptr<const IDetector> ptrDet100 = i->getDetector(100);
    TS_ASSERT_DELTA(ptrDet100->getPos().Z(), 0.0, 1e-8);
    std::shared_ptr<const IDetector> ptrDet109 = i->getDetector(109);
    TS_ASSERT_DELTA(ptrDet109->getPos().Z(), 1.0, 1e-8);
    std::shared_ptr<const IDetector> ptrDet110 = i->getDetector(110);
    TS_ASSERT_DELTA(ptrDet110->getPos().Y(), -1.0, 1e-8);
    TS_ASSERT_EQUALS(ptrDet110->getName(), "tube0");
    std::shared_ptr<const IDetector> ptrDet119 = i->getDetector(119);
    TS_ASSERT_DELTA(ptrDet119->getPos().Y(), 1.0, 1e-8);
    TS_ASSERT_EQUALS(ptrDet119->getName(), "tube9");
    std::shared_ptr<const IDetector> ptrDet120 = i->getDetector(120);
    TS_ASSERT_DELTA(ptrDet120->getPos().Y(), -1.0, 1e-8);
    TS_ASSERT_EQUALS(ptrDet120->getName(), "tube1");
    std::shared_ptr<const IDetector> ptrDet129 = i->getDetector(129);
    TS_ASSERT_DELTA(ptrDet129->getPos().Y(), 1.0, 1e-8);
    TS_ASSERT_EQUALS(ptrDet129->getName(), "tube10");

    std::shared_ptr<const IDetector> ptrDet200 = i->getDetector(200);
    TS_ASSERT_DELTA(ptrDet200->getPos().Y(), 0.0, 1e-8);
    std::shared_ptr<const IDetector> ptrDet209 = i->getDetector(209);
    TS_ASSERT_DELTA(ptrDet209->getPos().Y(), 1.0, 1e-8);

    // Check absence of distinct physical instrument
    TS_ASSERT(!i->getPhysicalInstrument());
  }

  void test_prase_IDF_for_unit_testing2() // IDF stands for Instrument
                                          // Definition File
  {
    std::string filename =
        ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/IDF_for_UNIT_TESTING2.xml";
    std::string xmlText = Strings::loadFile(filename);
    std::shared_ptr<const Instrument> i;

    // Parse the XML
    InstrumentDefinitionParser parser(filename, "For Unit Testing2", xmlText);
    TS_ASSERT_THROWS_NOTHING(i = parser.parseXML(nullptr););

    std::shared_ptr<const IDetector> ptrDetShape = i->getDetector(1100);
    TS_ASSERT_EQUALS(ptrDetShape->getID(), 1100);

    // test sample
    std::shared_ptr<const IComponent> sample = i->getSample();
    TS_ASSERT_EQUALS(sample->getName(), "nickel-holder");
    TS_ASSERT_DELTA(sample->getPos().X(), 2.0, 0.01);

    // test source
    std::shared_ptr<const IComponent> source = i->getSource();
    TS_ASSERT_EQUALS(source->getName(), "undulator");
    TS_ASSERT_DELTA(source->getPos().Z(), -95.0, 0.01);

    // Test of monitor shape
    std::shared_ptr<const IDetector> ptrMonShape = i->getDetector(1001);
    TS_ASSERT(ptrMonShape->isValid(V3D(0.002, 0.0, 0.0) + ptrMonShape->getPos()));
    TS_ASSERT(ptrMonShape->isValid(V3D(-0.002, 0.0, 0.0) + ptrMonShape->getPos()));
    TS_ASSERT(!ptrMonShape->isValid(V3D(0.003, 0.0, 0.0) + ptrMonShape->getPos()));
    TS_ASSERT(!ptrMonShape->isValid(V3D(-0.003, 0.0, 0.0) + ptrMonShape->getPos()));
    TS_ASSERT(ptrMonShape->isValid(V3D(-0.0069, 0.0227, 0.0) + ptrMonShape->getPos()));
    TS_ASSERT(!ptrMonShape->isValid(V3D(-0.0071, 0.0227, 0.0) + ptrMonShape->getPos()));
    TS_ASSERT(ptrMonShape->isValid(V3D(-0.0069, 0.0227, 0.009) + ptrMonShape->getPos()));
    TS_ASSERT(!ptrMonShape->isValid(V3D(-0.0069, 0.0227, 0.011) + ptrMonShape->getPos()));
    TS_ASSERT(ptrMonShape->isValid(V3D(-0.1242, 0.0, 0.0) + ptrMonShape->getPos()));
    TS_ASSERT(ptrMonShape->isValid(V3D(-0.0621, 0.0621, 0.0) + ptrMonShape->getPos()));
    TS_ASSERT(ptrMonShape->isValid(V3D(-0.0621, -0.0621, 0.0) + ptrMonShape->getPos()));
    TS_ASSERT(ptrMonShape->isValid(V3D(-0.0621, 0.0641, 0.0) + ptrMonShape->getPos()));
    TS_ASSERT(!ptrMonShape->isValid(V3D(-0.0621, 0.0651, 0.0) + ptrMonShape->getPos()));
    TS_ASSERT(!ptrMonShape->isValid(V3D(-0.0621, 0.0595, 0.0) + ptrMonShape->getPos()));
    TS_ASSERT(ptrMonShape->isValid(V3D(-0.0621, 0.0641, 0.01) + ptrMonShape->getPos()));
    TS_ASSERT(!ptrMonShape->isValid(V3D(-0.0621, 0.0641, 0.011) + ptrMonShape->getPos()));
    TS_ASSERT(!ptrMonShape->isValid(V3D(-0.0621, 0.0651, 0.01) + ptrMonShape->getPos()));
  }

  void test_parse_RectangularDetector() {
    std::string filename =
        ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/IDF_for_RECTANGULAR_UNIT_TESTING.xml";
    std::string xmlText = Strings::loadFile(filename);
    std::shared_ptr<const Instrument> i;

    // Parse the XML
    InstrumentDefinitionParser parser(filename, "RectangularUnitTest", xmlText);
    TS_ASSERT_THROWS_NOTHING(i = parser.parseXML(nullptr););

    // Now the XY detector in bank1
    std::shared_ptr<const RectangularDetector> bank1 =
        std::dynamic_pointer_cast<const RectangularDetector>(i->getComponentByName("bank1"));
    TS_ASSERT(bank1);
    if (!bank1)
      return;

    // Right # of x columns?
    TS_ASSERT_EQUALS(bank1->nelements(), 100);

    // Positions according to formula
    TS_ASSERT_DELTA(bank1->getAtXY(0, 0)->getPos().X(), -0.1, 1e-4);
    TS_ASSERT_DELTA(bank1->getAtXY(0, 0)->getPos().Y(), -0.2, 1e-4);
    TS_ASSERT_DELTA(bank1->getAtXY(1, 0)->getPos().X(), -0.098, 1e-4);
    TS_ASSERT_DELTA(bank1->getAtXY(1, 1)->getPos().Y(), -0.198, 1e-4);

    // Some IDs
    TS_ASSERT_EQUALS(bank1->getAtXY(0, 0)->getID(), 1000);
    TS_ASSERT_EQUALS(bank1->getAtXY(0, 1)->getID(), 1001);
    TS_ASSERT_EQUALS(bank1->getAtXY(1, 0)->getID(), 1300);
    TS_ASSERT_EQUALS(bank1->getAtXY(1, 1)->getID(), 1301);

    // The total number of detectors
    detid2det_map dets;
    i->getDetectors(dets);
    TS_ASSERT_EQUALS(dets.size(), 100 * 200 * 2);
  }

  void testGetAbsolutPositionInCompCoorSys() {
    CompAssembly base("base");
    base.setPos(1.0, 1.0, 1.0);
    base.rotate(Quat(90.0, V3D(0, 0, 1)));

    InstrumentDefinitionParser helper;
    V3D test = helper.getAbsolutPositionInCompCoorSys(&base, V3D(1, 0, 0));

    TS_ASSERT_DELTA(test.X(), 1.0, 0.0001);
    TS_ASSERT_DELTA(test.Y(), 2.0, 0.0001);
    TS_ASSERT_DELTA(test.Z(), 1.0, 0.0001);
  }

  // testing through Loading IDF_for_UNIT_TESTING5.xml method adjust()
  void testAdjust() {
    std::string filename =
        ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/IDF_for_UNIT_TESTING5.xml";
    std::string xmlText = Strings::loadFile(filename);
    std::shared_ptr<const Instrument> i;

    // Parse the XML
    InstrumentDefinitionParser parser(filename, "AdjustTest", xmlText);
    TS_ASSERT_THROWS_NOTHING(i = parser.parseXML(nullptr););

    // None rotated cuboid
    std::shared_ptr<const IDetector> ptrNoneRot = i->getDetector(1400);
    TS_ASSERT(!ptrNoneRot->isValid(V3D(0.0, 0.0, 0.0)));
    TS_ASSERT(ptrNoneRot->isValid(V3D(0.0, 0.0, 3.0)));
    TS_ASSERT(!ptrNoneRot->isValid(V3D(0.0, 4.5, 0.0)));
    TS_ASSERT(ptrNoneRot->isValid(V3D(0.0, 4.5, 3.0)));
    TS_ASSERT(!ptrNoneRot->isValid(V3D(0.0, 5.5, 3.0)));
    TS_ASSERT(!ptrNoneRot->isValid(V3D(4.5, 0.0, 3.0)));

    // rotated cuboids
    std::shared_ptr<const IDetector> ptrRot = i->getDetector(1300);
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 0.0, 0.0)));
    TS_ASSERT(!ptrRot->isValid(V3D(0.0, 0.0, 3.0)));
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 4.5, 0.0)));
    TS_ASSERT(!ptrRot->isValid(V3D(0.0, 4.5, 3.0)));
    TS_ASSERT(!ptrRot->isValid(V3D(0.0, 7.5, 0.0)));
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 10.0, 0.0)));
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 10.0, 4.5)));
    TS_ASSERT(!ptrRot->isValid(V3D(0.0, 10.0, 5.5)));
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 10.0, -4.5)));
    TS_ASSERT(!ptrRot->isValid(V3D(1.5, 10.0, 0.5)));
    TS_ASSERT(ptrRot->isValid(V3D(0.5, 10.0, 0.0)));
    TS_ASSERT(ptrRot->isValid(V3D(-0.5, 10.0, 0.0)));

    // nested rotated cuboids
    ptrRot = i->getDetector(1350);
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 0.0, 0.0)));
    TS_ASSERT(!ptrRot->isValid(V3D(0.0, 0.0, 3.0)));
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 4.5, 0.0)));
    TS_ASSERT(!ptrRot->isValid(V3D(0.0, 4.5, 3.0)));
    TS_ASSERT(!ptrRot->isValid(V3D(0.0, 7.5, 0.0)));
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 20.0, 0.0)));
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 20.0, 4.5)));
    TS_ASSERT(!ptrRot->isValid(V3D(0.0, 20.0, 5.5)));
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 20.0, -4.5)));
    TS_ASSERT(!ptrRot->isValid(V3D(1.5, 20.0, 0.5)));
    TS_ASSERT(ptrRot->isValid(V3D(0.5, 20.0, 0.0)));
    TS_ASSERT(ptrRot->isValid(V3D(-0.5, 20.0, 0.0)));

    // nested rotated cuboids which shape position moved
    ptrRot = i->getDetector(1360);
    TS_ASSERT(ptrRot->isValid(V3D(1.0, 0.0, 0.0)));
    TS_ASSERT(!ptrRot->isValid(V3D(1.0, 0.0, 3.0)));
    TS_ASSERT(ptrRot->isValid(V3D(1.0, 4.5, 0.0)));
    TS_ASSERT(!ptrRot->isValid(V3D(1.0, 4.5, 3.0)));
    TS_ASSERT(!ptrRot->isValid(V3D(1.0, 7.5, 0.0)));
    TS_ASSERT(ptrRot->isValid(V3D(1.0, 20.0, 0.0)));
    TS_ASSERT(ptrRot->isValid(V3D(1.0, 20.0, 4.5)));
    TS_ASSERT(!ptrRot->isValid(V3D(1.0, 20.0, 5.5)));
    TS_ASSERT(ptrRot->isValid(V3D(1.0, 20.0, -4.5)));
    TS_ASSERT(!ptrRot->isValid(V3D(2.5, 20.0, 0.5)));
    TS_ASSERT(ptrRot->isValid(V3D(1.5, 20.0, 0.0)));
    TS_ASSERT(ptrRot->isValid(V3D(0.5, 20.0, 0.0)));

    // nested rotated cuboids which shape position moved by the
    // opposite amount as the location of its parent
    ptrRot = i->getDetector(1370);
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 0.0, 0.0)));
    TS_ASSERT(!ptrRot->isValid(V3D(0.0, 0.0, 3.0)));
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 4.5, 0.0)));
    TS_ASSERT(!ptrRot->isValid(V3D(0.0, 4.5, 3.0)));
    TS_ASSERT(!ptrRot->isValid(V3D(0.0, 7.5, 0.0)));
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 20.0, 0.0)));
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 20.0, 4.5)));
    TS_ASSERT(!ptrRot->isValid(V3D(0.0, 20.0, 5.5)));
    TS_ASSERT(ptrRot->isValid(V3D(0.0, 20.0, -4.5)));
    TS_ASSERT(!ptrRot->isValid(V3D(1.5, 20.0, 0.5)));
    TS_ASSERT(ptrRot->isValid(V3D(0.5, 20.0, 0.0)));
    TS_ASSERT(ptrRot->isValid(V3D(-0.5, 20.0, 0.0)));
  }

  void testDefaultCaching() {
    InstrumentDefinitionParser parser;
    TS_ASSERT_EQUALS(InstrumentDefinitionParser::NoneApplied, parser.getAppliedCachingOption());
  }

  void testCreateVTPFilename() {
    IDFEnvironment instrumentEnv = create_idf_and_vtp_pair();
    const std::string idfFileName = instrumentEnv._idf.getFileName();
    const std::string instrumentName = instrumentEnv._instName;
    const std::string xmlText = Strings::loadFile(idfFileName);
    InstrumentDefinitionParser parser(idfFileName, instrumentName, xmlText);

    TS_ASSERT_EQUALS(instrumentEnv._vtp.getFileName(), parser.createVTPFileName());
  }

  void testReadFromCacheInTempDirectory() {
    const bool put_vtp_in_instrument_directory = false;
    IDFEnvironment instrumentEnv = create_idf_and_vtp_pair(put_vtp_in_instrument_directory);

    const std::string idfFileName = instrumentEnv._idf.getFileName();
    const std::string cacheFileName = instrumentEnv._vtp.getFileName();

    MockIDFObject *mockIDF = new MockIDFObject(idfFileName);
    MockIDFObject *mockCache = new MockIDFObject(cacheFileName);

    EXPECT_CALL(*mockIDF, exists()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockCache, exists()).WillRepeatedly(Return(false)); // Mock expectation set such that
                                                                     // geometry Cache file does not exist,
                                                                     // so should not be used.

    IDFObject_const_sptr idf(mockIDF);
    IDFObject_const_sptr cache(mockCache);

    InstrumentDefinitionParser parser(idf, cache, instrumentEnv._instName, instrumentEnv._xmlText);

    TS_ASSERT_THROWS_NOTHING(parser.parseXML(nullptr));

    TS_ASSERT_EQUALS(InstrumentDefinitionParser::ReadFallBack,
                     parser.getAppliedCachingOption()); // Check that the
                                                        // geometry cache file
                                                        // was used.
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockIDF));
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockCache));
  }

  void RemoveFallbackVTPFile(InstrumentDefinitionParser &parser) {
    Poco::Path vtpPath(parser.createVTPFileName());
    Poco::Path fallbackPath(ConfigService::Instance().getTempDir());
    fallbackPath.makeDirectory();
    fallbackPath.setFileName(vtpPath.getFileName());
    std::string fp = fallbackPath.toString();
    Poco::File fallbackFile(fallbackPath.toString());
    if (fallbackFile.exists()) {
      fallbackFile.remove();
    }
  }

  void testWriteCacheFileIfCacheDoestExist() {
    IDFEnvironment instrumentEnv = create_idf_and_vtp_pair();

    const std::string idfFileName = instrumentEnv._idf.getFileName();
    const std::string cacheFileName = "";

    MockIDFObject *mockIDF = new MockIDFObject(idfFileName);
    MockIDFObject *mockCache = new MockIDFObject(cacheFileName);

    // make sure the fallback location for the geometry file is deleted

    EXPECT_CALL(*mockIDF, exists()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockCache, exists()).WillRepeatedly(Return(false)); // Mock expectation set such that Cache
                                                                     // file does not exist, so should not be
                                                                     // used.

    IDFObject_const_sptr idf(mockIDF);
    IDFObject_const_sptr cache(mockCache);

    // remove the fallback file if it exists
    InstrumentDefinitionParser parser(idf, cache, instrumentEnv._instName, instrumentEnv._xmlText);
    RemoveFallbackVTPFile(parser);

    TS_ASSERT_THROWS_NOTHING(parser.parseXML(nullptr));

    TS_ASSERT_EQUALS(InstrumentDefinitionParser::WroteGeomCache,
                     parser.getAppliedCachingOption()); // Check that the
                                                        // geometry cache file
                                                        // was used.
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockIDF));
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockCache));
  }

  /**
  The point of this test is to ensure that if a xml file is not provided, a
  cache is always written to the temp directory.
  This is because it will not be possible to perform date modification
  comparisons.
  */
  void testWriteCacheFileToTempDirectoryIfNoIDF() {
    IDFEnvironment instrumentEnv = create_idf_and_vtp_pair();

    const std::string idfFileName = "";                                 // We provide no IDF
    const std::string cacheFileName = instrumentEnv._vtp.getFileName(); // We do provide a cache file, but
                                                                        // this shouldn't be used.

    MockIDFObject *mockIDF = new MockIDFObject(idfFileName);
    MockIDFObjectWithParentDirectory *mockCache = new MockIDFObjectWithParentDirectory(cacheFileName);

    EXPECT_CALL(*mockIDF, exists()).WillRepeatedly(Return(false));   // IDF set not to exist.
    EXPECT_CALL(*mockCache, exists()).WillRepeatedly(Return(false)); // Mock expectation set such that Cache
                                                                     // file does not exist, and location is
                                                                     // inaccessible.
    EXPECT_CALL(*mockCache, getParentDirectory()).WillRepeatedly(Return(Poco::Path("this does not exist")));

    IDFObject_const_sptr idf(mockIDF);
    IDFObject_const_sptr cache(mockCache);

    InstrumentDefinitionParser parser(idf, cache, instrumentEnv._instName, instrumentEnv._xmlText);

    TS_ASSERT_THROWS_NOTHING(parser.parseXML(nullptr));

    TS_ASSERT_EQUALS(InstrumentDefinitionParser::WroteCacheTemp,
                     parser.getAppliedCachingOption()); // Check that the TEMP
                                                        // cache file was used.
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockIDF));
    TS_ASSERT(Mock::VerifyAndClearExpectations(mockCache));

    // Have to manually clean-up because this file is not tracked and is
    // generated by the InstrumentDefinitionParser.
    Poco::Path path(Mantid::Kernel::ConfigService::Instance().getTempDir().c_str());
    path.append(parser.getMangledName() + ".vtp");
    remove(path.toString().c_str());
  }

  /**
   Here we test that the correct exception is thrown if a detector location
   element is missing its detector ID list
  */
  void testIDFFileWithMissingDetectorIDList() {
    const std::string instrumentName = "Minimal_Definition";
    const std::string idfFilename = instrumentName + "_MissingDetectorIDs.xml";

    const std::string idfFileContents = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                        "<instrument name=\"" +
                                        instrumentName +
                                        "\" valid-from   =\"1900-01-31 23:59:59\" valid-to=\"2100-01-31 "
                                        "23:59:59\" last-modified=\"2012-10-05 11:00:00\">"
                                        "<defaults/>"
                                        "<component type=\"cylinder-right\" >" // Missing idlist here
                                        "<location/>"
                                        "</component>"
                                        "<type name=\"cylinder-right\" is=\"detector\">"
                                        "<cylinder id=\"some-shape\">"
                                        "  <centre-of-bottom-base r=\"0.0\" t=\"0.0\" p=\"0.0\" />"
                                        "  <axis x=\"0.0\" y=\"0.0\" z=\"1.0\" />"
                                        "  <radius val=\"0.01\" />"
                                        "  <height val=\"0.03\" />"
                                        "</cylinder>"
                                        "</type>"
                                        "<idlist idname=\"cylinder-right\">"
                                        "<id val=\"1\" />"
                                        "</idlist>"
                                        "</instrument>";

    ScopedFile idfFile = createIDFFileObject(idfFilename, idfFileContents);

    InstrumentDefinitionParser parser(idfFilename, "For Unit Testing", idfFileContents);

    std::string errorMsg("");
    try {
      parser.parseXML(nullptr);
      errorMsg = "Exception not thrown";
    } catch (Kernel::Exception::InstrumentDefinitionError &e) {
      errorMsg = e.what();
    } catch (...) {
      errorMsg = "Unexpected exception";
    }
    TS_ASSERT_EQUALS(errorMsg.substr(0, 25), "Detector location element");
  }

  Instrument_sptr loadInstrLocations(const std::string &locations, detid_t numDetectors, bool rethrow = false) {
    std::string filename =
        ConfigService::Instance().getInstrumentDirectory() + "/unit_testing/IDF_for_locations_test.xml";

    std::string contents = Strings::loadFile(filename);

    boost::replace_first(contents, "%LOCATIONS%", locations);
    boost::replace_first(contents, "%NUM_DETECTORS%", boost::lexical_cast<std::string>(numDetectors));

    InstrumentDefinitionParser parser(filename, "LocationsTestInstrument", contents);

    Instrument_sptr instr;

    if (rethrow)
      instr = parser.parseXML(nullptr);
    else
      TS_ASSERT_THROWS_NOTHING(instr = parser.parseXML(nullptr));

    TS_ASSERT_EQUALS(instr->getNumberDetectors(), numDetectors);

    return instr;
  }

  void testLocationsNaming() {
    std::string locations = R"(<locations n-elements=" 5" name-count-start=" 10" name="det" />)";
    detid_t numDetectors = 5;

    Instrument_sptr instr = loadInstrLocations(locations, numDetectors);

    TS_ASSERT_EQUALS(instr->getDetector(1)->getName(), "det10");
    TS_ASSERT_EQUALS(instr->getDetector(3)->getName(), "det12");
    TS_ASSERT_EQUALS(instr->getDetector(5)->getName(), "det14");
  }

  void testLocationsIncrement() {
    std::string locations = R"(<locations n-elements="3" name-count-start="1" name-count-increment="2" name="det" />)";
    detid_t numDetectors = 3;

    Instrument_sptr instr = loadInstrLocations(locations, numDetectors);

    TS_ASSERT_EQUALS(instr->getDetector(1)->getName(), "det1");
    TS_ASSERT_EQUALS(instr->getDetector(2)->getName(), "det3");
    TS_ASSERT_EQUALS(instr->getDetector(3)->getName(), "det5");
  }

  void testLocationsIncrementDefaultsToOne() {
    std::string locations = R"(<locations n-elements="3" name-count-start="5" name="det" />)";
    detid_t numDetectors = 3;

    Instrument_sptr instr = loadInstrLocations(locations, numDetectors);

    TS_ASSERT_EQUALS(instr->getDetector(1)->getName(), "det5");
    TS_ASSERT_EQUALS(instr->getDetector(2)->getName(), "det6");
    TS_ASSERT_EQUALS(instr->getDetector(3)->getName(), "det7");
  }

  void testLocationsIncrementFailsAtOrBelowZero() {
    std::string locations = R"(<locations n-elements="3" name-count-start="5" name-count-increment="0" name="det" />)";
    detid_t numDetectors = 3;

    TS_ASSERT_THROWS(loadInstrLocations(locations, numDetectors, true), const Exception::InstrumentDefinitionError &);

    locations = R"(<locations n-elements="3" name-count-start="5" name-count-increment="-7" name="det" />)";

    TS_ASSERT_THROWS(loadInstrLocations(locations, numDetectors, true), const Exception::InstrumentDefinitionError &);
  }

  void testLocationsStaticValues() {
    std::string locations = R"(<locations n-elements="5" x=" 1.0" y=" 2.0" z=" 3.0" />)";
    detid_t numDetectors = 5;

    Instrument_sptr instr = loadInstrLocations(locations, numDetectors);

    for (detid_t i = 1; i <= numDetectors; ++i) {
      TS_ASSERT_DELTA(instr->getDetector(i)->getPos().X(), 1.0, 1.0E-8);
      TS_ASSERT_DELTA(instr->getDetector(i)->getPos().Y(), 2.0, 1.0E-8);
      TS_ASSERT_DELTA(instr->getDetector(i)->getPos().Z(), 3.0, 1.0E-8);
    }
  }

  void testLocationsRanges() {
    std::string locations = "<locations n-elements=\"5\" x=\"1.0\" x-end=\"5.0\"  "
                            "                            y=\"4.0\" y-end=\"1.0\"  "
                            "                            z=\"3.0\" z-end=\"3.0\"/>";
    detid_t numDetectors = 5;

    Instrument_sptr instr = loadInstrLocations(locations, numDetectors);

    TS_ASSERT_DELTA(instr->getDetector(1)->getPos().X(), 1.0, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(1)->getPos().Y(), 4.0, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(1)->getPos().Z(), 3.0, 1.0E-8);

    TS_ASSERT_DELTA(instr->getDetector(3)->getPos().X(), 3.0, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(3)->getPos().Y(), 2.5, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(3)->getPos().Z(), 3.0, 1.0E-8);

    TS_ASSERT_DELTA(instr->getDetector(5)->getPos().X(), 5.0, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(5)->getPos().Y(), 1.0, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(5)->getPos().Z(), 3.0, 1.0E-8);
  }

  void checkDetectorRot(const IDetector_const_sptr &det, double deg, double axisx, double axisy, double axisz) {
    double detDeg, detAxisX, detAxisY, detAxisZ;
    det->getRotation().getAngleAxis(detDeg, detAxisX, detAxisY, detAxisZ);

    TS_ASSERT_DELTA(deg, detDeg, 1.0E-8);
    TS_ASSERT_DELTA(axisx, detAxisX, 1.0E-8);
    TS_ASSERT_DELTA(axisy, detAxisY, 1.0E-8);
    TS_ASSERT_DELTA(axisz, detAxisZ, 1.0E-8);
  }

  void testLocationsMixed() {
    // Semicircular placement, like the one for e.g. MERLIN or IN5
    std::string locations = "<locations n-elements=\"7\" r=\"0.5\" t=\"0.0\" t-end=\"180.0\" "
                            "           rot=\"0.0\" rot-end=\"180.0\" axis-x=\"0.0\" "
                            "           axis-y=\"1.0\" axis-z=\"0.0\"/>";
    detid_t numDetectors = 7;

    Instrument_sptr instr = loadInstrLocations(locations, numDetectors);

    // Left-most (r = 0.5, t, rot = 0)
    TS_ASSERT_DELTA(instr->getDetector(1)->getPos().X(), 0, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(1)->getPos().Y(), 0, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(1)->getPos().Z(), 0.5, 1.0E-8);
    checkDetectorRot(instr->getDetector(1), 0, 0, 0,
                     1); // Special case for null rotation

    // Next to left-most (r = 0.5, t, rot = 30)
    TS_ASSERT_DELTA(instr->getDetector(2)->getPos().X(), 0.25, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(2)->getPos().Y(), 0, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(2)->getPos().Z(), 0.433, 1.0E-4);
    checkDetectorRot(instr->getDetector(2), 30, 0, 1, 0);

    // The one directly in front (r = 0.5, t, rot = 90)
    TS_ASSERT_DELTA(instr->getDetector(4)->getPos().X(), 0.5, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(4)->getPos().Y(), 0, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(4)->getPos().Z(), 0, 1.0E-8);
    checkDetectorRot(instr->getDetector(4), 90, 0, 1, 0);

    // Right-most to the one directly in front (r = 0.5, t, rot = 120)
    TS_ASSERT_DELTA(instr->getDetector(5)->getPos().X(), 0.433, 1.0E-4);
    TS_ASSERT_DELTA(instr->getDetector(5)->getPos().Y(), 0, 1.0E-8);
    TS_ASSERT_DELTA(instr->getDetector(5)->getPos().Z(), -0.25, 1.0E-8);
    checkDetectorRot(instr->getDetector(5), 120, 0, 1, 0);
  }

  void testLocationsInvalidNoElements() {
    std::string locations = R"(<locations n-elements="0" t="0.0" t-end="180.0" />)";
    detid_t numDetectors = 2;

    TS_ASSERT_THROWS(loadInstrLocations(locations, numDetectors, true), const Exception::InstrumentDefinitionError &);

    locations = R"(<locations n-elements="-1" t="0.0" t-end="180.0" />)";

    TS_ASSERT_THROWS(loadInstrLocations(locations, numDetectors, true), const Exception::InstrumentDefinitionError &);
  }

  void testLocationsNotANumber() {
    std::string locations = R"(<locations n-elements="2" t="0.0" t-end="180.x" />)";
    detid_t numDetectors = 2;

    TS_ASSERT_THROWS_ANYTHING(loadInstrLocations(locations, numDetectors, true));

    locations = R"(<locations n-elements="2" t="0.x" t-end="180.0" />)";

    TS_ASSERT_THROWS_ANYTHING(loadInstrLocations(locations, numDetectors, true));

    locations = R"(<locations n-elements="x" t="0.0" t-end="180.0" />)";
    TS_ASSERT_THROWS_ANYTHING(loadInstrLocations(locations, numDetectors, true));

    locations = R"(<locations n-elements="2" name-count-start="x"/>)";
    TS_ASSERT_THROWS_ANYTHING(loadInstrLocations(locations, numDetectors, true));
  }

  void testLocationsNoCorrespondingStartAttr() {
    std::string locations = R"(<locations n-elements="2" t-end="180.0" />)";
    detid_t numDetectors = 2;

    TS_ASSERT_THROWS(loadInstrLocations(locations, numDetectors, true), const Exception::InstrumentDefinitionError &);
  }
};

class InstrumentDefinitionParserTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentDefinitionParserTestPerformance *createSuite() {
    return new InstrumentDefinitionParserTestPerformance();
  }
  static void destroySuite(InstrumentDefinitionParserTestPerformance *suite) { delete suite; }

  InstrumentDefinitionParserTestPerformance()
      : m_instrumentDirectoryPath(ConfigService::Instance().getInstrumentDirectory()) {}

  void testLoadingAndParsing() {
    const std::string filename = m_instrumentDirectoryPath + "/unit_testing/IDF_for_UNIT_TESTING.xml";
    const std::string xmlText = Strings::loadFile(filename);

    std::shared_ptr<const Instrument> instrument;
    InstrumentDefinitionParser parser(filename, "For Unit Testing", xmlText);
    TS_ASSERT_THROWS_NOTHING(instrument = parser.parseXML(nullptr));

    // Clean up VTP file
    const std::string vtpFilename = parser.createVTPFileName();
    if (!vtpFilename.empty()) {
      Poco::File(vtpFilename).remove();
    }
  }

  void test_load_wish() {
    const auto definition = m_instrumentDirectoryPath + "/WISH_Definition_10Panels.xml";
    std::string contents = Strings::loadFile(definition);
    InstrumentDefinitionParser parser(definition, "dummy", contents);
    auto wishInstrument = parser.parseXML(nullptr);
    TS_ASSERT_EQUALS(extractDetectorInfo(*wishInstrument)->size(),
                     778245); // Sanity check
  }

  void test_load_sans2d() {
    const auto definition = m_instrumentDirectoryPath + "/SANS2D_Definition_Tubes.xml";
    std::string contents = Strings::loadFile(definition);
    InstrumentDefinitionParser parser(definition, "dummy", contents);
    auto sansInstrument = parser.parseXML(nullptr);
    TS_ASSERT_EQUALS(extractDetectorInfo(*sansInstrument)->size(),
                     122888); // Sanity check
  }

private:
  const std::string m_instrumentDirectoryPath;

  std::unique_ptr<Geometry::DetectorInfo> extractDetectorInfo(const Mantid::Geometry::Instrument &instrument) {
    Geometry::ParameterMap pmap;
    return std::move(std::get<1>(instrument.makeBeamline(pmap)));
  }
};
