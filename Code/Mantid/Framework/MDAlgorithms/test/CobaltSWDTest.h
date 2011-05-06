#ifndef COBALTSWDTEST_H_
#define COBALTSWDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/CobaltSpinWaveDSHO.h"
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
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Instrument.h"

#include <math.h>
#include <cmath>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;

// Implement an IMDWorkspace for testing  this is taken from QuadEnBackgroundTest.h
// This version will be used for testing foreground calculation.
// This calculation is not yet present, see below.

// 
namespace Mantid
{
    namespace API
    {
        // trivial iterator assumes that just have 4 MDCells by default
        class DLLExport IMDIterator1 : public IMDIterator
        {
        public:
            virtual size_t getDataSize() const { return 0; }
            virtual double getCoordinate(int i) const { return 0.0;}
            virtual bool next() { m_pnt++; return m_pnt<m_pntMax;}
            virtual size_t getPointer() const {return m_pnt; }

            IMDIterator1() { m_pnt=0; m_pntMax=4;};
            IMDIterator1(size_t pntMax) { m_pnt=0; m_pntMax=pntMax;};
            ~IMDIterator1() {};

        private:
            size_t m_pnt;
            size_t m_pntMax;

        };

    }
}

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

    virtual const Mantid::Geometry::SignalAggregate& getPoint(size_t index) const
    {
        //throw std::runtime_error("Not implemented");
        // assume that cut is one dimensional and can use idex as dim1Increment
        // assume also that getPoint is really the same as getCell in that the information
        // is about the cell
        return(m_mdcells.at(index));
    }

    virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment) const
    {
        return(m_mdcells.at(dim1Increment));
    };

    virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment, size_t dim2Increment) const
    {
        throw std::runtime_error("Not implemented");
    }

    virtual const Mantid::Geometry::SignalAggregate&  getCell(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment) const
    {
        throw std::runtime_error("Not implemented");
    }

    virtual const Mantid::Geometry::SignalAggregate&  getCell(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment, size_t dim4Increment) const
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
    virtual IMDIterator* createIterator() const {return new IMDIterator1();}

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

class CobaltSWDTest : public CxxTest::TestSuite
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
        else if(npnts==4) {
            points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(81,9,1,2,3,6)) );
            points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(81,9,1,2,3,6.5)) );
            points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(100,10,1,2,3,7)) );
            points.push_back(boost::shared_ptr<MDPoint>( constructMDPoint(100,10,1,2,3,7.5)) );
            c = coordinate::createCoordinate4D(1, 2, 3, 6);
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

    CobaltSWDTest() {};

    // create a test data set of 6 MDPoints contributing to 4 MDCells with 1, 2 and 3, 4 points each.
    void testInit()
    {
        FakeWSname = "testFakeMDWSSim";

        pContribCells.push_back(constructMDCell(1));
        pContribCells.push_back(constructMDCell(2));
        pContribCells.push_back(constructMDCell(3));
        pContribCells.push_back(constructMDCell(4));

        myCut = boost::shared_ptr<TestQCut> (new TestQCut(pContribCells) ) ;
        TS_ASSERT_EQUALS(myCut->getNPoints(),0);
        TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().addOrReplace(FakeWSname, myCut) );

        outCut = boost::dynamic_pointer_cast<TestQCut>(AnalysisDataService::Instance().retrieve(FakeWSname));
        TS_ASSERT_EQUALS(outCut->getNPoints(),0);
        TS_ASSERT_EQUALS(myCut->getXDimension()->getNBins(),4);

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
        // test GenericFit - note that fit is to cell data but that MDCell
        // returns the sum of point contributions, not average.
        // As the number of points in a cell varies 1 to 4 this must be taken into
        // account if comparing the fit to the cell data.
        // This is a development version for testing the resolution function integration as done
        // in tobyfit - Currently thiis is only the framework for GenericFit to call a class such
        // as CobaltSpinWaveDSHO which defines he foreground function.

        // *** THERE IS NO IMPLEMENTATION OF THE INTEGRATION METHOD YET ***
        // The function returns zero for all input and the "fit" does nothing.
        // The mechanism for passing run parameters is also not present.

        GenericFit alg2;
        TS_ASSERT_THROWS_NOTHING(alg2.initialize());
        TS_ASSERT( alg2.isInitialized() );

        // create mock data to test against
        std::string wsName = FakeWSname;

        // set up fitting function
        CobaltSpinWaveDSHO* fn = new CobaltSpinWaveDSHO();
        fn->initialize();

        // Set which spectrum to fit against and initial starting values
        alg2.setPropertyValue("InputWorkspace", wsName);

        alg2.setPropertyValue("Function",*fn);

        // execute fit
        TS_ASSERT_THROWS_NOTHING(
            TS_ASSERT( alg2.execute() )
            )

        TS_ASSERT( alg2.isExecuted() );

        std::string algStat;
        algStat = alg2.getPropertyValue("OutputStatus");
        TS_ASSERT( algStat.compare("success")==0 );

        // test the output from fit is as expected - since 3 variables and 4 data points DOF= 1
        double dummy = alg2.getProperty("OutputChi2overDoF");
        // This is INF at present and causes infinite loop
        //TS_ASSERT_DELTA( dummy, 0.0893, 0.001 );

        IFitFunction *out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function"));
        TS_ASSERT_DELTA( out->getParameter("Amplitude"), 0.0 ,0.001);
        TS_ASSERT_DELTA( out->getParameter("12SJ_AA"), 0.0 ,0.001);
        TS_ASSERT_DELTA( out->getParameter("12SJ_AB"), 0.0 ,0.001);
        TS_ASSERT_DELTA( out->getParameter("Gamma"), 0.0 ,0.001);

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

#endif /*COBALTSWDTEST_H_*/
