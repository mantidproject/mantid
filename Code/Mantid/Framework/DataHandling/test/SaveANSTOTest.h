#ifndef SAVEANSTOTEST_H_
#define SAVEANSTOTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveANSTO.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AlgorithmManager.h"
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveANSTOTest : public CxxTest::TestSuite
{

public:

  static SaveANSTOTest *createSuite() { return new SaveANSTOTest(); }
  static void destroySuite(SaveANSTOTest *suite) { delete suite; }

  SaveANSTOTest()
  {
    m_filename = "SaveANSTOTestFile.txt";
    m_name = "SaveANSTOWS";
    for (int i = 1; i < 11; ++i)
    {
      //X, Y and E get [1,2,3,4,5,6,7,8,9,10]
      //0 gets [0,0,0,0,0,0,0,0,0,0] and is used to make sure there is no problem with divide by zero
      m_dataX.push_back(i);
      m_dataY.push_back(i);
      m_dataE.push_back(i);
      m_data0.push_back(0);
    }
  }
  ~SaveANSTOTest()
  {
  }

  void testExec()
  {
    //create a new workspace and then delete it later on
    Mantid::API::IAlgorithm_sptr makews = Mantid::API::AlgorithmManager::Instance().create("CreateWorkspace",1);
    makews->setProperty("OutputWorkspace", m_name);

    makews->setProperty< std::vector<double> >("DataX", m_dataX);
    makews->setProperty< std::vector<double> >("DataY", m_dataY);
    makews->setProperty< std::vector<double> >("DataE", m_dataE);
    // execute the algorithm
    makews->execute();
    if ( ! makews->isExecuted() )
    {
      TS_FAIL("Could not create workspace");
    }

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SaveANSTO");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
   
    if ( ! alg->isExecuted() )
    {
      TS_FAIL("Could not run SaveANSTO");
    }
    std::string filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename).exists() );
    std::ifstream in(filename.c_str());
    double readX, readY, readE, readDQ;
    // Test that the first few column headers, separator and first two bins are as expected
    in >> readX >> readY >> readE >> readDQ;
    TS_ASSERT_EQUALS(readX, 1.5);
    TS_ASSERT_EQUALS(readY, 1);
    TS_ASSERT_EQUALS(readE, 1);
    TS_ASSERT_EQUALS(readDQ, 0.6);
    std::string fullline;
    getline(in,fullline);
    getline(in,fullline);
    std::list<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"), boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(),4);
    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }
  void testNoX()
  {
    //create a new workspace and then delete it later on
    Mantid::API::IAlgorithm_sptr makews = Mantid::API::AlgorithmManager::Instance().create("CreateWorkspace",1);
    makews->setProperty("OutputWorkspace", m_name);
    
    makews->setProperty< std::vector<double> >("DataX", m_data0);
    makews->setProperty< std::vector<double> >("DataY", m_dataY);
    makews->setProperty< std::vector<double> >("DataE", m_dataE);
    // execute the algorithm
    makews->execute();
    if ( ! makews->isExecuted() )
    {
      TS_FAIL("Could not create workspace");
    }

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SaveANSTO");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
   
    if ( ! alg->isExecuted() )
    {
      TS_FAIL("Could not run SaveANSTO");
    }
    std::string filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename).exists() );
    std::ifstream in(filename.c_str());
    double readX, readY, readE, readDQ;
    // Test that the first few column headers, separator and first two bins are as expected
    in >> readX >> readY >> readE >> readDQ;
    TS_ASSERT_EQUALS(readX, 0);
    TS_ASSERT_EQUALS(readY, 1);
    TS_ASSERT_EQUALS(readE, 1);
    TS_ASSERT_EQUALS(readDQ, -1);
    std::string fullline;
    getline(in,fullline);
    getline(in,fullline);
    std::list<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"), boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(),4);
    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }
  void testNoY()
  {
    //create a new workspace and then delete it later on
    Mantid::API::IAlgorithm_sptr makews = Mantid::API::AlgorithmManager::Instance().create("CreateWorkspace",1);
    makews->setProperty("OutputWorkspace", m_name);
    
    makews->setProperty< std::vector<double> >("DataX", m_dataX);
    makews->setProperty< std::vector<double> >("DataY", m_data0);
    makews->setProperty< std::vector<double> >("DataE", m_dataE);
    // execute the algorithm
    makews->execute();
    if ( ! makews->isExecuted() )
    {
      TS_FAIL("Could not create workspace");
    }

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SaveANSTO");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
   
    if ( ! alg->isExecuted() )
    {
      TS_FAIL("Could not run SaveANSTO");
    }
    std::string filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename).exists() );
    std::ifstream in(filename.c_str());
    double readX, readY, readE, readDQ;
    // Test that the first few column headers, separator and first two bins are as expected
    in >> readX >> readY >> readE >> readDQ;
    TS_ASSERT_EQUALS(readX, 1.5);
    TS_ASSERT_EQUALS(readY, 0);
    TS_ASSERT_EQUALS(readE, 1);
    TS_ASSERT_EQUALS(readDQ, 0.6);
    std::string fullline;
    getline(in,fullline);
    getline(in,fullline);
    std::list<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"), boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(),4);
    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }
  void testNoE()
  {
    //create a new workspace and then delete it later on
    Mantid::API::IAlgorithm_sptr makews = Mantid::API::AlgorithmManager::Instance().create("CreateWorkspace",1);
    makews->setProperty("OutputWorkspace", m_name);

    makews->setProperty< std::vector<double> >("DataX", m_dataX);
    makews->setProperty< std::vector<double> >("DataY", m_dataY);
    makews->setProperty< std::vector<double> >("DataE", m_data0);
    // execute the algorithm
    makews->execute();
    if ( ! makews->isExecuted() )
    {
      TS_FAIL("Could not create workspace");
    }

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SaveANSTO");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
   
    if ( ! alg->isExecuted() )
    {
      TS_FAIL("Could not run SaveANSTO");
    }
    std::string filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename).exists() );
    std::ifstream in(filename.c_str());
    double readX, readY, readE, readDQ;
    // Test that the first few column headers, separator and first two bins are as expected
    in >> readX >> readY >> readE >> readDQ;
    TS_ASSERT_EQUALS(readX, 1.5);
    TS_ASSERT_EQUALS(readY, 1);
    TS_ASSERT_EQUALS(readE, 0);
    TS_ASSERT_EQUALS(readDQ, 0.6);
    std::string fullline;
    getline(in,fullline);
    getline(in,fullline);
    std::list<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"), boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(),4);
    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }
  std::string m_filename, m_name;
  std::vector<double> m_dataX, m_dataY, m_dataE, m_data0;
};


#endif /*SAVEANSTOTEST_H_*/
