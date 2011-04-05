#ifndef QUADENBACKGROUNDTEST_H_
#define QUADENBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/QuadEnBackground.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/GenericFit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FunctionFactory.h"

#include <iostream>
#include <boost/scoped_ptr.hpp> 
#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/MDGeometry/MDPoint.h"
#include "MantidGeometry/MDGeometry/MDCell.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidMDAlgorithms/QuadEnBackground.h"

#include <math.h>
#include <cmath>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;
// Implement an IMDWorkspace for testing

// Add a concrete IMDDimension class
namespace Mantid
{
    namespace Geometry
    {
        class DLLExport TestQIMDDimension : public IMDDimension
        {
        public:
            virtual std::string getName() const { return("TestX"); }
            virtual std::string getUnits() const { return("TestUnits"); }
            virtual std::string getDimensionId() const { return("TestX"); }
            virtual bool getIsIntegrated() const {return(0);}
            virtual double getMaximum() const {return(1.0);}
            virtual double getMinimum() const {return(0.0);}
            virtual size_t getNBins() const {return(m_cells);}
            virtual bool isReciprocal() const {return false;}
            virtual std::string toXMLString() const { return "";}
            virtual size_t getStride()const {throw std::runtime_error("Not Implemented");}
            virtual double getScale()const {throw std::runtime_error("Not Implemented");} 
            virtual double getX(size_t ind)const {throw std::runtime_error("Not Implemented");}
            virtual std::vector<double>const & getCoord(void)const {throw std::runtime_error("Not Implemented");}
            virtual void getAxisPoints(std::vector<double>  &)const {throw std::runtime_error("Not Implemented");}
            virtual double getDataShift()const{return 0;}
            virtual V3D getDirection()const{throw std::runtime_error("Not Implemented");}
            virtual V3D getDirectionCryst()const{throw std::runtime_error("Not Implemented");}

            TestQIMDDimension() {
            m_cells=0;
            };
            TestQIMDDimension(int cells) {
            m_cells=cells;
            };
            ~TestQIMDDimension() {};
        private:
            int m_cells;
        };
    }
}

// Minimal IMDWorkspace class
class DLLExport TestQCut : public IMDWorkspace
{
private:
    int m_points;
    int m_cells;
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> m_xDim;
    std::vector<Mantid::Geometry::MDCell> m_mdcells;

public:

    virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getXDimension() const 
    {
        //return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(new Mantid::Geometry::TestIMDDimension(3));
        return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(m_xDim);
    }

    virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getYDimension() const
    {
        throw std::runtime_error("Not implemented");
    }

    virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getZDimension() const
    {
        throw std::runtime_error("Not implemented");
    }

    virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getTDimension() const
    {
        throw std::runtime_error("Not implemented");
    }

    virtual uint64_t getNPoints() const 
    {
        return m_points;
    }

    virtual size_t getNDimensions() const
    {
        throw std::runtime_error("Not implemented");
    }

    const std::vector<std::string> getDimensionIDs() const
    {
        // just one dimensional data in energy (if en is correct)
        std::vector<std::string> ids;
        ids.push_back("en");
        return ids;
    }

    virtual const Mantid::Geometry::SignalAggregate& getPoint(unsigned int index) const
    {
        //throw std::runtime_error("Not implemented");
        // assume that cut is one dimensional and can use idex as dim1Increment
        // assume also that getPoint is really the same as getCell in that the information
        // is about the cell
        return(m_mdcells.at(index));
    }

    virtual const Mantid::Geometry::SignalAggregate& getCell(unsigned int dim1Increment) const
    {
        return(m_mdcells.at(dim1Increment));
    };

    virtual const Mantid::Geometry::SignalAggregate& getCell(unsigned int dim1Increment, unsigned int dim2Increment) const
    {
        throw std::runtime_error("Not implemented");
    }

    virtual const Mantid::Geometry::SignalAggregate&  getCell(unsigned int dim1Increment, unsigned int dim2Increment, unsigned int dim3Increment) const
    {
        throw std::runtime_error("Not implemented");
    }

    virtual const Mantid::Geometry::SignalAggregate&  getCell(unsigned int dim1Increment, unsigned int dim2Increment, unsigned int dim3Increment, unsigned int dim4Increment) const
    {
        throw std::runtime_error("Not implemented");
    }

    virtual const Mantid::Geometry::SignalAggregate&  getCell(...) const
    {
        throw std::runtime_error("Not implemented");
    }

    virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimension(std::string id) const
    {
        // only one dimension in this mock up
        return m_xDim;
    }

    /// return ID specifying the workspace kind
    virtual const std::string id() const {return "TestIMDDWorkspace";}
    /// return number of dimensions in MD workspace
    virtual size_t getNumDims()const{return 4;}
    virtual size_t getMemorySize() const {return 0;};
    virtual std::string getWSLocation() const
    {
        throw std::runtime_error("Not implemented");
    }
    virtual std::string getGeometryXML() const
    {
        throw std::runtime_error("Not implemented");
    }

    TestQCut()
    {
        m_points=0;
        m_cells=0;
    }

    TestQCut(std::vector<Mantid::Geometry::MDCell> pContribCells ) :
    m_mdcells(pContribCells)
    {
        m_cells=pContribCells.size();
        m_points=0;
        m_xDim=boost::shared_ptr<const Mantid::Geometry::IMDDimension>(new Mantid::Geometry::TestQIMDDimension(m_cells));
    }
    ~TestQCut() {};

};

class QuadEnBackgroundTest : public CxxTest::TestSuite
{
private:
    boost::shared_ptr<TestQCut> myCut;
    boost::shared_ptr<TestQCut> outCut;
    std::vector<Mantid::Geometry::MDCell> pContribCells;
    std::string FakeWSname;

    std::vector<Mantid::Geometry::MDPoint> pnts1,pnts2;

    //Helper constructional method - based on code from MD_CELL_TEST
    // Returns a cell with 1, 2 or 3 points depending on npnts
    static Mantid::Geometry::MDCell constructMDCell(int npnts)
    {
        using namespace Mantid::Geometry;
        std::vector<coordinate> vertices;

        std::vector<boost::shared_ptr<MDPoint> > points;
        coordinate c;
        if(npnts==1) {
            points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(16,4,1,2,3,0)) );
            c = coordinate::createCoordinate4D(1, 2, 3, 0);
        }
        else if(npnts==2) {
            points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(25,5,1,2,3,1)) );
            points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(36,6,1,2,3,2)) );
            c = coordinate::createCoordinate4D(1, 2, 3, 1.5);
        }
        else if(npnts==3) {
            points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(49,7,1,2,3,3)) );
            points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(49,7,1,2,3,4)) );
            points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(64,8,1,2,3,5)) );
            c = coordinate::createCoordinate4D(1, 2, 3, 4);
        }
        vertices.push_back(c);

        return  MDCell(points, vertices);
    }


    // Code from MDPoint test
    class DummyDetector : public Mantid::Geometry::Detector
    {
    public:
        DummyDetector(std::string name) : Mantid::Geometry::Detector(name, 0, NULL) {}
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
        coordinate c = coordinate::createCoordinate4D(x, y, z, t);
        vertices.push_back(c);
        IDetector_sptr detector = IDetector_sptr(new DummyDetector("dummydetector"));
        IInstrument_sptr instrument = IInstrument_sptr(new DummyInstrument("dummyinstrument"));
        return new MDPoint(s, e, vertices, detector, instrument);
    }

public:

    QuadEnBackgroundTest() {};

    // create a test data set of 6 MDPoints contributing to 3 MDCells with 1, 2 and 3 points each.
    void testInit()
    {
        FakeWSname = "testFakeMDWSSim";

        pContribCells.push_back(constructMDCell(1));
        pContribCells.push_back(constructMDCell(2));
        pContribCells.push_back(constructMDCell(3));

        myCut = boost::shared_ptr<TestQCut> (new TestQCut(pContribCells) ) ;
        TS_ASSERT_EQUALS(myCut->getNPoints(),0);
        TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().addOrReplace(FakeWSname, myCut) );

        outCut = boost::dynamic_pointer_cast<TestQCut>(AnalysisDataService::Instance().retrieve(FakeWSname));
        TS_ASSERT_EQUALS(outCut->getNPoints(),0);
        TS_ASSERT_EQUALS(myCut->getXDimension()->getNBins(),3);

        std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > contributingPoints;
        std::vector<Mantid::Geometry::coordinate> vertices;

        // test that cells and points are as expected
        int firstCell = 0;
        int secondCell = 1;
        const Mantid::Geometry::SignalAggregate& firstMDCell=myCut->getCell(firstCell);
        TS_ASSERT_THROWS_NOTHING( contributingPoints=firstMDCell.getContributingPoints() );
        TS_ASSERT_EQUALS(contributingPoints.size(),1);
        const Mantid::Geometry::SignalAggregate& secondMDCell=myCut->getCell(secondCell);
        TS_ASSERT_THROWS_NOTHING( contributingPoints=secondMDCell.getContributingPoints() );
        TS_ASSERT_EQUALS(contributingPoints.size(),2);
        TS_ASSERT_THROWS_NOTHING(vertices=contributingPoints.at(0)->getVertexes());
        TS_ASSERT_EQUALS(vertices.size(),1);
        TS_ASSERT_EQUALS(vertices.at(0).gett(),1);
        TS_ASSERT_EQUALS(vertices.at(0).getX(),1);

    }

    void testWithGenericFit()
    {
        GenericFit alg2;
        TS_ASSERT_THROWS_NOTHING(alg2.initialize());
        TS_ASSERT( alg2.isInitialized() );

        // create mock data to test against
        std::string wsName = FakeWSname;

        // set up fitting function
        QuadEnBackground* fn = new QuadEnBackground();
        fn->initialize();

        // Set which spectrum to fit against and initial starting values
        alg2.setPropertyValue("InputWorkspace", wsName);

        alg2.setPropertyValue("Function",*fn);

        // execute fit NOT YET WORKING - needs MDIterator over MDCells, rather than MDPoints
        TS_ASSERT_THROWS_NOTHING(
            TS_ASSERT( alg2.execute() )
            )

        TS_ASSERT( alg2.isExecuted() );

        std::string algStat;
        algStat = alg2.getPropertyValue("Output Status");
        TS_ASSERT( algStat.compare("success")==0 );

        // test the output from fit is as expected - since 3 variables and 3 data points DOF=0
        double dummy = alg2.getProperty("Output Chi^2/DoF");
        TS_ASSERT( dummy==std::numeric_limits<double>::infinity() );

        IFitFunction *out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function"));
        TS_ASSERT_DELTA( out->getParameter("Linear"), 9.777 ,0.02);
        TS_ASSERT_DELTA( out->getParameter("Constant"), 16.0 ,0.01);
        TS_ASSERT_DELTA( out->getParameter("Quadratic"), -.0666 ,0.003);

        // test with output workspace - ties
        GenericFit alg3;
        TS_ASSERT_THROWS_NOTHING(alg3.initialize());
        TS_ASSERT( alg3.isInitialized() );
        // Set which spectrum to fit against and initial starting values
        alg3.setPropertyValue("InputWorkspace", wsName);

        alg3.setPropertyValue("Function",*fn);
        alg3.setPropertyValue("Output","out");

        // execute fit
        TS_ASSERT_THROWS_NOTHING(
            TS_ASSERT( alg3.execute() )
            )
        TS_ASSERT( alg3.isExecuted() );
        algStat = alg3.getPropertyValue("Output Status");
        TS_ASSERT( algStat.compare("success")==0 );
        TWS_type outParams = getTWS("out_Parameters");
        TS_ASSERT(outParams);
        TS_ASSERT_EQUALS(outParams->rowCount(),4);
        TS_ASSERT_EQUALS(outParams->columnCount(),2);

        TableRow row = outParams->getFirstRow();
        TS_ASSERT_EQUALS(row.String(0),"Constant");
        TS_ASSERT_DELTA(row.Double(1),16.0,0.01);

        row = outParams->getRow(1);
        TS_ASSERT_EQUALS(row.String(0),"Linear");
        TS_ASSERT_DELTA(row.Double(1),9.777,0.1);

        row = outParams->getRow(2);
        TS_ASSERT_EQUALS(row.String(0),"Quadratic");
        TS_ASSERT_DELTA(row.Double(1),-0.0666,0.03);

        AnalysisDataService::Instance().remove(wsName);
        removeWS("out_Parameters");

    }
    void testTidyUp()
    {
    }

    TWS_type getTWS(const std::string& name)
    {
        return boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve(name));
    }
    void removeWS(const std::string& name)
    {
        AnalysisDataService::Instance().remove(name);
    }


};

#endif /*QUADENBACKGROUNDTEST_H_*/
