#ifndef MANTID_GEOMETRY_COMPONENTPARSERTEST_H_
#define MANTID_GEOMETRY_COMPONENTPARSERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidGeometry/ComponentParser.h"
#include <Poco/SAX/SAXParser.h>

using namespace Mantid;
using namespace Mantid::Geometry;

class ComponentParserTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentParserTest *createSuite() { return new ComponentParserTest(); }
  static void destroySuite( ComponentParserTest *suite ) { delete suite; }


  void test_parse()
  {
    std::string xml = "<Component name=\"aName\"> </Component>";

    ComponentParser compParser;
    Poco::XML::SAXParser parser;
    parser.setContentHandler(&compParser);

    try
    {
      parser.parseMemoryNP(xml.c_str(), xml.size());
    }
    catch ( Poco::Exception & e )
    {
      TS_FAIL( e.displayText() );
    }

    Component * comp = compParser.getComponent();
    TS_ASSERT(comp);
    TS_ASSERT_EQUALS( comp->getName(), "aName");

  }


};


#endif /* MANTID_GEOMETRY_COMPONENTPARSERTEST_H_ */

