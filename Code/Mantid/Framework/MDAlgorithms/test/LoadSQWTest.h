#ifndef MANTID_MDEVENTS_LOAD_SQW_TEST_H_
#define MANTID_MDEVENTS_LOAD_SQW_TEST_H_

#include "MantidMDAlgorithms/LoadSQW.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidDataObjects/MDBoxBase.h"

#include <cxxtest/TestSuite.h>


using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using Mantid::Geometry::OrientedLattice;

//Test helper LoadSQWHelper 
class LoadSQWTestHelper
{   

    LoadSQWHelper::dataPositions cdp;
    // helper function
     std::string conv2str(std::streamoff val){return boost::lexical_cast<std::string>(int(val));}
  public: 
    // Constructor with fills in the data corresponding to the test file
    LoadSQWTestHelper()
    {
      // proper field should start from: 
      cdp.if_sqw_start = 18;
      cdp.n_dims_start = 22;
      cdp.sqw_header_start=26;
      //cdp.component_headers_starts //= 106; 2 contributing files
      cdp.detectors_start = 902;
      cdp.data_start      = 676815;
      cdp.n_cell_pix_start= 677439;
      cdp.pix_start       = 677771;
    }
 
    void checkPosCorrect(const LoadSQWHelper::dataPositions &pos)
    {
      if(cdp.if_sqw_start != pos.if_sqw_start){throw(std::invalid_argument(" pixels location differs from expected"+conv2str(pos.if_sqw_start)+
                                                                          " expected: "+conv2str(cdp.if_sqw_start)));}
      if(cdp.n_dims_start != pos.n_dims_start){throw(std::invalid_argument(" n_dims location differs from expected"+conv2str(pos.n_dims_start)+
                                                                           " expected: "+conv2str(cdp.n_dims_start)));}
      if(cdp.sqw_header_start != pos.sqw_header_start){throw(std::invalid_argument(" sqw_header location differs from expected"+conv2str(pos.sqw_header_start)+
                                                                                   " expected: "+conv2str(cdp.sqw_header_start)));}
      if(cdp.detectors_start != pos.detectors_start){throw(std::invalid_argument(" detectors location differs from expected"+conv2str(pos.detectors_start)+
                                                                                 " expected: "+conv2str(cdp.detectors_start)));} 
      if(cdp.data_start != pos.data_start){throw(std::invalid_argument(" data location differs from expected"+conv2str(pos.data_start)+
                                                                       " expected: "+conv2str(cdp.data_start)));}      
      if(cdp.n_cell_pix_start != pos.n_cell_pix_start){throw(std::invalid_argument(" cells pixels location differs from expected"+conv2str(pos.n_cell_pix_start)+
                                                                                   " expected: "+conv2str(cdp.n_cell_pix_start)));}   
      if(cdp.pix_start != pos.pix_start) {throw(std::invalid_argument(" pixels location differs from expected"+conv2str(pos.pix_start)+
                                                                     " expected: "+conv2str(cdp.pix_start)));} 
    }
   
};

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
      // Parse Extract metadata. Including data locations.
      parseMetadata(filename);
    }

    // test if the metadata correspond to what is expected
    void testMetadata()
    {
      LoadSQWTestHelper tester;
      tester.checkPosCorrect(this->m_dataPositions);
      if(this->m_nDataPoints!=580)throw(std::invalid_argument("incorrect number of data points in the file, expected 580, got "+
                                                              boost::lexical_cast<std::string>(int(m_nDataPoints))));
    }
    virtual void readEvents(MDEventWorkspace4* ws) { LoadSQW::readEvents(ws); };
    void readDNDDimensions(MDEventWorkspace4* ws) 
    {
      std::vector<Mantid::Geometry::MDHistoDimensionBuilder> DimVector;
      LoadSQW::readDNDDimensions(DimVector); 
      this->addDimsToWs(ws,DimVector);
    }
    void readSQWDimensions(MDEventWorkspace4* ws)
    {      
      std::vector<Mantid::Geometry::MDHistoDimensionBuilder> DimVector;
      LoadSQW::readDNDDimensions(DimVector,false);
      LoadSQW::readSQWDimensions(DimVector); 
      this->addDimsToWs(ws,DimVector);
    }
    virtual void addLattice(MDEventWorkspace4* ws) { LoadSQW::addLattice(ws); }
    void readBoxSizes(){LoadSQW::readBoxSizes();}
//    void readBoxSizes(){LoadSQW::readBoxSizes();}
};
//=====================================================================================
// Functional Tests
//=====================================================================================
class LoadSQWTest : public CxxTest::TestSuite
{
private:


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

    TS_ASSERT_THROWS_NOTHING(alg.testMetadata());

    MDEventWorkspace4 ws;
    alg.readDNDDimensions(&ws);

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
    TS_ASSERT_EQUALS("A^-1", a->getUnits().ascii());
    TS_ASSERT_EQUALS("A^-1", b->getUnits().ascii());
    TS_ASSERT_EQUALS("A^-1", c->getUnits().ascii());
    TS_ASSERT_EQUALS("meV", d->getUnits().ascii());

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
    alg.readDNDDimensions(&ws);
    ws.initialize();
    alg.readEvents(&ws);

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
  void testReadDNDvsSQWDim()
  {
    ExposedLoadSQW alg;
    alg.initialize();
    alg.setPropertyValue("Filename","test_horace_reader.sqw");
    alg.setPropertyValue("OutputWorkspace", "testAddDimension");
    alg.setup();

    MDEventWorkspace4 ws1;
    alg.readDNDDimensions(&ws1);
    Mantid::Geometry::IMDDimension_const_sptr a = ws1.getDimension(0);
    Mantid::Geometry::IMDDimension_const_sptr b = ws1.getDimension(1);
    Mantid::Geometry::IMDDimension_const_sptr c = ws1.getDimension(2);
    Mantid::Geometry::IMDDimension_const_sptr d = ws1.getDimension(3);

    //Check dimension ids
    TS_ASSERT_EQUALS("qx", a->getDimensionId());
    TS_ASSERT_EQUALS("qy", b->getDimensionId());
    TS_ASSERT_EQUALS("qz", c->getDimensionId());
    TS_ASSERT_EQUALS("en", d->getDimensionId());


    MDEventWorkspace4 ws2;
    alg.readSQWDimensions(&ws2);
    a = ws2.getDimension(0);
    b = ws2.getDimension(1);
    c = ws2.getDimension(2);
    d = ws2.getDimension(3);

    //Check dimension ids
    TS_ASSERT_EQUALS("qx", a->getDimensionId());
    TS_ASSERT_EQUALS("qy", b->getDimensionId());
    TS_ASSERT_EQUALS("qz", c->getDimensionId());
    TS_ASSERT_EQUALS("en", d->getDimensionId());


    alg.setPropertyValue("Filename","slice2D.sqw");
    alg.setup();
    MDEventWorkspace4 ws3;
    alg.readDNDDimensions(&ws3);
    a = ws3.getDimension(0);
    b = ws3.getDimension(1);
    c = ws3.getDimension(2);
    d = ws3.getDimension(3);

    TS_ASSERT_EQUALS("qy", a->getDimensionId());
    TS_ASSERT_EQUALS("en", b->getDimensionId());
    TS_ASSERT_EQUALS("qx", c->getDimensionId());
    TS_ASSERT_EQUALS("qz", d->getDimensionId());


    MDEventWorkspace4 ws4;
    alg.readSQWDimensions(&ws4);
    a = ws4.getDimension(0);
    b = ws4.getDimension(1);
    c = ws4.getDimension(2);
    d = ws4.getDimension(3);

    //Check dimension ids
    TS_ASSERT_EQUALS("qx", a->getDimensionId());
    TS_ASSERT_EQUALS("qy", b->getDimensionId());
    TS_ASSERT_EQUALS("qz", c->getDimensionId());
    TS_ASSERT_EQUALS("en", d->getDimensionId());

  }

  void testRead2DSlice()
  {
    LoadSQW alg;
    alg.initialize();
    alg.setPropertyValue("Filename","slice2D.sqw");
    alg.setPropertyValue("OutputWorkspace", "testRead2D");

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() );
 

  }
};

//=====================================================================================
// Performance Tests
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
