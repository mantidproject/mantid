// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_MATERIALXMLPARSERTEST_H_
#define MANTID_KERNEL_MATERIALXMLPARSERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/MaterialXMLParser.h"
#include "Poco/AutoPtr.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/SAX/InputSource.h"
#include <sstream>

using Mantid::Kernel::MaterialXMLParser;

class MaterialXMLParserTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaterialXMLParserTest *createSuite() {
    return new MaterialXMLParserTest();
  }
  static void destroySuite(MaterialXMLParserTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success tests
  //
  // The assumption here is that the complex logic of building the material is
  // tested by the MaterialBuilderTest. Therefore, here we just test that
  // all of the attributes are handled.
  //----------------------------------------------------------------------------
  void test_Formula_Attribute() {
    auto mat = parseMaterial(R"(<material id="vanadium" formula="V"/>)");

    TS_ASSERT_EQUALS("vanadium", mat.name());
    TS_ASSERT_DELTA(0.07223047, mat.numberDensity(), 1e-8);
  }

  void test_AtomicNumber_Attribute() {
    auto mat = parseMaterial(
        R"(<material id="n" atomicnumber="28" numberdensity="0.12"/>)");

    TS_ASSERT_DELTA(mat.totalScatterXSection(), 18.5, 0.0001);
    TS_ASSERT_DELTA(mat.absorbXSection(), 4.49, 0.0001);
  }

  void test_MassNumber_Attribute() {
    auto mat = parseMaterial("<material id=\"n\" atomicnumber=\"28\" "
                             "massnumber=\"58\" numberdensity=\"0.12\"/>");

    TS_ASSERT_DELTA(mat.totalScatterXSection(), 26.1, 0.0001);
    TS_ASSERT_DELTA(mat.absorbXSection(), 4.6, 0.0001);
  }

  void test_NumberDensity_Attribute() {
    auto mat = parseMaterial("<material id=\"n\" atomicnumber=\"28\" "
                             "massnumber=\"58\" numberdensity=\"0.12\"/>");

    TS_ASSERT_DELTA(0.12, mat.numberDensity(), 1e-04);
  }

  void test_ZParameter_And_UnitCellVolume_Attributes() {
    auto mat = parseMaterial("<material id=\"a\" formula=\"Al2-O3\" "
                             "zparameter=\"6\" unitcellvol=\"253.54\"/>");

    TS_ASSERT_DELTA(mat.numberDensity(), 0.1183245, 1e-07);
  }

  void test_MassDensity_Attribute() {
    auto mat = parseMaterial("<material id=\"a\" formula=\"Al2-O3\" "
                             "massdensity=\"4\" />");

    TS_ASSERT_DELTA(mat.numberDensity(), 0.0236252 * 5, 1e-07);
  }

  void test_TotalScattering_Attribute() {
    auto mat =
        parseMaterial("<material id=\"a\" formula=\"Al2-O3\" "
                      "totalscatterxsec=\"18.1\" numberdensity=\"0.12\"/>");

    TS_ASSERT_DELTA(mat.totalScatterXSection(), 18.1, 1e-04);
  }

  void test_CoherentScattering_Attribute() {
    auto mat = parseMaterial("<material id=\"a\" formula=\"Al2-O3\" "
                             "cohscatterxsec=\"4.6\" numberdensity=\"0.12\"/>");

    TS_ASSERT_DELTA(mat.cohScatterXSection(), 4.6, 1e-04);
  }

  void test_IncoherentScattering_Attribute() {
    auto mat =
        parseMaterial("<material id=\"a\" formula=\"Al2-O3\" "
                      "incohscatterxsec=\"0.1\" numberdensity=\"0.12\"/>");

    TS_ASSERT_DELTA(mat.incohScatterXSection(), 0.1, 1e-04);
  }

  void test_Absorption_Attribute() {
    auto mat =
        parseMaterial("<material id=\"a\" formula=\"Al2-O3\" "
                      "absorptionxsec=\"4.45\" numberdensity=\"0.12\"/>");

    TS_ASSERT_DELTA(mat.absorbXSection(), 4.45, 1e-04);
  }

  void test_Read_Valid_XML_Returns_Expected_Material_From_Stream_Source() {
    const std::string xml = "<material id=\"vanadium\" formula=\"V\">"
                            "</material>";
    std::istringstream instream(xml);
    MaterialXMLParser parser;
    auto matr = parser.parse(instream);

    TS_ASSERT_EQUALS("vanadium", matr.name());
    TS_ASSERT_DELTA(0.07223047, matr.numberDensity(), 1e-8);
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Empty_Source_Throws_Error() {
    std::string xml;
    std::istringstream instream(xml);
    MaterialXMLParser parser;
    TS_ASSERT_THROWS(parser.parse(instream), const std::invalid_argument &);
  }

  void test_Missing_Or_Empty_Id_Tag_Throws_Error() {
    TS_ASSERT_THROWS(parseMaterial("<material formula=\"V\">"
                                   "</material>"),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS(parseMaterial("<material id=\"\" formula=\"V\">"
                                   "</material>"),
                     const std::invalid_argument &);
  }

  void test_Unknown_Attribute_Throws_Error() {
    TS_ASSERT_THROWS(parseMaterial("<material id=\"n\" atomic=\"28\">"
                                   "</material>"),
                     const std::runtime_error &);
  }

  //----------------------------------------------------------------------------
  // Non-test methods
  //----------------------------------------------------------------------------
private:
  Mantid::Kernel::Material parseMaterial(const std::string &text) {
    using Poco::AutoPtr;
    using namespace Poco::XML;
    AutoPtr<Document> doc = createXMLDocument(text);
    AutoPtr<NodeList> elements = doc->getElementsByTagName("material");

    MaterialXMLParser parser;
    return parser.parse(static_cast<Element *>(elements->item(0)));
  }

  Poco::AutoPtr<Poco::XML::Document>
  createXMLDocument(const std::string &text) {
    using Poco::AutoPtr;
    using namespace Poco::XML;
    std::istringstream instream(text);
    InputSource src(instream);
    DOMParser parser;
    return parser.parse(&src);
  }
};

#endif /* MANTID_KERNEL_MATERIALXMLPARSERTEST_H_ */
