#ifndef SHAPEFACTORYTEST_H_
#define SHAPEFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Object.h"
#include <vector>

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;


using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class ShapeFactoryTest : public CxxTest::TestSuite
{
public:


	void testCuboid()
	{
		std::string xmlShape = "<cuboid id=\"shape\"> ";
		xmlShape +=	"<left-front-bottom-point x=\"0.005\" y=\"-0.1\" z=\"0.0\" /> " ;
  	xmlShape +=	"<left-front-top-point x=\"0.005\" y=\"-0.1\" z=\"0.0001\" />  " ;
  	xmlShape +=	"<left-back-bottom-point x=\"-0.005\" y=\"-0.1\" z=\"0.0\" />  " ;
  	xmlShape +=	"<right-front-bottom-point x=\"0.005\" y=\"0.1\" z=\"0.0\" />  " ;
  	xmlShape +=	"</cuboid> ";
		xmlShape +=	"<algebra val=\"shape\" /> ";  
		
		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape);

    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.0,0.00001)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,0.001)) );
    TS_ASSERT( shape_sptr->isValid(V3D(-0.004,0.0,0.00001)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(-0.006,0.0,0.00001)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.09, 0.00001)) );
	}


	void testHexahedron()
	{
		std::string xmlShape = "<hexahedron id=\"shape\"> ";
    xmlShape +=	"<left-back-bottom-point  x=\"0.0\" y=\"0.0\" z=\"0.0\"  /> " ;
    xmlShape +=	"<left-front-bottom-point x=\"1.0\" y=\"0.0\" z=\"0.0\"  /> " ;
    xmlShape +=	"<right-front-bottom-point x=\"1.0\" y=\"1.0\" z=\"0.0\"  /> " ;
    xmlShape +=	"<right-back-bottom-point  x=\"0.0\" y=\"1.0\" z=\"0.0\"  /> " ;
    xmlShape +=	"<left-back-top-point  x=\"0.0\" y=\"0.0\" z=\"2.0\"  /> " ;
    xmlShape +=	"<left-front-top-point  x=\"0.5\" y=\"0.0\" z=\"2.0\"  /> " ;
    xmlShape +=	"<right-front-top-point  x=\"0.5\" y=\"0.5\" z=\"2.0\"  /> " ;
    xmlShape +=	"<right-back-top-point  x=\"0.0\" y=\"0.5\" z=\"2.0\"  /> " ;
  	xmlShape +=	"</hexahedron> ";
		xmlShape +=	"<algebra val=\"shape\" /> ";  
		
		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape);

    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.0,0.0)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(1.1,0.0,0.0)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.9,0.9,0.0)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.49,0.49,1.99)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.49,0.81, 1.99)) );
	}
	
	void testHexahedron2()
	{
		std::string xmlShape = "<hexahedron id=\"shape\"> ";
    xmlShape +=	"<left-front-bottom-point x=\"0.0\" y=\"-0.0031\" z=\"-0.037\"  /> " ;
    xmlShape +=	"<right-front-bottom-point x=\"0.0\" y=\"0.0031\" z=\"-0.037\"  /> " ;
    xmlShape +=	"<left-front-top-point x=\"0.0\" y=\"-0.0104\" z=\"0.037\"  /> " ;
    xmlShape +=	"<right-front-top-point x=\"0.0\" y=\"0.0104\" z=\"0.037\"  /> " ;
    xmlShape +=	"<left-back-bottom-point x=\"0.005\" y=\"-0.0031\" z=\"-0.037\"  /> " ;
    xmlShape +=	"<right-back-bottom-point x=\"0.005\" y=\"0.0031\" z=\"-0.037\"  /> " ;
    xmlShape +=	"<left-back-top-point x=\"0.005\" y=\"-0.0104\" z=\"0.037\"  /> " ;
    xmlShape +=	"<right-back-top-point x=\"0.005\" y=\"0.0104\" z=\"0.037\"  /> " ;
  	xmlShape +=	"</hexahedron> ";
		xmlShape +=	"<algebra val=\"shape\" /> ";  
		
		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape);

    TS_ASSERT( shape_sptr->isValid(V3D(0.0001,0.0,0.0)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0055,0.0,0.0)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.004,0.003,0.003)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.-0.003,-0.036)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,-0.003, -0.038)) );
	}

	void testSphere()
	{
		//algebra line is essential
		std::string xmlShape = "<sphere id=\"shape\"> ";
		xmlShape +=	"<centre x=\"4.1\"  y=\"2.1\" z=\"8.1\" /> " ;
  	xmlShape +=	"<radius val=\"3.2\" /> " ;
  	xmlShape +=	"</sphere>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape);

    TS_ASSERT( shape_sptr->isValid(V3D(4.1,2.1,8.1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(47.1,2.1,8.1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(5.1,2.1,8.1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(-0.006,0.0,0.00001)) );
    TS_ASSERT( shape_sptr->isValid(V3D(4.1,2.1,9.1)) );
	}

	void testTwoSpheres()
	{
		//algebra line is essential
		std::string xmlShape = "<sphere id=\"shape1\"> ";
		xmlShape +=	"<centre x=\"4.1\"  y=\"2.1\" z=\"8.1\" /> " ;
  	xmlShape +=	"<radius val=\"3.2\" /> " ;
  	xmlShape +=	"</sphere>";
    xmlShape +=	"<sphere id=\"shape2\"> ";
		xmlShape +=	"<centre x=\"2.1\"  y=\"2.1\" z=\"8.1\" /> " ;
  	xmlShape +=	"<radius val=\"3.2\" /> " ;
  	xmlShape +=	"</sphere>";
    xmlShape +=	"<algebra val=\"shape1 : shape2\" /> ";  

		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape);

    TS_ASSERT( shape_sptr->isValid(V3D(4.1,2.1,8.1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(47.1,2.1,8.1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(5.1,2.1,8.1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(-0.006,0.0,0.00001)) );
    TS_ASSERT( shape_sptr->isValid(V3D(4.1,2.1,9.1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(-0.8,2.1,9.1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(7.1,2.1,9.1)) );
	}

	void testTwoSpheresNoAlgebraString()
	{
		//algebra line is essential
		std::string xmlShape = "<sphere id=\"shape1\"> ";
		xmlShape +=	"<centre x=\"4.1\"  y=\"2.1\" z=\"8.1\" /> " ;
  	xmlShape +=	"<radius val=\"3.2\" /> " ;
  	xmlShape +=	"</sphere>";
    xmlShape +=	"<sphere id=\"shape2\"> ";
		xmlShape +=	"<centre x=\"2.1\"  y=\"2.1\" z=\"8.1\" /> " ;
  	xmlShape +=	"<radius val=\"3.2\" /> " ;
  	xmlShape +=	"</sphere>";

		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape);

    TS_ASSERT( shape_sptr->isValid(V3D(4.1,2.1,8.1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(47.1,2.1,8.1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(5.1,2.1,8.1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(-0.006,0.0,0.00001)) );
    TS_ASSERT( shape_sptr->isValid(V3D(4.1,2.1,9.1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(-0.8,2.1,9.1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(7.1,2.1,9.1)) );
	}

	void testCylinder()
	{
		//algebra line is essential
		std::string xmlShape = "<cylinder id=\"shape\"> ";
		xmlShape +=	"<centre-of-bottom-base x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
  	xmlShape +=	"<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> " ;
  	xmlShape +=	"<radius val=\"0.1\" /> " ;
  	xmlShape +=	"<height val=\"3\" /> " ;
  	xmlShape +=	"</cylinder>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape);

    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.0,1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,10)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.05,1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.15,1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.01,0.01,1)) );
	}

	void testCylinderNoAlgebraString()
	{
		//algebra line is essential
		std::string xmlShape = "<cylinder id=\"shape\"> ";
		xmlShape +=	"<centre-of-bottom-base x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
  	xmlShape +=	"<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> " ;
  	xmlShape +=	"<radius val=\"0.1\" /> " ;
  	xmlShape +=	"<height val=\"3\" /> " ;
  	xmlShape +=	"</cylinder>";

		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape);

    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.0,1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,10)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.05,1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.15,1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.01,0.01,1)) );
	}

	void testCylinderTwoAlgebraStrings()
	{
		//algebra line is essential
		std::string xmlShape = "<cylinder id=\"shape\"> ";
		xmlShape +=	"<centre-of-bottom-base x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
  	xmlShape +=	"<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> " ;
  	xmlShape +=	"<radius val=\"0.1\" /> " ;
  	xmlShape +=	"<height val=\"3\" /> " ;
  	xmlShape +=	"</cylinder>";
		xmlShape +=	"<algebra val=\"shape\" /> "; 
		xmlShape +=	"<algebra val=\"shape\" /> "; 

		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape);

    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,10)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.05,1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.15,1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.01,0.01,1)) );
	}

	void testInfiniteCylinder()
	{
		//algebra line is essential
		std::string xmlShape = "<infinite-cylinder id=\"shape\"> ";
		xmlShape +=	"<centre x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
  	xmlShape +=	"<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> " ;
  	xmlShape +=	"<radius val=\"0.1\" /> " ;
  	xmlShape +=	"</infinite-cylinder>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape);

    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.0,1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.0,10)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.05,1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.15,1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.01,0.01,1)) );
	}

	void testCone()
	{
		//algebra line is essential
		std::string xmlShape = "<cone id=\"shape\"> ";
		xmlShape +=	"<tip-point x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
  	xmlShape +=	"<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> " ;
  	xmlShape +=	"<angle val=\"8.1\" /> " ;
  	xmlShape +=	"<height val=\"4\" /> " ;
  	xmlShape +=	"</cone>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape);

    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.0,-1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.001,1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.001,-1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.01,0.01,-1)) );
	}

	void testConeUseDirectStringArgument()
	{
		//algebra line is essential
		std::string xmlShape = "<cone id=\"shape\"> ";
		xmlShape +=	"<tip-point x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
  	xmlShape +=	"<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> " ;
  	xmlShape +=	"<angle val=\"8.1\" /> " ;
  	xmlShape +=	"<height val=\"4\" /> " ;
  	xmlShape +=	"</cone>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		ShapeFactory sFactory;
		boost::shared_ptr<Object> shape_sptr = sFactory.createShape(xmlShape);

    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.0,-1)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.001,1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.001,-1)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.01,0.01,-1)) );
	}

  void testComplement()
  {
    std::string xmlShape = "<cylinder id=\"stick\"> ";
    xmlShape +=	"<centre-of-bottom-base x=\"-0.5\" y=\"0.0\" z=\"0.0\" />";
    xmlShape +=	"<axis x=\"1.0\" y=\"0.0\" z=\"0.0\" />"; 
    xmlShape +=	"<radius val=\"0.05\" />";
    xmlShape +=	"<height val=\"1.0\" />";
    xmlShape +=	"</cylinder>";
    xmlShape +=	"<sphere id=\"some-sphere\">";
    xmlShape +=	"<centre x=\"0.0\"  y=\"0.0\" z=\"0.0\" />";
    xmlShape +=	"<radius val=\"0.5\" />";
    xmlShape +=	"</sphere>";     
    xmlShape +=	"<algebra val=\"some-sphere # stick\" />";    

    boost::shared_ptr<Object> shape_sptr = getObject(xmlShape);

    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,0.0)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,-0.04)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.0,-0.06)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.04,0.0)) );
    TS_ASSERT( shape_sptr->isValid(V3D(0.0,0.06,0.0)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.06,0.0,0.0)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.51,0.0,0.0)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.51,0.0)) );
    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,0.51)) );
  }

	void testNoneExistingShape()
	{
		//algebra line is essential
		std::string xmlShape = "<c5one id=\"shape\"> ";
		xmlShape +=	"<tip-point x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
  	xmlShape +=	"<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> " ;
  	xmlShape +=	"<angle val=\"8.1\" /> " ;
  	xmlShape +=	"<height val=\"4\" /> " ;
  	xmlShape +=	"</c5one>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape); // should return empty object

    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,1)) ); 
	}

	void testTypingErrorInSubElement()
	{
		//algebra line is essential
		std::string xmlShape = "<cone id=\"shape\"> ";
		xmlShape +=	"<tip-point x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
  	xmlShape +=	"<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> " ;
  	xmlShape +=	"<angle val=\"8.1\" /> " ;
  	xmlShape +=	"<heeight val=\"4\" /> " ;
  	xmlShape +=	"</cone>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape); // should return empty object

    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,1)) ); 
	}

  void testTypingErrorInAttribute()
	{
		//algebra line is essential
		std::string xmlShape = "<cone id=\"shape\"> ";
		xmlShape +=	"<tip-point x=\"0.0\" y=\"0.0\" z=\"0.0\" /> " ; 
  	xmlShape +=	"<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> " ;
  	xmlShape +=	"<angle val=\"8.1\" /> " ;
  	xmlShape +=	"<height vaal=\"4\" /> " ;
  	xmlShape +=	"</cone>";
		xmlShape +=	"<algebra val=\"shape\" /> ";  

		boost::shared_ptr<Object> shape_sptr = getObject(xmlShape); // should return empty object

    TS_ASSERT( !shape_sptr->isValid(V3D(0.0,0.0,1)) ); 
	}


	boost::shared_ptr<Object> getObject(std::string xmlShape)
  {
		std::string shapeXML = "<type name=\"userShape\"> " + xmlShape + " </type>";

	  // Set up the DOM parser and parse xml string
		DOMParser pParser;
		Document* pDoc;

  	pDoc = pParser.parseString(shapeXML);

		// Get pointer to root element
		Element* pRootElem = pDoc->documentElement();

		//convert into a Geometry object
		ShapeFactory sFactory;
		boost::shared_ptr<Object> shape_sptr = sFactory.createShape(pRootElem);
		pDoc->release();
    return shape_sptr;
  }



private:
  std::string inputFile;

};

#endif /*SHAPEFACTORYTEST_H_*/
