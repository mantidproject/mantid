// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECPARSERTEST_H_
#define MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECPARSERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SampleEnvironmentSpecParser.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidKernel/Material.h"

#include "Poco/AutoPtr.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/SAX/InputSource.h"

#include <sstream>

using Mantid::DataHandling::SampleEnvironmentSpec;
using Mantid::DataHandling::SampleEnvironmentSpecParser;

class SampleEnvironmentSpecParserTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleEnvironmentSpecParserTest *createSuite() {
    return new SampleEnvironmentSpecParserTest();
  }
  static void destroySuite(SampleEnvironmentSpecParserTest *suite) {
    delete suite;
  }

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Single_Can_Single_Material_With_SampleGeometry() {
    using Mantid::Geometry::Container_const_sptr;

    const std::string name = "CRYO001";
    auto spec = parseSpec(name, "<environmentspec>"
                                " <materials>"
                                "  <material id=\"van\" formula=\"V\"/>"
                                " </materials>"
                                " <components>"
                                "  <containers>"
                                "   <container id=\"10mm\" material=\"van\">"
                                "    <geometry>"
                                "     <sphere id=\"sp-1\">"
                                "      <radius val=\"0.01\"/>"
                                "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                                "     </sphere>"
                                "    </geometry>"
                                "    <samplegeometry>"
                                "     <sphere id=\"sp-1\">"
                                "      <radius val=\"0.01\"/>"
                                "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                                "     </sphere>"
                                "    </samplegeometry>"
                                "   </container>"
                                "  </containers>"
                                " </components>"
                                "</environmentspec>");

    TS_ASSERT_EQUALS(name, spec->name());
    TS_ASSERT_EQUALS(1, spec->ncans());
    TS_ASSERT_EQUALS(0, spec->ncomponents());
    Container_const_sptr can10mm;
    TS_ASSERT_THROWS_NOTHING(can10mm = spec->findContainer("10mm"));
    TS_ASSERT(can10mm);
    TS_ASSERT_EQUALS("10mm", can10mm->id());
    TS_ASSERT(can10mm->hasValidShape());
    TS_ASSERT(can10mm->hasCustomizableSampleShape());
  }

  void test_Single_Can_Single_Material_With_No_SampleGeometry() {
    using Mantid::Geometry::Container_const_sptr;

    const std::string name = "CRYO001";
    auto spec = parseSpec(name, "<environmentspec>"
                                " <materials>"
                                "  <material id=\"van\" formula=\"V\"/>"
                                " </materials>"
                                " <components>"
                                "  <containers>"
                                "   <container id=\"10mm\" material=\"van\">"
                                "    <geometry>"
                                "     <sphere id=\"sp-1\">"
                                "      <radius val=\"0.01\"/>"
                                "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                                "     </sphere>"
                                "    </geometry>"
                                "   </container>"
                                "  </containers>"
                                " </components>"
                                "</environmentspec>");

    TS_ASSERT_EQUALS(name, spec->name());
    TS_ASSERT_EQUALS(1, spec->ncans());
    TS_ASSERT_EQUALS(0, spec->ncomponents());
    Container_const_sptr can10mm;
    TS_ASSERT_THROWS_NOTHING(can10mm = spec->findContainer("10mm"));
    TS_ASSERT(can10mm);
    TS_ASSERT_EQUALS("10mm", can10mm->id());
    TS_ASSERT(can10mm->hasValidShape());
    TS_ASSERT(!can10mm->hasCustomizableSampleShape());
  }

  void test_Single_Can_And_Single_Component_With_SampleGeometry() {
    using Mantid::Geometry::Container_const_sptr;

    const std::string name = "CRYO001";
    auto spec = parseSpec(name, "<environmentspec>"
                                " <materials>"
                                "  <material id=\"van\" formula=\"V\"/>"
                                "  <material id=\"alum\" formula=\"Al\"/>"
                                " </materials>"
                                " <components>"
                                "  <containers>"
                                "   <container id=\"10mm\" material=\"van\">"
                                "    <geometry>"
                                "     <sphere id=\"sp-1\">"
                                "      <radius val=\"0.01\"/>"
                                "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                                "     </sphere>"
                                "    </geometry>"
                                "    <samplegeometry>"
                                "     <sphere id=\"sp-1\">"
                                "      <radius val=\"0.01\"/>"
                                "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                                "     </sphere>"
                                "    </samplegeometry>"
                                "   </container>"
                                "  </containers>"
                                "  <component id=\"outer\" material=\"alum\">"
                                "    <geometry>"
                                "     <sphere id=\"sp-1\">"
                                "      <radius val=\"0.05\"/>"
                                "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                                "     </sphere>"
                                "    </geometry>"
                                "  </component>"
                                " </components>"
                                "</environmentspec>");

    TS_ASSERT_EQUALS(name, spec->name());
    TS_ASSERT_EQUALS(1, spec->ncans());
    Container_const_sptr can10mm;
    TS_ASSERT_THROWS_NOTHING(can10mm = spec->findContainer("10mm"));
    TS_ASSERT(can10mm);
    TS_ASSERT_EQUALS("10mm", can10mm->id());
    TS_ASSERT(can10mm->hasValidShape());
    TS_ASSERT_EQUALS("van", can10mm->material().name());
    TS_ASSERT(can10mm->hasCustomizableSampleShape());
    TS_ASSERT_EQUALS(1, spec->ncomponents());
  }

  void test_Multiple_Cans_And_Muliple_Componenents_With_SampleGeometry() {
    using Mantid::Geometry::Container_const_sptr;

    const std::string name = "CRYO001";
    auto spec = parseSpec(name, "<environmentspec>"
                                " <materials>"
                                "  <material id=\"van\" formula=\"V\"/>"
                                "  <material id=\"alum\" formula=\"Al\"/>"
                                " </materials>"
                                " <components>"
                                "  <containers>"
                                "   <container id=\"8mm\" material=\"alum\">"
                                "    <geometry>"
                                "     <sphere id=\"sp-1\">"
                                "      <radius val=\"0.05\"/>"
                                "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                                "     </sphere>"
                                "    </geometry>"
                                "    <samplegeometry>"
                                "     <sphere id=\"sp-1\">"
                                "      <radius val=\"0.1\"/>"
                                "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                                "     </sphere>"
                                "    </samplegeometry>"
                                "   </container>"
                                "   <container id=\"10mm\" material=\"van\">"
                                "    <geometry>"
                                "     <sphere id=\"sp-1\">"
                                "      <radius val=\"0.1\"/>"
                                "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                                "     </sphere>"
                                "    </geometry>"
                                "    <samplegeometry>"
                                "     <sphere id=\"sp-1\">"
                                "      <radius val=\"0.1\"/>"
                                "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                                "     </sphere>"
                                "    </samplegeometry>"
                                "   </container>"
                                "  </containers>"
                                "  <component id=\"outer1\" material=\"alum\">"
                                "    <geometry>"
                                "     <sphere id=\"sp-1\">"
                                "      <radius val=\"0.5\"/>"
                                "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                                "     </sphere>"
                                "    </geometry>"
                                "  </component>"
                                "  <component id=\"outer2\" material=\"alum\">"
                                "    <geometry>"
                                "     <sphere id=\"sp-1\">"
                                "      <radius val=\"0.75\"/>"
                                "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                                "     </sphere>"
                                "    </geometry>"
                                "  </component>"
                                " </components>"
                                "</environmentspec>");

    TS_ASSERT_EQUALS(name, spec->name());
    TS_ASSERT_EQUALS(2, spec->ncomponents());
    TS_ASSERT_EQUALS(2, spec->ncans());
    // 10mm
    Container_const_sptr can10mm;
    TS_ASSERT_THROWS_NOTHING(can10mm = spec->findContainer("10mm"));
    TS_ASSERT(can10mm);
    TS_ASSERT_EQUALS("10mm", can10mm->id());
    TS_ASSERT(can10mm->hasValidShape());
    TS_ASSERT_EQUALS("van", can10mm->material().name());
    TS_ASSERT(can10mm->hasCustomizableSampleShape());
    // 8mm
    Container_const_sptr can8mm;
    TS_ASSERT_THROWS_NOTHING(can8mm = spec->findContainer("8mm"));
    TS_ASSERT(can8mm);
    TS_ASSERT_EQUALS("8mm", can8mm->id());
    TS_ASSERT(can8mm->hasValidShape());
    TS_ASSERT_EQUALS("alum", can8mm->material().name());
    TS_ASSERT(can8mm->hasCustomizableSampleShape());
  }

  void test_Single_Can_Single_Material_With_SampleGeometry_STL() {
    using Mantid::Geometry::Container_const_sptr;

    const std::string name = "CRYO001";

    auto spec = parseSpec(name, R"(<environmentspec>
                           <materials>
                            <material id="van" formula="V"/>
                           </materials>
                           <components>
                            <containers>
                             <container id="10mm" material="van" >
                              <stlfile filename ="Sphere10units.stl" scale="mm">
                              </stlfile>
                              <samplestlfile filename ="Sphere10units.stl" scale="mm">
                              </samplestlfile>
                             </container>
                            </containers>
                           </components>
                           </environmentspec>)");

    TS_ASSERT_EQUALS(name, spec->name());
    TS_ASSERT_EQUALS(1, spec->ncans());
    TS_ASSERT_EQUALS(0, spec->ncomponents());
    Container_const_sptr can10mm;
    TS_ASSERT_THROWS_NOTHING(can10mm = spec->findContainer("10mm"));
    TS_ASSERT(can10mm);
    TS_ASSERT_EQUALS("10mm", can10mm->id());
    TS_ASSERT(can10mm->hasValidShape());
    TS_ASSERT_EQUALS(can10mm->hasCustomizableSampleShape(), false);
  }

  void test_Single_Can_And_Single_Component_With_SampleGeometry_STL() {
    using Mantid::Geometry::Container_const_sptr;

    const std::string name = "CRYO001";
    auto spec = parseSpec(name, R"(<environmentspec>
                           <materials>
                            <material id="van" formula="V"/>
                            <material id="alum" formula="Al"/>
                           </materials>
                           <components>
                            <containers>
                             <container id="10mm" material="van">
                              <stlfile filename ="Sphere10units.stl" scale="mm">
                              </stlfile>
                              <samplestlfile filename ="Sphere10units.stl" scale="mm">
                              </samplestlfile>
                             </container>
                            </containers>
                            <component id="outer" material="alum">
                             <stlfile filename ="Sphere10units.stl" scale="cm">
                             </stlfile>
                            </component>
                           </components>
                          </environmentspec>)");

    TS_ASSERT_EQUALS(name, spec->name());
    TS_ASSERT_EQUALS(1, spec->ncans());
    Container_const_sptr can10mm;
    TS_ASSERT_THROWS_NOTHING(can10mm = spec->findContainer("10mm"));
    TS_ASSERT(can10mm);
    TS_ASSERT_EQUALS("10mm", can10mm->id());
    TS_ASSERT(can10mm->hasValidShape());
    TS_ASSERT_EQUALS("van", can10mm->material().name());
    TS_ASSERT_EQUALS(can10mm->hasCustomizableSampleShape(), false);
    TS_ASSERT_EQUALS(1, spec->ncomponents());
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Empty_Source_Throws_Error() {
    std::string xml;
    std::istringstream instream(xml);
    SampleEnvironmentSpecParser parser;
    TS_ASSERT_THROWS(parser.parse("name", "", instream),
                     const std::runtime_error &);
  }

  void test_Root_Tag_Must_Be_EnvironmentSpec() {
    TS_ASSERT_THROWS(parseSpec("name", "<materials>"
                                       "</materials>"),
                     const std::invalid_argument &);
  }

  void test_Missing_Geometry_Tag_Under_Can_Throws_Error() {
    using Mantid::Geometry::Container_const_sptr;

    const std::string name = "CRYO001";
    TS_ASSERT_THROWS(parseSpec(name,
                               "<environmentspec>"
                               " <materials>"
                               "  <material id=\"van\" formula=\"V\"/>"
                               " </materials>"
                               " <components>"
                               "  <containers>"
                               "   <container id=\"10mm\" material=\"van\">"
                               "     <sphere id=\"sp-1\">"
                               "      <radius val=\"0.1\"/>"
                               "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                               "     </sphere>"
                               "    <samplegeometry>"
                               "     <sphere id=\"sp-1\">"
                               "      <radius val=\"0.1\"/>"
                               "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                               "     </sphere>"
                               "    </samplegeometry>"
                               "   </container>"
                               "  </containers>"
                               " </components>"
                               "</environmentspec>"),
                     const std::runtime_error &);
  }

  void test_Missing_Can_ID_Throws_Error() {
    using Mantid::Geometry::Container_const_sptr;

    const std::string name = "CRYO001";
    TS_ASSERT_THROWS(parseSpec(name,
                               "<environmentspec>"
                               " <materials>"
                               "  <material id=\"van\" formula=\"V\"/>"
                               " </materials>"
                               " <components>"
                               "  <containers>"
                               "   <container material=\"van\">"
                               "     <sphere id=\"sp-1\">"
                               "      <radius val=\"0.1\"/>"
                               "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                               "     </sphere>"
                               "    <samplegeometry>"
                               "     <sphere id=\"sp-1\">"
                               "      <radius val=\"0.1\"/>"
                               "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                               "     </sphere>"
                               "    </samplegeometry>"
                               "   </container>"
                               "  </containers>"
                               " </components>"
                               "</environmentspec>"),
                     const std::runtime_error &);
  }

  void test_Missing_Material_For_Can_Throws_Error() {
    using Mantid::Geometry::Container_const_sptr;

    const std::string name = "CRYO001";
    TS_ASSERT_THROWS(parseSpec(name,
                               "<environmentspec>"
                               " <materials>"
                               "  <material id=\"van\" formula=\"V\"/>"
                               " </materials>"
                               " <components>"
                               "  <containers>"
                               "   <container id=\"10mm\">"
                               "     <sphere id=\"sp-1\">"
                               "      <radius val=\"0.1\"/>"
                               "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                               "     </sphere>"
                               "    <samplegeometry>"
                               "     <sphere id=\"sp-1\">"
                               "      <radius val=\"0.1\"/>"
                               "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                               "     </sphere>"
                               "    </samplegeometry>"
                               "   </container>"
                               "  </containers>"
                               " </components>"
                               "</environmentspec>"),
                     const std::runtime_error &);
  }

  void test_Single_Can_Single_Material_With_TwoGeometrys_Throws_Error() {
    using Mantid::Geometry::Container_const_sptr;

    const std::string name = "CRYO001";

    TS_ASSERT_THROWS(parseSpec(name, R"(<environmentspec>
                           <materials>
                            <material id="van" formula="V"/>
                           </materials>
                           <components>
                            <containers>
                             <container id="10mm" material="van" >
                              <stlfile filename ="Sphere10units.stl" scale="mm">
                              </stlfile>
                              <geometry>
                               <sphere id="sp-1">
                                <radius val="0.01"/>
                                <centre x="0.0" y="0.0" z="0.0"/>
                               </sphere>
                              </geometry>
                             </container>
                            </containers>
                           </components>
                           </environmentspec>)"),
                     const std::runtime_error &);
  }

  void test_Missing_Scale_For_STLFile_Throws_Error() {
    using Mantid::Geometry::Container_const_sptr;

    const std::string name = "CRYO001";

    TS_ASSERT_THROWS(parseSpec(name, R"(<environmentspec>
                           <materials>
                            <material id="van" formula="V"/>
                           </materials>
                           <components>
                            <containers>
                             <container id="10mm" material="van" >
                              <stlfile filename ="Sphere10units.stl" >
                              </stlfile>
                             </container>
                            </containers>
                           </components>
                           </environmentspec>)"),
                     const std::runtime_error &);
  }

  void test_Invalid_STLFileName_Throws_Error() {
    using Mantid::Geometry::Container_const_sptr;

    const std::string name = "CRYO001";

    TS_ASSERT_THROWS(parseSpec(name, R"(<environmentspec>
                           <materials>
                            <material id="van" formula="V"/>
                           </materials>
                           <components>
                            <containers>
                             <container id="10mm" material="van" >
                              <stlfile filename ="InvalidFilename.stl" scale="mm">
                              </stlfile>
                             </container>
                            </containers>
                           </components>
                           </environmentspec>)"),
                     const std::runtime_error &);
  }

  //----------------------------------------------------------------------------
  // Non-test methods
  //----------------------------------------------------------------------------
private:
  Mantid::DataHandling::SampleEnvironmentSpec_uptr
  parseSpec(const std::string &name, const std::string &text) {
    std::istringstream instream(text);
    SampleEnvironmentSpecParser parser;
    return parser.parse(name, "", instream);
  }
};

#endif /* MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECPARSERTEST_H_ */
