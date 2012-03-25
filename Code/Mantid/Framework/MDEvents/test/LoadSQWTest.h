#ifndef MANTID_MDEVENTS_LOAD_SQW_TEST_H_
#define MANTID_MDEVENTS_LOAD_SQW_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDEvents/LoadSQW.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidMDEvents/IMDBox.h"
#include <boost/shared_ptr.hpp>

using namespace Mantid::MDEvents;
using Mantid::Geometry::OrientedLattice;

//=====================================================================================
// Functional Tests
//=====================================================================================
class LoadSQWTest : public CxxTest::TestSuite
{
private:

  /* Helper type provides public access to methods for testing. 
  */
  class ExposedLoadSQW : public LoadSQW
  {
  public:
    ExposedLoadSQW() : LoadSQW() {}
    void exec()
    {
      std::runtime_error("Don't use this method, use setup instead, or the full-blown LoadSQW type.");
    }
    //Call instead of execute to set-up.
    virtual void setup()
    {
      std::string filename = getProperty("Filename");
      m_fileStream.open(filename.c_str(), std::ios::binary);

      // Parse Extract metadata. Including data locations.
      parseMetadata();
    }
    virtual void addEvents(MDEventWorkspace4* ws) { LoadSQW::addEvents(ws); };
    virtual void addDimensions(MDEventWorkspace4* ws) { LoadSQW::addDimensions(ws); };
    virtual void addLattice(MDEventWorkspace4* ws) { LoadSQW::addLattice(ws); };
  };

public:

  void testOpenInValidFile()
  {
    LoadSQW alg;
    alg.initialize();
    TS_ASSERT_THROWS(alg.setPropertyValue("Filename","x.sqw"), std::invalid_argument);
  }

  void testAddDimensions()
  {
    //Check that all dimensions from the file are being read-in. These tests characterize what is in the file!
    
    ExposedLoadSQW alg;
    alg.initialize();
    alg.setPropertyValue("Filename","test_horace_reader.sqw");
    alg.setPropertyValue("OutputWorkspace", "testAddDimension");
    alg.setup();

    MDEventWorkspace4 ws;
    alg.addDimensions(&ws);

    TSM_ASSERT_EQUALS("Wrong number of dimensions", 4, ws.getNumDims());

    Mantid::Geometry::IMDDimension_const_sptr a = ws.getDimension(0);
    Mantid::Geometry::IMDDimension_const_sptr b = ws.getDimension(1);
    Mantid::Geometry::IMDDimension_const_sptr c = ws.getDimension(2);
    Mantid::Geometry::IMDDimension_const_sptr d = ws.getDimension(3);

    //Check dimension ids
    TS_ASSERT_EQUALS("qx", a->getDimensionId());
    TS_ASSERT_EQUALS("qy", b->getDimensionId());
    TS_ASSERT_EQUALS("qz", c->getDimensionId());
    TS_ASSERT_EQUALS("en", d->getDimensionId());

    //Check Units
    TS_ASSERT_EQUALS("A^(-1)", a->getUnits());
    TS_ASSERT_EQUALS("A^(-1)", b->getUnits());
    TS_ASSERT_EQUALS("A^(-1)", c->getUnits());
    TS_ASSERT_EQUALS("MeV", d->getUnits());

    //Check Nbins
    TS_ASSERT_EQUALS(3, a->getNBins());
    TS_ASSERT_EQUALS(3, b->getNBins());
    TS_ASSERT_EQUALS(2, c->getNBins());
    TS_ASSERT_EQUALS(2, d->getNBins());

    //Check Limits
    TS_ASSERT_DELTA(3.9197, a->getMaximum(), 0.01);
    TS_ASSERT_DELTA(0.0399, a->getMinimum(), 0.01);
    TS_ASSERT_DELTA(6.6162, b->getMaximum(), 0.01);
    TS_ASSERT_DELTA(-6.5965, b->getMinimum(), 0.01);
    TS_ASSERT_DELTA(6.5965, c->getMaximum(), 0.01);
    TS_ASSERT_DELTA(-6.5965, c->getMinimum(), 0.01);
    TS_ASSERT_DELTA(147.5000, d->getMaximum(), 0.01);
    TS_ASSERT_DELTA(2.5, d->getMinimum(), 0.01);
  }


  void testAddEvents()
  {
    //Check that pixels can be read into events.

    ExposedLoadSQW alg;
    alg.initialize();
    alg.setPropertyValue("Filename","test_horace_reader.sqw");
    alg.setPropertyValue("OutputWorkspace", "testAddDimension");
    alg.setup();

    MDEventWorkspace4 ws;
    alg.addDimensions(&ws);
    ws.initialize();
    alg.addEvents(&ws);

    TSM_ASSERT_EQUALS("Wrong number of events in workspace", 580, ws.getNPoints());
  }

  void testCreateWithoutEvents()
  {
    LoadSQW alg;
    alg.initialize();
    alg.setPropertyValue("Filename","test_horace_reader.sqw");
    alg.setPropertyValue("OutputWorkspace", "wsWithoutEvents");
    alg.setProperty("MetadataOnly", true); //Load only metadata.
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    MDEventWorkspace4::sptr ws =
        boost::dynamic_pointer_cast<MDEventWorkspace4>(Mantid::API::AnalysisDataService::Instance().retrieve("wsWithoutEvents"));

    //Check the product
    TSM_ASSERT_EQUALS("Should have no events!", 0, ws->getNPoints());
    TSM_ASSERT_EQUALS("Wrong number of dimensions", 4, ws->getNumDims());

  }

  void testSuccessfulLoad()
  {
    LoadSQW alg;
    alg.initialize();
    alg.setPropertyValue("Filename","test_horace_reader.sqw");
    alg.setPropertyValue("OutputWorkspace", "createdWs");

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() );
    MDEventWorkspace4::sptr ws =
        boost::dynamic_pointer_cast<MDEventWorkspace4>(Mantid::API::AnalysisDataService::Instance().retrieve("createdWs"));

    //Check the product
    TSM_ASSERT_EQUALS("Wrong number of points", 580, ws->getNPoints());
    TSM_ASSERT_EQUALS("Wrong number of dimensions", 4, ws->getNumDims());
  };

  /*Even though we have no need for the oriented lattice as part of the MDEW yet, test that the functionality
    is there to extract it*/
  void testReadLattice()
  {
    ExposedLoadSQW alg; //Derived type exposes protected method as public ::extractLattice.
    alg.initialize();
    alg.setPropertyValue("Filename","test_horace_reader.sqw");
    alg.setPropertyValue("OutputWorkspace", "testAddDimension");
    alg.setup();

    MDEventWorkspace4 ws;
    alg.addLattice(&ws);

    const Mantid::Geometry::OrientedLattice& lattice = ws.getExperimentInfo(0)->sample().getOrientedLattice();
    TS_ASSERT_DELTA(2.8699, lattice.a1(), 0.0001);
    TS_ASSERT_DELTA(2.8699, lattice.a2(), 0.0001);
    TS_ASSERT_DELTA(2.8699, lattice.a3(), 0.0001);
    TS_ASSERT_DELTA(0.3484, lattice.b1(), 0.0001);
    TS_ASSERT_DELTA(0.3484, lattice.b2(), 0.0001);
    TS_ASSERT_DELTA(0.3484, lattice.b3(), 0.0001);
  }
};

//=====================================================================================
// Perfomance Tests
//=====================================================================================
class LoadSQWTestPerformance : public CxxTest::TestSuite
{
public:

  //Simple benchmark test so that we can monitor changes to performance.
  void testLoading()
  {
    LoadSQW alg;
    alg.initialize();
    alg.setPropertyValue("Filename","test_horace_reader.sqw");
    alg.setPropertyValue("OutputWorkspace", "benchmarkWS");

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() );
    MDEventWorkspace4::sptr  ws =
        boost::dynamic_pointer_cast<MDEventWorkspace4>(Mantid::API::AnalysisDataService::Instance().retrieve("benchmarkWS"));

    //Check the product
    TSM_ASSERT_EQUALS("Wrong number of points", 580, ws->getNPoints());
    TSM_ASSERT_EQUALS("Wrong number of dimensions", 4, ws->getNumDims());
  };
};

#endif
