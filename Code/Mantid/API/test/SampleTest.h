#ifndef TESTSAMPLE_H_
#define TESTSAMPLE_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/Path.h"
#include "Poco/File.h"


using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::API::Sample;

class SampleTest : public CxxTest::TestSuite
{
public:
  void testSetGetName()
  {
    TS_ASSERT( ! sample.getName().compare("") )
    sample.setName("test");
    TS_ASSERT( ! sample.getName().compare("test") )
  }

  void testShape()
  {
    
    std::string xmlShape = "<cylinder id=\"shape\"> ";
    xmlShape +=	"<centre-of-bottom-base x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
    xmlShape +=	"<axis x=\"0.0\" y=\"1.0\" z=\"0\" /> " ;
    xmlShape +=	"<radius val=\"0.0127\" /> " ;
    xmlShape +=	"<height val=\"1\" /> " ;
    xmlShape +=	"</cylinder>";
    xmlShape +=	"<algebra val=\"shape\" /> ";  
    std::string shapeXML = "<type name=\"userShape\"> " + xmlShape + " </type>";
    
    // Set up the DOM parser and parse xml string
    Poco::XML::DOMParser pParser;
    Poco::XML::Document* pDoc;
    
    pDoc = pParser.parseString(shapeXML);
    
    // Get pointer to root element
    Poco::XML::Element* pRootElem = pDoc->documentElement();
    
    //convert into a Geometry object
    ShapeFactory sFactory;
    boost::shared_ptr<Object> shape_sptr = sFactory.createShape(pRootElem);
    pDoc->release();
    
    TS_ASSERT_THROWS_NOTHING(sample.setShapeObject(*shape_sptr))
    const Object & sampleShape = sample.getShapeObject();
    TS_ASSERT_EQUALS(shape_sptr->getName(), sampleShape.getName());
  }

private:

  Sample sample;
};

#endif /*TESTSAMPLE_H_*/
