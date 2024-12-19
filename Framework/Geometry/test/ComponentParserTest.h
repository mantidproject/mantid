// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/ComponentParser.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include <Poco/SAX/AttributesImpl.h>
#include <Poco/SAX/SAXParser.h>
#include <cxxtest/TestSuite.h>
#include <sstream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Poco::XML;
using NeXus::File;

class ComponentParserTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentParserTest *createSuite() { return new ComponentParserTest(); }
  static void destroySuite(ComponentParserTest *suite) { delete suite; }

  void test_todo() {}

  //  void test_parse()
  //  {
  //    Component comp1("SlimShady", NULL);
  //    comp1.setPos(V3D(1,2,3));
  //    comp1.setRot(Quat(4,5,6,7));
  //    std::string xml;
  //    std::ostringstream str;
  //    size_t num = 1000000;
  //    CPUTimer tim;
  //    std::cout << '\n';
  //
  //    if (false)
  //    {
  //      tim.reset();
  //      Poco::XML::XMLWriter writer(str, XMLWriter::WRITE_XML_DECLARATION );
  //      writer.startFragment();
  //      //comp1.writeXML(writer);
  //      for (size_t i=0; i<num; i++)
  //      {
  //        AttributesImpl attr;
  //        attr.addAttribute("", "name", "", "", "SlimShady");
  //        writer.startElement("", "Component", "", attr);
  //        writer.startElement("", "pos", "");
  //        writer.characters(comp1.getRelativePos().toString());
  //        writer.endElement("", "pos", "");
  //        writer.startElement("", "rot", "");
  //        writer.characters(comp1.getRelativeRot().toString());
  //        writer.endElement("", "rot", "");
  //        writer.endElement("", "Component", "");
  //      }
  //      writer.endFragment();
  //      std::cout << tim << " to write out simple XML with Poco XMLWriter. "
  //      << str.str().size() << " chars.\n";
  //      //std::cout << str.str().substr(0, 500);
  //    }
  //
  //    if (true)
  //    {
  //      tim.reset();
  //      std::ostringstream xmlStr;
  //      for (size_t i=0; i<num; i++)
  //      {
  //        xmlStr << "<" << comp1.typeName() << " name=\"" << comp1.getName()
  //        << "\">\n";
  //        comp1.appendXML(xmlStr);
  //        xmlStr << "</" << comp1.typeName() << ">\n";
  //      }
  //      std::cout << tim << " to write out simple XML by manually appending
  //      strings (fastest possible). " << xmlStr.str().size() << " chars." <<
  //      '\n';
  //      //std::cout << xmlStr.str().substr(0, 500);
  //      xml = xmlStr.str();
  //      tim.reset();
  //    }
  //
  //    if (true)
  //    {
  //      ::NeXus::File * file = new ::NeXus::File("components.nxs",
  //      NXACC_CREATE5);
  //      file->makeGroup("test_comps", "NXdata",1);
  //      std::vector<double> posArray(num*3);
  //      std::vector<double> rotArray(num*4);
  //      //std::vector<std::string> names(num);
  //      std::string names;
  //      for (size_t i=0; i<num; i++)
  //      {
  //        size_t index = i*3;
  //        V3D pos = comp1.getRelativePos();
  //        posArray[index] = pos.X();
  //        posArray[index+1] = pos.Y();
  //        posArray[index+2] = pos.Z();
  //        index = i*4;
  //        Quat rot = comp1.getRelativeRot();
  //        for (size_t j=0; j<4; j++)
  //          rotArray[index+j] = rot[j];
  //        //names[i] = comp1.getName();
  //        names += comp1.getName() + "\n";
  //      }
  //      file->writeData("pos", posArray);
  //      file->writeData("rot", rotArray);
  //      file->writeData("names", names);
  //      file->close();
  //      std::cout << tim << " to write make a NXS array " << num << "
  //      entries.\n";
  //    }
  //
  //    xml = "<root>" + xml + "</root>";
  //    if (xml.size() < 400) std::cout << xml << std::endl << '\n';
  //    tim.reset();
  //
  //    ComponentParser compParser;
  //    Poco::XML::SAXParser parser;
  //    parser.setContentHandler(&compParser);
  //
  //    try
  //    {
  //      parser.parseMemoryNP(xml.c_str(), xml.size());
  //    }
  //    catch ( Poco::Exception & e )
  //    {
  //      TS_FAIL( e.displayText() );
  //    }
  //
  //    TS_ASSERT_EQUALS( compParser.size(), num);
  //
  //    Component * comp = compParser.getComponent();
  //    TS_ASSERT(comp);
  //    TS_ASSERT_EQUALS( comp->getName(), "SlimShady");
  //    TS_ASSERT_EQUALS( comp->getRotation(), Quat(4,5,6,7));
  //    TS_ASSERT_EQUALS( comp->getPos(), V3D(1,2,3));
  //
  //    std::cout << tim << " to parse " << num << " entries.\n";
  //  }
};
