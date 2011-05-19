#ifndef _TEST_MD_GEOMETRY_H
#define _TEST_MD_GEOMETRY_H

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"

#include <boost/scoped_ptr.hpp>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <cfloat>


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
    basisDimensions.insert(MDBasisDimension("q1", true, 0));
    basisDimensions.insert(MDBasisDimension("q2", true, 1));
    basisDimensions.insert(MDBasisDimension("q3", true, 2));
    basisDimensions.insert(MDBasisDimension("p", false, 3));
    basisDimensions.insert(MDBasisDimension("T", false, 4));
	boost::shared_ptr<UnitCell> spCell = boost::shared_ptr<UnitCell>(new UnitCell(2.87,2.87,2.87));   
    MDGeometryBasis basis(basisDimensions,spCell);

    //Dimensions generated, but have default values for bins and extents.
    std::vector<boost::shared_ptr<IMDDimension> > dimensions;
    boost::shared_ptr<IMDDimension> dimX = boost::shared_ptr<MDDimension>(new MDDimensionRes("q1",q1));
    boost::shared_ptr<IMDDimension> dimY = boost::shared_ptr<MDDimension>(new MDDimensionRes("q2",q2));
    boost::shared_ptr<IMDDimension> dimZ = boost::shared_ptr<MDDimension>(new MDDimensionRes("q3",q3));
    boost::shared_ptr<IMDDimension> dimt = boost::shared_ptr<MDDimension>(new MDDimension("p"));
    boost::shared_ptr<IMDDimension> dimTemp = boost::shared_ptr<MDDimension>(new MDDimension("T"));

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

	boost::shared_ptr<UnitCell> spCell = boost::shared_ptr<UnitCell>(new UnitCell(2.87,2.87,2.87));  

    TSM_ASSERT_THROWS_NOTHING("Valid MD geometry constructor should not throw",tDND_geometry= std::auto_ptr<testMDGeometry>(new testMDGeometry(MDGeometryBasis(basisDimensions,spCell))));
    
	TSM_ASSERT_EQUALS("Empty geometry initiated by MDBasis only should be size 0",0,tDND_geometry->getGeometryExtend());

  }
void testMDGeometryUnitRotations(){
	MantidMat rotMat = tDND_geometry->getRotations();
	MantidMat uno(3,3,true);

	TSM_ASSERT_EQUALS("Natural rotation matrix for unmodified geomerty should be unit matrix",true,rotMat.equals(uno,FLT_EPSILON));
}

  void testMDGeometryDimAccessors(void){
    TS_ASSERT_THROWS_NOTHING(tDND_geometry->getXDimension());
    TS_ASSERT_THROWS_NOTHING(tDND_geometry->getYDimension());
    TS_ASSERT_THROWS_NOTHING(tDND_geometry->getZDimension());
    TS_ASSERT_THROWS_NOTHING(tDND_geometry->getTDimension());

  }
  void testGetDefaultDimDirections(void){
	  TSM_ASSERT_EQUALS("first  default direction should be along first  basis direction",true,V3D(1,0,0)==tDND_geometry->getXDimension()->getDirection());
	  TSM_ASSERT_EQUALS("second default direction should be along second basis direction",true,V3D(0,1,0)==tDND_geometry->getYDimension()->getDirection());
	  TSM_ASSERT_EQUALS("third  default direction should be along third  basis direction",true,V3D(0,0,1)==tDND_geometry->getZDimension()->getDirection());
	  TSM_ASSERT_EQUALS("fourth default direction should be 0",true,V3D(0,0,0)==tDND_geometry->getTDimension()->getDirection());
  }
  void testMDGeomIntegrated(void){
    std::vector<boost::shared_ptr<IMDDimension> > Dims = tDND_geometry->getIntegratedDimensions();
    // default size of the dimensions is equal 4
    TS_ASSERT_EQUALS(Dims.size(),4);
  }
  void testMDGeomDimAcessors(void){
    // get pointer to the dimension 0
    boost::shared_ptr<IMDDimension> pDim=tDND_geometry->getDimension(0);
   // TS_ASSERT_EQUALS(pDim->getDimensionTag(),"qx");
    
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
	// it is reciprocal dimension
	TSM_ASSERT_EQUALS("qx defined as reciprocal dimension",true,pSlice->pDimDescription("qx")->isReciprocal);
	TSM_ASSERT_EQUALS("p  defined as orthogonal dimension",false,pSlice->pDimDescription("p")->isReciprocal);
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
  void testSetSlicingRotations(){
	  // get access to geometry basis and derive new transformation matrix, which would transform data into new basis, defined
	  // by two vectors expressed in the units of the reciprocal lattice
	  MantidMat rot = tDND_geometry->get_constMDGeomBasis().get_constUnitCell().getUmatrix(V3D(1,1,0),V3D(1,-1,0));

	  TSM_ASSERT_THROWS_NOTHING("It is nothing to throw here",pSlice->setRotationMatrix(rot));
  }
  void testMDGeomSetFromSlice1(void){
   // pSlice describes 4x3 geometry with 200x100 dimensions expanded and others integrated; rotated by 45% around z axis;
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
  void testDimDirections(){
	  V3D dir1(1,1,0);
	  V3D dir2(1,-1,0);
	  dir1.normalize();
	  dir2.normalize();
	  TSM_ASSERT_EQUALS("qx difection should rougly coinside with (1,1,0)",true,dir1==tDND_geometry->getDimension("qx")->getDirection());
	  TSM_ASSERT_EQUALS("qy difection should rougly coinside with (1,-1,0)",true,dir2==tDND_geometry->getDimension("qy")->getDirection());
	  TSM_ASSERT_EQUALS("qz difection should go to z (0,0,-1)",true,V3D(0,0,-1)==tDND_geometry->getDimension("qz")->getDirection());
	  TSM_ASSERT_EQUALS("p difection should be 0    (0,0,0)",true,V3D(0,0,0)==tDND_geometry->getDimension("p")->getDirection());
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
  void testReducedBasisRotations(){
	  // build default geometrh
	  std::auto_ptr<MDGeometry>  pGeom = std::auto_ptr<MDGeometry>(constructGeometry());
	  // and default description for this geometry
	  std::auto_ptr<MDGeometryDescription> pDescr = std::auto_ptr<MDGeometryDescription>(new MDGeometryDescription(*pGeom));
	  // set the geometry description
	  pDescr->pDimDescription("q1")->nBins=200;
	  pDescr->pDimDescription("p")->nBins =200;
      pDescr->setPAxis(0,"p");
	  pDescr->setPAxis(1,"q1");
	  pDescr->setPAxis(2,"q3");

	  pGeom->initialize(*pDescr);
	  // this new geomerty should have left-hand rotation matrix;
	  std::vector<double> rm = pGeom->getRotations().get_vector();
	  std::vector<double> sample(9,0);
	  sample[0]=1;
	  sample[5]=-1;
	  sample[7]=1;
	  double err(0);
	  for(int i=0;i<9;i++){
		  err+=fabs(rm[i]-sample[i]);
	  }
	  TSM_ASSERT_DELTA("samples should coinside with mathematical presision",0,err,FLT_EPSILON);



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

  ~MDGeometryTest()
  {
  }
};
#endif
