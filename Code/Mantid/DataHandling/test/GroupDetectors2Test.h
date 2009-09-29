#ifndef GROUPDETECTORS2TEST_H_
#define GROUPDETECTORS2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/GroupDetectors2.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/exception.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <iostream>
#include <numeric>

using Mantid::DataHandling::GroupDetectors2;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

class GroupDetectors2Test : public CxxTest::TestSuite
{
public:
  GroupDetectors2Test() :
      inputWS("groupdetectorstests_input_workspace"),
        outputBase("groupdetectorstests_output_basename")
  {
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space =
      WorkspaceFactory::Instance().create("Workspace2D", m_nHist, m_nBins+1, m_nBins);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    Histogram1D::RCtype xs, errors, data[m_nHist];
    xs.access().resize(m_nBins+1, 10.0);
    errors.access().resize(m_nBins, 1.0);
    int detIDs[m_nHist];
    int specNums[m_nHist];
    for (int j = 0; j < m_nHist; ++j)
    {
      space2D->setX(j,xs);
      data[j].access().resize(m_nBins, j + 1);  // the y values will be different for each spectra (1+index_number) but the same for each bin
      space2D->setData(j, data[j], errors);
      space2D->getAxis(1)->spectraNo(j) = j+1;  // spectra numbers are also 1 + index_numbers because this is the tradition
      detIDs[j] = j;
      specNums[j] = j+1;
    }
    Detector *d = new Detector("det",0);
    d->setID(0);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d);
    Detector *d1 = new Detector("det",0);
    d1->setID(1);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d1);
    Detector *d2 = new Detector("det",0);
    d2->setID(2);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d2);
    Detector *d3 = new Detector("det",0);
    d3->setID(3);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d3);
    Detector *d4 = new Detector("det",0);
    d4->setID(4);
    boost::dynamic_pointer_cast<Instrument>(space->getInstrument())->markAsDetector(d4);

    // Populate the spectraDetectorMap with fake data to make spectrum number = detector id = workspace index
    space->mutableSpectraMap().populate(specNums, detIDs, m_nHist );

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(inputWS, space);
  }
  
  ~GroupDetectors2Test()
  {
    AnalysisDataService::Instance().remove(inputWS);
  }

  void testSetup()
  {
    GroupDetectors2 gd;
    TS_ASSERT_EQUALS( gd.name(), "GroupDetectors" )
    TS_ASSERT_EQUALS( gd.version(), 2 )
    TS_ASSERT_EQUALS( gd.category(), "DataHandling\\Detectors" )
    TS_ASSERT_THROWS_NOTHING( gd.initialize() )
    TS_ASSERT( gd.isInitialized() )

    gd.setPropertyValue("InputWorkspace", inputWS);
    gd.setPropertyValue("OutputWorkspace", outputBase);
    TS_ASSERT_THROWS_NOTHING( gd.execute());
    TS_ASSERT( ! gd.isExecuted() );
    
    AnalysisDataService::Instance().remove(outputBase);
  }

  void testSpectraList()
  {
    GroupDetectors2 grouper3;
    grouper3.initialize();
    grouper3.setPropertyValue("InputWorkspace", inputWS);
    std::string output(outputBase + "Specs");
    grouper3.setPropertyValue("OutputWorkspace", output);
    grouper3.setPropertyValue("SpectraList","1,4");
    // if you change the default for KeepUngrou... then uncomment what follows    grouper3.setProperty<bool>("KeepUngroupedSpectra",false);
    TS_ASSERT_THROWS_NOTHING( grouper3.execute());
    TS_ASSERT( grouper3.isExecuted() );

    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(output));
    TS_ASSERT_EQUALS( outputWS->getNumberHistograms(), 1 )
    std::vector<double> tens(m_nBins+1, 10.0);
    std::vector<double> ones(m_nBins, 1.0);
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(0), std::vector<double>(m_nBins, (0+1)+(1+3)) )
    for (int i = 0; i < m_nBins; ++i)
    {
      TS_ASSERT_DELTA( outputWS->dataE(0)[i], std::sqrt(double(2)), 0.0001 )
    }

    boost::shared_ptr<IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) )
    TS_ASSERT( boost::dynamic_pointer_cast<DetectorGroup>(det) )
    TS_ASSERT_THROWS_ANYTHING(det = outputWS->getDetector(1))
    
    AnalysisDataService::Instance().remove(output);
  }


  void testDetectorList()
  {
    GroupDetectors2 grouper3;
    grouper3.initialize();
    grouper3.setPropertyValue("InputWorkspace", inputWS);
    std::string output(outputBase + "Detects");
    grouper3.setPropertyValue("OutputWorkspace", output);
    grouper3.setPropertyValue("DetectorList","3,1,4,0,2");
    grouper3.setProperty<bool>("KeepUngroupedSpectra", true);

    TS_ASSERT_THROWS_NOTHING( grouper3.execute());
    TS_ASSERT( grouper3.isExecuted() );

    MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(output));
    TS_ASSERT_EQUALS( outputWS->getNumberHistograms(), 1 )
    std::vector<double> tens(m_nBins+1, 10.0);
    std::vector<double> ones(m_nBins, 1.0);
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(0),
      std::vector<double>(m_nBins, (3+1)+(1+1)+(4+1)+(0+1)+(2+1)) )
    for (int i = 0; i < m_nBins; ++i)
    {
      TS_ASSERT_DELTA( outputWS->dataE(0)[i], std::sqrt(double(5)), 0.0001 )
    }

    boost::shared_ptr<IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) )
    TS_ASSERT( boost::dynamic_pointer_cast<DetectorGroup>(det) )
    TS_ASSERT_THROWS_ANYTHING( det = outputWS->getDetector(1) )
    
    AnalysisDataService::Instance().remove(output);
  }

  void testFileInput()
  {
    GroupDetectors2 grouper;
    grouper.initialize();
    grouper.setPropertyValue("InputWorkspace", inputWS);
    std::string output(outputBase + "File");
    grouper.setPropertyValue("OutputWorkspace", output);
    grouper.setPropertyValue("MapFile", "GroupDetectors2Test_mapfile_example");
    grouper.setProperty<bool>("KeepUngroupedSpectra",true);

    TS_ASSERT_THROWS_NOTHING( grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    MatrixWorkspace_const_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(output));
    TS_ASSERT_EQUALS( outputWS->getNumberHistograms(), m_nHist-1 )
    std::vector<double> tens(m_nBins+1, 10.0);
    std::vector<double> ones(m_nBins, 1.0);
    // check the two grouped spectra
    TS_ASSERT_EQUALS( outputWS->dataX(0), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(0), std::vector<double>(m_nBins, 1+3) );
    for (int i = 0; i < m_nBins; ++i)
    {
      TS_ASSERT_DELTA( outputWS->dataE(0)[i], 1.4142, 0.0001 )
    }
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(0), 1 )
    TS_ASSERT_EQUALS( outputWS->dataX(1), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(1), std::vector<double>(m_nBins, 4 ) );
    
    TS_ASSERT_EQUALS( outputWS->dataE(1), ones )

      TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(1), 4 )
    //check the unmoved spectra
    TS_ASSERT_EQUALS( outputWS->dataX(2), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(2), std::vector<double>(m_nBins, 2) )
    TS_ASSERT_EQUALS( outputWS->dataE(2), ones )
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(2), 2 )
    TS_ASSERT_EQUALS( outputWS->dataX(3), tens )
    TS_ASSERT_EQUALS( outputWS->dataY(3), std::vector<double>(m_nBins, 5) )
    TS_ASSERT_EQUALS( outputWS->dataE(3), ones )
    TS_ASSERT_EQUALS( outputWS->getAxis(1)->spectraNo(3), 5 )

    boost::shared_ptr<IDetector> det;
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(0) )
    TS_ASSERT( boost::dynamic_pointer_cast<DetectorGroup>(det) )
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(1) )
    TS_ASSERT( boost::dynamic_pointer_cast<Detector>(det) )
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(2) )
    TS_ASSERT( boost::dynamic_pointer_cast<Detector>(det) )
    TS_ASSERT_THROWS_NOTHING( det = outputWS->getDetector(3) )
    TS_ASSERT( boost::dynamic_pointer_cast<Detector>(det) )
    
    AnalysisDataService::Instance().remove(output);
  }

  private:
    const std::string inputWS, outputBase;

    enum constants { m_nHist = 5, m_nBins = 4 };
};

#endif /*GROUPDETECTORS2TEST_H_*/
