#ifndef _TEST_MD_GEOMETRY_H
#define _TEST_MD_GEOMETRY_H

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"

#include "boost/scoped_ptr.hpp"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"


using namespace Mantid::Geometry;
class testMDGeometry: public MDGeometry
{
public:
	testMDGeometry(const MDGeometryBasis &basis):
	  MDGeometry(basis){};

	  boost::shared_ptr<MDDimension> getDimension(unsigned int num){ return MDGeometry::getDimension(num);  }
	  boost::shared_ptr<MDDimension> getDimension(const std::string &ID, bool doThrow=true){ return MDGeometry::getDimension(ID,doThrow);  }
};


class MDGeometryTest : public CxxTest::TestSuite
{
  // helper method to construct a near-complete geometry.
  static MDGeometry* constructGeometry()
  {
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("q1", true, 1));
    basisDimensions.insert(MDBasisDimension("q2", true, 2));
    basisDimensions.insert(MDBasisDimension("q3", true, 3));
    basisDimensions.insert(MDBasisDimension("p", false, 0));
    basisDimensions.insert(MDBasisDimension("T", false, 4));
    UnitCell cell;
    MDGeometryBasis basis(basisDimensions, cell);

    //Dimensions generated, but have default values for bins and extents.
    std::vector<boost::shared_ptr<IMDDimension> > dimensions; 
    boost::shared_ptr<IMDDimension> dimX = boost::shared_ptr<IMDDimension>(new MDDimension("q1"));
    boost::shared_ptr<IMDDimension> dimY = boost::shared_ptr<IMDDimension>(new MDDimension("q2"));
    boost::shared_ptr<IMDDimension> dimZ = boost::shared_ptr<IMDDimension>(new MDDimension("q3"));
    boost::shared_ptr<IMDDimension> dimt = boost::shared_ptr<IMDDimension>(new MDDimension("p"));
    boost::shared_ptr<IMDDimension> dimTemp = boost::shared_ptr<IMDDimension>(new MDDimension("T"));

    dimensions.push_back(dimX);
    dimensions.push_back(dimY);
    dimensions.push_back(dimZ);
    dimensions.push_back(dimt);
    dimensions.push_back(dimTemp);
    RotationMatrix rotationMatrix(9,0);
    rotationMatrix[0] = rotationMatrix[4] = rotationMatrix[8] = 1;
    MDGeometryDescription description(dimensions, dimX, dimY, dimZ, dimTemp, rotationMatrix);

    //Create a geometry.
    return new MDGeometry(basis, description);

  }

  std::auto_ptr<testMDGeometry> tDND_geometry;
  std::auto_ptr<MDGeometryDescription> pSlice;
public:

  MDGeometryTest()
  {
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("qx", true, 0));
    basisDimensions.insert(MDBasisDimension("p", false, 3));
    basisDimensions.insert(MDBasisDimension("qy", true, 1));
    basisDimensions.insert(MDBasisDimension("qz", true, 2));


    UnitCell cell;
    TSM_ASSERT_THROWS_NOTHING("Valid MD geometry constructor should not throw",tDND_geometry= std::auto_ptr<testMDGeometry>(new testMDGeometry(MDGeometryBasis(basisDimensions, cell))));
    
	TSM_ASSERT_EQUALS("Empty geometry initiated by MDBasis only should be size 0",0,tDND_geometry->getGeometryExtend());

  }
 

  void testMDGeometryDimAccessors(void){
    TS_ASSERT_THROWS_NOTHING(tDND_geometry->getXDimension());
    TS_ASSERT_THROWS_NOTHING(tDND_geometry->getYDimension());
    TS_ASSERT_THROWS_NOTHING(tDND_geometry->getZDimension());
    TS_ASSERT_THROWS_NOTHING(tDND_geometry->getTDimension());

  }
  void testMDGeomIntegrated(void){
    std::vector<boost::shared_ptr<IMDDimension> > Dims = tDND_geometry->getIntegratedDimensions();
    // default size of the dimensions is equal 4
    TS_ASSERT_EQUALS(Dims.size(),4);
  }
  void testMDGeomDimAcessors(void){
    // get pointer to the dimension 0
    boost::shared_ptr<MDDimension> pDim=tDND_geometry->getDimension(0);
    TS_ASSERT_EQUALS(pDim->getDimensionTag(),"qx");
    
    boost::shared_ptr<MDDimension> pDim0;
    // no such dimension
    TS_ASSERT_THROWS_ANYTHING(pDim0=tDND_geometry->getDimension(8));
    // no such dimension
    TS_ASSERT_THROWS_ANYTHING(pDim0=tDND_geometry->getDimension("u7"));
    //        TS_ASSERT_EQUALS(pDim0,NULL);

    // the same dimension as above
    TS_ASSERT_THROWS_NOTHING(pDim0=tDND_geometry->getDimension("qx"));
    TS_ASSERT_EQUALS(pDim0.get(),pDim.get());
  }
  void testSlicingProperty(void){
    pSlice = std::auto_ptr<MDGeometryDescription>(new MDGeometryDescription(*tDND_geometry));

    //       we want these data to be non-integrated;
	TS_ASSERT_THROWS_NOTHING(pSlice->pDimDescription("p")->nBins=100);
    // wrong tag
	TS_ASSERT_THROWS_ANYTHING(pSlice->pDimDescription("eh")->nBins=200);
    // right tag
	TS_ASSERT_THROWS_NOTHING(pSlice->pDimDescription("qx")->nBins=200);

    // we want first (0) axis to be energy 
    TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(0,"p"));
    TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(0,"p"));
    // and the third (2) ->el (z-axis) 
    TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(3,"qz"));
    TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(2,"qz"));

    TS_ASSERT_THROWS_NOTHING(pSlice->setPAxis(3,"qx"));

    std::vector<std::string> names = pSlice->getDimensionsTags();
    for(unsigned int i=0;i<names.size();i++){
		TS_ASSERT_EQUALS(names[i],pSlice->pDimDescription(i)->Tag);
		TS_ASSERT_EQUALS(names[i],pSlice->pDimDescription(i)->AxisName);
    }
	TSM_ASSERT_EQUALS("The slice describes grid of specific size: ",100*200,pSlice->getImageSize());
  }
  void testMDGeomSetFromSlice1(void){
   // pSlice describes 4x3 geometry with 200x100 dimensions expanded and others integrated;
    TS_ASSERT_THROWS_NOTHING(tDND_geometry->initialize(*pSlice));
    unsigned int i,ic;

	TSM_ASSERT_EQUALS("The geometry initialized by the slicing property above has to have specific extend",200*100,tDND_geometry->getGeometryExtend());
    boost::shared_ptr<MDDimension> pDim;

    std::vector<std::string> expanded_tags(tDND_geometry->getNumDims());

    // arrange dimensions tags like the dimensions are arranged in the geometry
    ic=0;
    TS_ASSERT_THROWS_NOTHING(
      for(i=0;i<expanded_tags.size();i++){
		  if(pSlice->pDimDescription(i)->nBins>1){  // non-integrated;
          expanded_tags[ic]=pSlice->pDimDescription(i)->Tag;
          ic++;
        }
      }
      for(i=0;i<expanded_tags.size();i++){
        if(pSlice->pDimDescription(i)->nBins<2){  // non-integrated;
          expanded_tags[ic]=pSlice->pDimDescription(i)->Tag;
          ic++;
        }
      }
      )

        for(i=0;i<tDND_geometry->getNumDims();i++){
          TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(i));
          TS_ASSERT_EQUALS(pDim->getDimensionTag(),expanded_tags[i]);
        }

        TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(0));
        TS_ASSERT_EQUALS(pDim->getStride(),1);

        TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(1));
        TS_ASSERT_EQUALS(pDim->getStride(),100);
        TS_ASSERT_EQUALS(pDim->getIntegrated(),false);

        TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(2));
        TS_ASSERT_EQUALS(pDim->getStride(),0);
        TS_ASSERT_EQUALS(pDim->getIntegrated(),true);
  }
 void testDimArrangementByBasis(){
     // here we check if the dimension returned in a way, as they are arranged in basis and MDDataPoints
     std::vector<boost::shared_ptr<IMDDimension> > psDims = tDND_geometry->getDimensions(true);
     std::vector<std::string> dimID(4);
     dimID[0]="qx";
     dimID[1]="qy";
     dimID[2]="qz";
     dimID[3]="p";
     for(unsigned int i=0; i<this->tDND_geometry->getNumDims();i++){
         TSM_ASSERT_EQUALS("The dimension in the geometry is not located properly",dimID[i],psDims[i]->getDimensionId());
     }
  }
 void testDimArrangementByGeometry(){
     // here we check if the dimension returned in a way, as they are arranged in MDGeometry
     std::vector<boost::shared_ptr<IMDDimension> > psDims = tDND_geometry->getDimensions();
     std::vector<std::string> dimID(4);
     dimID[0]="p";
     dimID[1]="qx";
     dimID[3]="qy";
     dimID[2]="qz";
     for(unsigned int i=0; i<this->tDND_geometry->getNumDims();i++){
         TSM_ASSERT_EQUALS("The dimension in the geometry is not located properly",dimID[i],psDims[i]->getDimensionId());
     }
  }
  void testGeometryFromSlice1Size(){
	  TSM_ASSERT_EQUALS("The size of the image, described by this geometry after resizing, differs from expected",tDND_geometry->getGeometryExtend(),100*200);
  }
  void testMDGeomSetFromSlice2(void){
    // this should be fully equivalent to 1

    // arrange final dimensions according to pAxis, this will run through one branch of initialize only
    TS_ASSERT_THROWS_NOTHING(tDND_geometry->initialize(*pSlice));

    boost::shared_ptr<MDDimension> pDim;


    TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(0));
    TS_ASSERT_EQUALS(pDim->getStride(),1);

    TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(1));
    TS_ASSERT_EQUALS(pDim->getStride(),100);
    TS_ASSERT_EQUALS(pDim->getIntegrated(),false);

    TS_ASSERT_THROWS_NOTHING(pDim = tDND_geometry->getDimension(2));
    TS_ASSERT_EQUALS(pDim->getStride(),0);
    TS_ASSERT_EQUALS(pDim->getIntegrated(),true);
  }

  void testgetNumDims()
  {
     TSM_ASSERT_EQUALS("The number of dimensions returned is not equal to the expected value.", 4, tDND_geometry->getNumDims());
  }
  /// returns the number of reciprocal dimensions
  void  testGetNumReciprocalDims()
  {
   
    TSM_ASSERT_EQUALS("The number of reciprocal dimensions returned is not equal to the expected value.", 3, tDND_geometry->getNumReciprocalDims());
  }

  void testGetNumExpandedDims()
  {
    TSM_ASSERT_EQUALS("The number of expanded dimensions returned is not equal to the expected value.", 2, tDND_geometry->getNumExpandedDims());
  }

  void testToXMLString()
  {

    boost::scoped_ptr<MDGeometry> geometry(constructGeometry());

    //Only practicle way to check the xml output in the absense of xsd is as part of a dom tree.
    Poco::XML::DOMParser pParser;
    std::string xmlToParse = geometry->toXMLString(); //Serialize the geometry.
    Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
    Poco::XML::Element* pRootElem = pDoc->documentElement();

    //Check that the number of dimensions provided is correct.
    TSM_ASSERT_EQUALS("Wrong number of dimension in geometry xml", 5, pRootElem->getElementsByTagName("Dimension")->length());

    //Check that mapping nodes have been provided.
    TSM_ASSERT_EQUALS("No DimensionX in geometry xml", 1, pRootElem->getElementsByTagName("XDimension")->length());
    TSM_ASSERT_EQUALS("No DimensionY in geometry xml", 1, pRootElem->getElementsByTagName("YDimension")->length());
    TSM_ASSERT_EQUALS("No DimensionZ in geometry xml", 1, pRootElem->getElementsByTagName("ZDimension")->length());
    TSM_ASSERT_EQUALS("No DimensionT in geometry xml", 1, pRootElem->getElementsByTagName("TDimension")->length());

    //Check that mapping nodes give correct mappings.
    Poco::XML::Element* dimensionSetElement = pRootElem;
    TSM_ASSERT_EQUALS("No DimensionX mapping is incorrect", "q1", dimensionSetElement->getChildElement("XDimension")->getChildElement("RefDimensionId")->innerText());
    TSM_ASSERT_EQUALS("No DimensionY mapping is incorrect", "q2", dimensionSetElement->getChildElement("YDimension")->getChildElement("RefDimensionId")->innerText());
    TSM_ASSERT_EQUALS("No DimensionZ mapping is incorrect", "q3", dimensionSetElement->getChildElement("ZDimension")->getChildElement("RefDimensionId")->innerText());
    TSM_ASSERT_EQUALS("No DimensionT mapping is incorrect", "T",  dimensionSetElement->getChildElement("TDimension")->getChildElement("RefDimensionId")->innerText());


  }

  ~MDGeometryTest()
  {
  }
};
#endif
