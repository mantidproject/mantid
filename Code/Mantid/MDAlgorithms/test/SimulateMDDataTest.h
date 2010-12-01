#ifndef SIMULATEMDDATATEST_H_
#define SIMULATEMDDATATEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <iostream>
#include <boost/scoped_ptr.hpp> 
#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/MDGeometry/MDPoint.h"
#include "MantidGeometry/MDGeometry/MDCell.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidMDAlgorithms/SimulateMDD.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;


// Add a concrete IMDDimension class
namespace Mantid
{
  namespace Geometry
  {
    class DLLExport TestIMDDimension : public IMDDimension
    {
    public:
      virtual std::string getName() const { return("TestX"); };
      virtual std::string getDimensionId() const { return("TestX"); };
      virtual bool getIsIntegrated() const {return(0);};
      virtual double getMaximum() const {return(1.0);};
      virtual double getMinimum() const {return(0.0);};
      virtual unsigned int getNBins() const {return(2);};
      TestIMDDimension() {};
      ~TestIMDDimension() {};
    };
  }
}

// Test Cut data
class DLLExport TestCut : public IMDWorkspace
{
private:
   int m_points;
   int m_cells;
   //std::vector<boost::shared_ptr<Mantid::Geometry::MDCell>> m_mdcells;
   std::vector< Mantid::Geometry::MDCell * > m_mdcells;

public:

      virtual Mantid::Geometry::IMDDimension * getXDimensionImp() const {return new Mantid::Geometry::TestIMDDimension() ;};
      virtual Mantid::Geometry::IMDDimension * getYDimensionImp() const
              {throw std::runtime_error("Not implemented");};
      virtual Mantid::Geometry::IMDDimension * getZDimensionImp() const
              {throw std::runtime_error("Not implemented");};
      virtual Mantid::Geometry::IMDDimension * gettDimensionImp() const
              {throw std::runtime_error("Not implemented");};
      virtual int getNPoints() const {return m_points;};
      virtual Mantid::Geometry::MDPoint * getPointImp(int index) const
              {throw std::runtime_error("Not implemented");};
      virtual Mantid::Geometry::MDCell * getCellImp(int dim1Increment) const
      {
          if (dim1Increment<0 || dim1Increment >= m_mdcells.size())
          {
              throw std::range_error("TestCut::getCell, increment out of range");
          }
          return(m_mdcells.at(dim1Increment));
          
      };
      virtual Mantid::Geometry::MDCell * getCellImp(int dim1Increment, int dim2Increment) const {return 0;};
      virtual Mantid::Geometry::MDCell * getCellImp(int dim1Increment, int dim2Increment, int dim3Increment) const {return 0;};
      virtual Mantid::Geometry::MDCell * getCellImp(int dim1Increment, int dim2Increment, int dim3Increment, int dim4Increment) const {return 0;};
      virtual Mantid::Geometry::MDCell * getCellImp(...) const {return 0;};

      virtual Mantid::Geometry::IMDDimension * getDimensionImp(std::string id) const
      {
         throw std::runtime_error("Not implemented");
      }

      /// return ID specifying the workspace kind
      virtual const std::string id() const {return "TestIMDDWorkspace";}
      /// return number of dimensions in MD workspace
      virtual unsigned int getNumDims()const{return 4;}
      /// Get the footprint in memory in KB - return 0 for now
      virtual long int getMemorySize() const {return 0;};

   TestCut()
   {
      m_points=0;
      m_cells=0;
   }
   TestCut(std::vector<Mantid::Geometry::MDCell * > pContribCells ) :
           m_mdcells(pContribCells)
   {
      m_cells=pContribCells.size();
      m_points=0;
   }
   ~TestCut() {};
};

class testSimulateMDD : public CxxTest::TestSuite
{
private:
  boost::shared_ptr<TestCut> myCut;
  boost::shared_ptr<TestCut> outCut;
  std::vector<Mantid::Geometry::MDCell *> pContribCells;
  std::string FakeWSname;

  std::vector<Mantid::Geometry::MDPoint> pnts1,pnts2;

  //Helper constructional method - based on code from MD_CELL_TEST
  // Returns a cell with one or 2 points depending on npnts
  static Mantid::Geometry::MDCell * constructMDCell(int npnts)
  {
    using namespace Mantid::Geometry;
    std::vector<coordinate> vertices;
    coordinate c;
    c.x = 4;
    c.y = 3;
    c.z = 2;
    c.t = 1;
    vertices.push_back(c);

    std::vector<boost::shared_ptr<MDPoint> > points;
    if(npnts==1) {
       points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(16,4,1,2,3,0)) );
    }
    else if(npnts==2) {
       points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(25,5,1,2,3,1)) );
       points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(36,6,1,2,3,2)) );
    }

    return (MDCell *) (new MDCell(points, vertices));
  }


  // Code from MDPoint test
  class DummyDetector : public Mantid::Geometry::Detector
  {
  public:
    DummyDetector(std::string name) : Mantid::Geometry::Detector(name, NULL) {}
    ~DummyDetector() {}
  };

  class DummyInstrument : public Mantid::Geometry::Instrument
  {
  public:
    DummyInstrument(std::string name) : Mantid::Geometry::Instrument(name) {}
    ~DummyInstrument() {}
  };

  //Helper constructional method.
  static Mantid::Geometry::MDPoint* constructMDPoint(double s, double e, double x, double y, double z, double t)
  {
    using namespace Mantid::Geometry;
    std::vector<coordinate> vertices;
    coordinate c;
    c.x = x;
    c.y = y;
    c.z = z;
    c.t = t;
    vertices.push_back(c);
    IDetector_sptr detector = IDetector_sptr(new DummyDetector("dummydetector"));
    IInstrument_sptr instrument = IInstrument_sptr(new DummyInstrument("dummyinstrument"));
    return new MDPoint(s, e, vertices, detector, instrument);
  }


public:

  testSimulateMDD() {};

  // create a test data set of 3 pixels contributing to 2 points to 1 cut
  void testInit()
  {
    FakeWSname = "test_FakeMDWS";

    pContribCells.push_back(constructMDCell(1));
    pContribCells.push_back(constructMDCell(2));

    TS_ASSERT_THROWS_NOTHING( myCut = boost::shared_ptr<TestCut> (new TestCut(pContribCells) ) );
    TS_ASSERT_EQUALS(myCut->getNPoints(),0);
    TS_ASSERT_THROWS_ANYTHING(myCut->getPoint(0));
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add(FakeWSname, myCut) );

    outCut = boost::dynamic_pointer_cast<TestCut>(AnalysisDataService::Instance().retrieve(FakeWSname));
    TS_ASSERT_EQUALS(outCut->getNPoints(),0);
    TS_ASSERT_EQUALS(myCut->getXDimension()->getNBins(),2);

    Mantid::Geometry::MDCell* myCell;
    std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > contributingPoints;
    std::vector<Mantid::Geometry::coordinate> vertices;

    // test that cells and points are as expected
    int firstCell = 0;
    int secondCell = 1;
    TS_ASSERT_THROWS_NOTHING( myCell=myCut->getCellImp(firstCell) );
    TS_ASSERT_THROWS_NOTHING( contributingPoints=myCell->getContributingPoints() );
    TS_ASSERT_EQUALS(contributingPoints.size(),1);
    TS_ASSERT_THROWS_NOTHING( myCell=myCut->getCellImp(secondCell) );
    TS_ASSERT_THROWS_NOTHING( contributingPoints=myCell->getContributingPoints() );
    TS_ASSERT_EQUALS(contributingPoints.size(),2);
    TS_ASSERT_THROWS_NOTHING(vertices=contributingPoints.at(0)->getVertexes());
    TS_ASSERT_EQUALS(vertices.size(),1);
    TS_ASSERT_EQUALS(vertices.at(0).t,1);
    TS_ASSERT_EQUALS(vertices.at(0).x,1);

  }

  void testExecSimulate()
  {
    using namespace Mantid::MDAlgorithms;

    SimulateMDD alg;

    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
      alg.setPropertyValue("InputMDWorkspace",FakeWSname);
      alg.setPropertyValue("OutputMDWorkspace","test_out1");
      alg.setPropertyValue("BackgroundModel","QuadEnTrans");
      alg.setPropertyValue("BackgroundModel_p1", "1.0" );
      alg.setPropertyValue("BackgroundModel_p2", "0.1" );
      alg.setPropertyValue("BackgroundModel_p3", "0.01" );
      alg.setPropertyValue("ForegroundModel","Simple cubic Heisenberg FM spin waves, DSHO, uniform damping");
    )
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    // for now we put the result into the input workspace, over writing data.

    TS_ASSERT_THROWS_NOTHING( outCut = 
      boost::dynamic_pointer_cast<TestCut>(AnalysisDataService::Instance().retrieve(FakeWSname)) );
    TS_ASSERT_EQUALS( outCut->getNPoints(),0);

    //TS_ASSERT_THROWS_NOTHING( pnt1=outCut->getPnt(0) );
    //TS_ASSERT_DELTA( pnt1->getSignal(), 3.1574, 0.1e-3 );
  }
  void testTidyUp()
  {
  }

};

#endif /*SIMULATEMDDATATEST_H_*/
