#ifndef MANTID_DATAOBJECTS_MDBOXBASETEST_H_
#define MANTID_DATAOBJECTS_MDBOXBASETEST_H_

#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidDataObjects/MDBoxBase.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/File.h>
#include "MantidAPI/CoordTransform.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using Mantid::Kernel::ConfigService;

/** Tester class that implements the minimum MDBoxBase to
 * allow testing
 */
TMDE_CLASS
class MDBoxBaseTester : public MDBoxBase<MDE,nd>
{
public:
  MDBoxBaseTester()
  : MDBoxBase<MDE,nd>()
  {     
  }
  virtual ~MDBoxBaseTester(){}
  MDBoxBaseTester(uint64_t /*filePos*/)
  : MDBoxBase<MDE,nd>()
  { 
  }
  MDBoxBaseTester(const MDBoxBaseTester  &source)
  : MDBoxBase<MDE,nd>(source,source.getBoxController())
  {
  }

  MDBoxBaseTester(const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > & extentsVector)
  : MDBoxBase<MDE,nd>(NULL,0,0,extentsVector)
  { 
  }
  //-----------------------------------------------------------------------------------------------
  Kernel::ISaveable * getISaveable(){return NULL;}
  Kernel::ISaveable * getISaveable()const{return NULL;}
  void setFileBacked(const uint64_t /*fileLocation*/,const size_t /*fileSize*/, const bool /*markSaved*/){};
  void clearFileBacked(bool /* loadData*/){/**does nothing*/};
  void setFileBacked(){};
  void saveAt(API::IBoxControllerIO *const /* */,  uint64_t /*position*/)const{/*Not saveable */};
  void loadAndAddFrom(API::IBoxControllerIO *const /* */, uint64_t /*position*/, size_t /* Size */){};
  void reserveMemoryForLoad(uint64_t /* Size */){};
  // regardless of what is actually instantiated, base tester would call itself gridbox
  bool isBox()const{return false;}

  /// Clear all contained data
  virtual void clear()
  {}

  virtual uint64_t getNPoints()const
  {
      return 0;
   // return this->getFileSize();
  }
    virtual size_t getDataInMemorySize()const
    {return 0;}
   /// @return the amount of memory that the object takes up in the MRU.
    virtual uint64_t getTotalDataSize() const
    {return 0;}

  /// Get number of dimensions
  virtual size_t getNumDims() const
  {return nd;}

  /// Get the total # of unsplit MDBoxes contained.
  virtual size_t getNumMDBoxes() const
  {return 0;}

  virtual size_t getNumChildren() const
  {return 0;}

  MDBoxBase<MDE,nd> * getChild(size_t /*index*/)
  { throw std::runtime_error("MDBox does not have children."); }

  /// Sets the children from a vector of children
  void setChildren(const std::vector<API::IMDNode *> & /*boxes*/, const size_t /*indexStart*/, const size_t /*indexEnd*/)
  { throw std::runtime_error("MDBox cannot have children."); }

  /// Return a copy of contained events
  virtual std::vector< MDE > * getEventsCopy()
  {return NULL;}

  /// Add a single event
  virtual void addEvent(const MDE & /*point*/)
  {}
  /// Add a single event and trace it if the box it has been added may need splitting 
  virtual void addAndTraceEvent(const MDE & /*point*/,size_t /*index */)
  {}


  /// Add a single event
  virtual void addEventUnsafe(const MDE & /*point*/)
  {}
  virtual size_t buildAndAddEvents(const std::vector<signal_t> & /*sigErrSq*/,const  std::vector<coord_t> & /*Coord*/,const std::vector<uint16_t> & /*runIndex*/,const std::vector<uint32_t> & /*detectorId*/)
  {return 0;}
  virtual void buildAndAddEvent(const Mantid::signal_t,const Mantid::signal_t,const std::vector<coord_t> &,uint16_t,uint32_t)
  {};
  virtual void buildAndTraceEvent(const Mantid::signal_t,const Mantid::signal_t,const std::vector<coord_t> &,uint16_t,uint32_t,size_t)
  {};
  virtual void buildAndAddEventUnsafe(const Mantid::signal_t,const Mantid::signal_t,const std::vector<coord_t> &,uint16_t,uint32_t)
  {};



  /** Perform centerpoint binning of events
   * @param bin :: MDBin object giving the limits of events to accept.
   */
  virtual void centerpointBin(MDBin<MDE,nd> & /*bin*/, bool * ) const
  {}
  virtual void splitAllIfNeeded(Mantid::Kernel::ThreadScheduler * /*ts*/ = NULL){}; 
  virtual void refreshCache(Kernel::ThreadScheduler * /*ts*/ = NULL){};
  //virtual void refreshCentroid(Kernel::ThreadScheduler * /*ts*/ = NULL){};
  virtual void calculateCentroid(coord_t * /*centroid*/) const{};
  virtual coord_t * getCentroid() const{return 0;};
  virtual void integrateSphere(Mantid::API::CoordTransform & /*radiusTransform*/, const coord_t /*radiusSquared*/, signal_t & /*signal*/, signal_t & /*errorSquared*/) const {};
  virtual void centroidSphere(Mantid::API::CoordTransform & /*radiusTransform*/, const coord_t /*radiusSquared*/, coord_t *, signal_t & ) const {};
  virtual void integrateCylinder(Mantid::API::CoordTransform & /*radiusTransform*/, const coord_t /*radius*/,const coord_t /*length*/, signal_t & /*signal*/, signal_t & /*errorSquared*/, std::vector<signal_t> & /*signal_fit*/) const {};
  virtual void getBoxes(std::vector<API::IMDNode *>&  /*boxes*/, size_t /*maxDepth*/, bool) {};
  virtual void getBoxes(std::vector<API::IMDNode *>&  /*boxes*/, size_t /*maxDepth*/, bool, Mantid::Geometry::MDImplicitFunction *) {};

  virtual void generalBin(MDBin<MDE,nd> & /*bin*/, Mantid::Geometry::MDImplicitFunction & /*function*/) const {}
  virtual void clearDataFromMemory(){};

  virtual bool getIsMasked() const
  {
    throw std::runtime_error("MDBoxBaseTester does not implement getIsMasked");
  }

  virtual void mask()
  {
    throw std::runtime_error("MDBoxBaseTester does not implement mask");
  }

  virtual void unmask()
  {
    throw std::runtime_error("MDBoxBaseTester does not implement unmask");
  }

};


class MDBoxBaseTest : public CxxTest::TestSuite
{
public:

  void test_default_constructor()
  {
    MDBoxBaseTester<MDLeanEvent<3>,3> box;
    TS_ASSERT_EQUALS( box.getSignal(), 0.0);
    TS_ASSERT_EQUALS( box.getErrorSquared(), 0.0);
  }

  void test_extents_constructor()
  {
    typedef MDBoxBaseTester<MDLeanEvent<3>,3> ibox3;
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > extentsVector;
    TS_ASSERT_THROWS_ANYTHING( ibox3 box(extentsVector) );
    extentsVector.resize(3);
    for (size_t d=0; d<3; d++)
    {
      extentsVector[d].setExtents(static_cast<double>(d) + 0.1,static_cast<double>(d + 1));

    }
    MDBoxBaseTester<MDLeanEvent<3>,3> box(extentsVector);
    TS_ASSERT_DELTA( box.getExtents(0).getMin(), 0.1, 1e-4 );
    TS_ASSERT_DELTA( box.getExtents(0).getMax(), 1.0, 1e-4 );
    TS_ASSERT_DELTA( box.getExtents(1).getMin(), 1.1, 1e-4 );
    TS_ASSERT_DELTA( box.getExtents(1).getMax(), 2.0, 1e-4 );
    TS_ASSERT_DELTA( box.getExtents(2).getMin(), 2.1, 1e-4 );
    TS_ASSERT_DELTA( box.getExtents(2).getMax(), 3.0, 1e-4 );
  }

  void test_transformDimensions()
  {
    typedef MDBoxBaseTester<MDLeanEvent<2>,2> ibox3;
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > extentsVector;
    TS_ASSERT_THROWS_ANYTHING( ibox3 box(extentsVector) );
    extentsVector.resize(2);
    for (size_t d=0; d<2; d++)
    {
      extentsVector[d].setExtents(1,2);
    }
    MDBoxBaseTester<MDLeanEvent<2>,2> box(extentsVector);
    // Now transform
    std::vector<double> scaling(2, 3.0);
    std::vector<double> offset(2, 1.0);
    box.transformDimensions(scaling, offset);
    for (size_t d=0; d<2; d++)
    {
      TS_ASSERT_DELTA( box.getExtents(d).getMin(), 4.0, 1e-4 );
      TS_ASSERT_DELTA( box.getExtents(d).getMax(), 7.0, 1e-4 );
    }
    TS_ASSERT_DELTA( box.getVolume(), 9.0, 1e-4);

  }

  void test_get_and_set_signal()
  {
    MDBoxBaseTester<MDLeanEvent<3>,3> box;
    TS_ASSERT_EQUALS( box.getSignal(), 0.0);
    TS_ASSERT_EQUALS( box.getErrorSquared(), 0.0);
    box.setSignal(123.0);
    box.setErrorSquared(456.0);
    TS_ASSERT_EQUALS( box.getSignal(), 123.0);
    TS_ASSERT_EQUALS( box.getErrorSquared(), 456.0);
    TS_ASSERT_DELTA( box.getError(), sqrt(456.0), 1e-4);
  }

  void test_getTotalWeight()
  {
    MDBoxBaseTester<MDLeanEvent<3>,3> box;
    TS_ASSERT_EQUALS( box.getTotalWeight(), 0.0);
    box.setTotalWeight(123.0);
    TS_ASSERT_EQUALS( box.getTotalWeight(), 123.0);
  }

  void test_get_and_set_depth()
  {
    MDBoxBaseTester<MDLeanEvent<3>,3> b;
    b.setDepth(123);
    TS_ASSERT_EQUALS( b.getDepth(), 123);
  }

  void test_getBoxAtCoord()
  {
    coord_t dummy[3] = {1,2,3};
    MDBoxBaseTester<MDLeanEvent<3>,3> b;
    TSM_ASSERT_EQUALS("MDBoxBase->getBoxAtCoord() always returns this.", b.getBoxAtCoord(dummy), &b);
  }

  void test_getParent_and_setParent()
  {
    MDBoxBaseTester<MDLeanEvent<3>,3> b;
    TSM_ASSERT( "Default parent is NULL", !b.getParent() );
    MDBoxBaseTester<MDLeanEvent<3>,3> * daddy = new MDBoxBaseTester<MDLeanEvent<3>,3>;
    b.setParent(daddy);
    TS_ASSERT_EQUALS( b.getParent(), daddy);
    // Copy ctor
    MDBoxBaseTester<MDLeanEvent<3>,3> c(b);
    TS_ASSERT_EQUALS( c.getParent(), daddy);

  }

  /** Setting and getting the extents;
   * also, getting the center */
  void test_setExtents()
  {
    MDBoxBaseTester<MDLeanEvent<2>,2> b;
    b.setExtents(0, -8.0, 10.0);
    TS_ASSERT_DELTA(b.getExtents(0).getMin(), -8.0, 1e-6);
    TS_ASSERT_DELTA(b.getExtents(0).getMax(), +10.0, 1e-6);

    b.setExtents(1, -4.0, 12.0);
    TS_ASSERT_DELTA(b.getExtents(1).getMin(), -4.0, 1e-6);
    TS_ASSERT_DELTA(b.getExtents(1).getMax(), +12.0, 1e-6);

    TS_ASSERT_THROWS( b.setExtents(2, 0, 1.0), std::invalid_argument);

    coord_t center[2];
    b.getCenter(center);
    TS_ASSERT_DELTA( center[0], +1.0, 1e-6);
    TS_ASSERT_DELTA( center[1], +4.0, 1e-6);
  }

  void test_copy_constructor()
  {
    MDBoxBaseTester<MDLeanEvent<2>,2> b;
    b.setDepth(6);
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    b.setSignal(123.0);
    b.setErrorSquared(456.0);
    b.setID(8765);
    b.calcVolume();

    // Perform the copy
    MDBoxBaseTester<MDLeanEvent<2>,2> box(b);
    TS_ASSERT_DELTA(box.getExtents(0).getMin(), -10.0, 1e-6);
    TS_ASSERT_DELTA(box.getExtents(0).getMax(), +10.0, 1e-6);
    TS_ASSERT_DELTA(box.getExtents(1).getMin(), -4.0, 1e-6);
    TS_ASSERT_DELTA(box.getExtents(1).getMax(), +6.0, 1e-6);
    TS_ASSERT_DELTA( box.getSignal(), b.getSignal(), 1e-6);
    TS_ASSERT_DELTA( box.getErrorSquared(), b.getErrorSquared(), 1e-6);
    TS_ASSERT_DELTA( box.getInverseVolume(), b.getInverseVolume(), 1e-6);
    TS_ASSERT_EQUALS( box.getID(), b.getID());
    TS_ASSERT_EQUALS( box.getDepth(), b.getDepth());
  }



  /** Calculating volume and normalizing signal by it. */
  void test_calcVolume()
  {
    MDBoxBaseTester<MDLeanEvent<2>,2> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    b.calcVolume();
    TS_ASSERT_DELTA( b.getVolume(), 200.0, 1e-5);
    TS_ASSERT_DELTA( b.getInverseVolume(), 1.0/200.0, 1e-5);

    b.setSignal(100.0);
    b.setErrorSquared(300.0);

    TS_ASSERT_DELTA( b.getSignal(), 100.0, 1e-5);
    TS_ASSERT_DELTA( b.getSignalNormalized(), 0.5, 1e-5);
    TS_ASSERT_DELTA( b.getErrorSquared(), 300.0, 1e-5);
    TS_ASSERT_DELTA( b.getErrorSquaredNormalized(), 1.5, 1e-5);
  }



  /** Get vertexes using the extents */
  void test_getVertexes()
  {
    MDBoxBaseTester<MDLeanEvent<2>,2> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    std::vector<Mantid::Kernel::VMD> v = b.getVertexes();
    TS_ASSERT_EQUALS( v[0][0], -10.0);
    TS_ASSERT_EQUALS( v[0][1], -4.0);
    TS_ASSERT_EQUALS( v[1][0], 10.0);
    TS_ASSERT_EQUALS( v[1][1], -4.0);
    TS_ASSERT_EQUALS( v[2][0], -10.0);
    TS_ASSERT_EQUALS( v[2][1], 6.0);
    TS_ASSERT_EQUALS( v[3][0], 10.0);
    TS_ASSERT_EQUALS( v[3][1], 6.0);
  }

  /** Get vertexes as a bare array */
  void test_getVertexesArray()
  {
    MDBoxBaseTester<MDLeanEvent<2>,2> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    size_t numVertexes = 0;
    coord_t * v = b.getVertexesArray(numVertexes);
    TS_ASSERT_EQUALS( numVertexes, 4);
    TS_ASSERT_EQUALS( v[0], -10.0);
    TS_ASSERT_EQUALS( v[0+1], -4.0);
    TS_ASSERT_EQUALS( v[2], 10.0);
    TS_ASSERT_EQUALS( v[2+1], -4.0);
    TS_ASSERT_EQUALS( v[4], -10.0);
    TS_ASSERT_EQUALS( v[4+1], 6.0);
    TS_ASSERT_EQUALS( v[6], 10.0);
    TS_ASSERT_EQUALS( v[6+1], 6.0);
    delete [] v;
  }

  /** Get vertexes as a bare array,
   * projecting down into fewer dimensions */
  void test_getVertexesArray_reducedDimension()
  {
    MDBoxBaseTester<MDLeanEvent<2>,2> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    size_t numVertexes = 0;
    coord_t * v;

    bool maskDim[2] = {true, false};
    v = b.getVertexesArray(numVertexes, 1, maskDim);
    TS_ASSERT_EQUALS( numVertexes, 2);
    TS_ASSERT_EQUALS( v[0], -10.0);
    TS_ASSERT_EQUALS( v[1], 10.0);
    delete [] v;

    bool maskDim2[2] = {false, true};
    v = b.getVertexesArray(numVertexes, 1, maskDim2);
    TS_ASSERT_EQUALS( numVertexes, 2);
    TS_ASSERT_EQUALS( v[0], -4.0);
    TS_ASSERT_EQUALS( v[1], 6.0);
    delete [] v;
  }

  /** Get vertexes as a bare array,
   * projecting down into fewer dimensions */
  void test_getVertexesArray_reducedDimension_3D()
  {
    MDBoxBaseTester<MDLeanEvent<3>,3> b;
    b.setExtents(0, -10.0, 10.0);
    b.setExtents(1, -4.0, 6.0);
    b.setExtents(2, -2.0, 8.0);
    size_t numVertexes = 0;
    coord_t * v;

    // 3D projected down to 2D in X/Y
    bool maskDim[3] = {true, true, false};
    v = b.getVertexesArray(numVertexes, 2, maskDim);
    TS_ASSERT_EQUALS( numVertexes, 4);
    TS_ASSERT_EQUALS( v[0], -10.0);
    TS_ASSERT_EQUALS( v[0+1], -4.0);
    TS_ASSERT_EQUALS( v[2], 10.0);
    TS_ASSERT_EQUALS( v[2+1], -4.0);
    TS_ASSERT_EQUALS( v[4], -10.0);
    TS_ASSERT_EQUALS( v[4+1], 6.0);
    TS_ASSERT_EQUALS( v[6], 10.0);
    TS_ASSERT_EQUALS( v[6+1], 6.0);
    delete [] v;

    // Can't give 0 dimensions.
    TS_ASSERT_THROWS_ANYTHING( v = b.getVertexesArray(numVertexes, 0, maskDim) );

    // 3D projected down to 1D in Y
    bool maskDim2[3] = {false, true, false};
    v = b.getVertexesArray(numVertexes, 1, maskDim2);
    TS_ASSERT_EQUALS( numVertexes, 2);
    TS_ASSERT_EQUALS( v[0], -4.0);
    TS_ASSERT_EQUALS( v[1], 6.0);
    delete [] v;

    // 3D projected down to 2D in Y/Z
    bool maskDim3[3] = {false, true, true};
    v = b.getVertexesArray(numVertexes, 2, maskDim3);
    TS_ASSERT_EQUALS( numVertexes, 4);
    TS_ASSERT_EQUALS( v[0], -4.0);
    TS_ASSERT_EQUALS( v[0+1], -2.0);
    TS_ASSERT_EQUALS( v[2], 6.0);
    TS_ASSERT_EQUALS( v[2+1], -2.0);
    TS_ASSERT_EQUALS( v[4], -4.0);
    TS_ASSERT_EQUALS( v[4+1], 8.0);
    TS_ASSERT_EQUALS( v[6], 6.0);
    TS_ASSERT_EQUALS( v[6+1], 8.0);
    delete [] v;
  }

  void test_sortBoxesByFilePos()
  {
    std::vector<API::IMDNode *> boxes;
    // 10 to 1 in reverse order

    for (uint64_t i=0; i<10; i++)
    {
      boxes.push_back(new MDBoxBaseTester<MDLeanEvent<1>,1>(10-i));
    }
    //TODO:
    //Kernel::ISaveable::sortObjByFilePos(boxes);
    //// After sorting, they are in the right order 1,2,3, etc.
    //for (uint64_t i=0; i<10; i++)
    //{
    //    TS_ASSERT_EQUALS( boxes[i]->getFilePosition(), i+1); 
    //    delete boxes[i];
    //}
  }


};



//======================================================================
//======================================================================
//======================================================================
class MDBoxBaseTestPerformance : public CxxTest::TestSuite
{
public:

  /** Vector of VMD version of getVertexes (slower than the bare array version)
   * (this is only 100 thousand, not a million times like the others).
   */
  void test_getVertexes_3D()
  {
    MDBoxBaseTester<MDLeanEvent<3>,3> b;
    b.setExtents(0, -9.0, 9.0);
    b.setExtents(1, -8.0, 8.0);
    b.setExtents(2, -7.0, 7.0);
    for (size_t i=0; i<100000; i++)
    {
      std::vector<Mantid::Kernel::VMD> v = b.getVertexes();
    }
  }

  void test_getVertexesArray_3D()
  {
    MDBoxBaseTester<MDLeanEvent<3>,3> b;
    b.setExtents(0, -9.0, 9.0);
    b.setExtents(1, -8.0, 8.0);
    b.setExtents(2, -7.0, 7.0);
    for (size_t i=0; i<1000000; i++)
    {
      size_t numVertexes;
      coord_t * v = b.getVertexesArray(numVertexes);
      delete [] v;
    }
  }

  void test_getVertexesArray_3D_projected_to_2D()
  {
    MDBoxBaseTester<MDLeanEvent<3>,3> b;
    b.setExtents(0, -9.0, 9.0);
    b.setExtents(1, -8.0, 8.0);
    b.setExtents(2, -7.0, 7.0);
    bool maskDim[3] = {true, true, false};
    for (size_t i=0; i<1000000; i++)
    {
      size_t numVertexes;
      coord_t * v = b.getVertexesArray(numVertexes, 2, maskDim);
      delete [] v;
    }
  }

  void test_getVertexesArray_4D()
  {
    MDBoxBaseTester<MDLeanEvent<4>,4> b;
    b.setExtents(0, -9.0, 9.0);
    b.setExtents(1, -8.0, 8.0);
    b.setExtents(2, -7.0, 7.0);
    b.setExtents(3, -6.0, 6.0);
    for (size_t i=0; i<1000000; i++)
    {
      size_t numVertexes;
      coord_t * v = b.getVertexesArray(numVertexes);
      delete [] v;
    }
  }
  void test_getVertexesArray_4D_projected_to_3D()
  {
    MDBoxBaseTester<MDLeanEvent<4>,4> b;
    bool maskDim[4] = {true, true, true, false};
    b.setExtents(0, -9.0, 9.0);
    b.setExtents(1, -8.0, 8.0);
    b.setExtents(2, -7.0, 7.0);
    b.setExtents(3, -6.0, 6.0);
    for (size_t i=0; i<1000000; i++)
    {
      size_t numVertexes;
      coord_t * v = b.getVertexesArray(numVertexes, 3, maskDim);
      delete [] v;
    }
  }


};


#endif /* MANTID_DATAOBJECTS_MDBOXBASETEST_H_ */

