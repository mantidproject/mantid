#ifndef MANTID_ICAT_COMPOSITECATALOGTEST_H_
#define MANTID_ICAT_COMPOSITECATALOGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidICat/CatalogSearchParam.h"
#include "MantidICat/CompositeCatalog.h"

#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::ICat;

/**
 * Used to verify that multiple catalogs function as expected.
 */
class DummyCatalog : public ICatalog
{
  public:

    void login(const std::string&,const std::string&,const std::string&)
    {
      m_counter++;
    }

    void logout()
    {
      m_counter++;
    }

    void search(const CatalogSearchParam&,ITableWorkspace_sptr&,const int &,const int &)
    {
      m_counter++;
    }

    int64_t getNumberOfSearchResults(const CatalogSearchParam&)
    {
      m_counter++;
      return 0;
    }

    void myData(ITableWorkspace_sptr&)
    {
      m_counter++;
    }

    void getDataSets(const std::string& ,ITableWorkspace_sptr&)
    {
      m_counter++;
    }

    void getDataFiles(const std::string&,ITableWorkspace_sptr&)
    {
      m_counter++;
    }

    void listInstruments(std::vector<std::string>&)
    {
      m_counter++;
    }

    void listInvestigationTypes(std::vector<std::string>&)
    {
      m_counter++;
    }

    void getFileLocation(const long long&,std::string&)
    {
      m_counter++;
    }

    void getDownloadURL(const long long&,std::string&)
    {
      m_counter++;
    }

    const std::string getUploadURL(const std::string&,const std::string&,const std::string&)
    {
      m_counter++;
      return "";
    }

    void keepAlive()
    {
      m_counter++;
    }

    int keepAliveinminutes()
    {
      m_counter++;
      return 0;
    }

    /// The counter used to verify each method works when
    /// multiple counters perform the same operation on the catalog.
    static int m_counter;
};

// Initialise to make global.
int DummyCatalog::m_counter(0);

class CompositeCatalogTest : public CxxTest::TestSuite
{
  public:

    /// Verifies that multiple catalogs are being logged in to.
    void testLogin()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      // Set the inital state to zero as it's static & used in other methods.
      DummyCatalog::m_counter = 0;
      // Add catalogs to the composite catalog.
      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      // This will attempt to login to each catalog that has been added.
      std::string temp = "";
      compositeCatalog->login(temp,temp,temp);
      // Verify that the composite catalog login method works as expected.
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
      // Delete the objects that were manually created.
      delete compositeCatalog;
    }

    void testLogout()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      compositeCatalog->logout();

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

    void testSearch()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      CatalogSearchParam params;
      ITableWorkspace_sptr ws;
      int limit = 0;
      int offset = 0;
      compositeCatalog->search(params,ws,offset,limit);

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

    void testGetNumberOfSearchResults()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      CatalogSearchParam params;
      compositeCatalog->getNumberOfSearchResults(params);

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

    void testMyData()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      ITableWorkspace_sptr ws;
      compositeCatalog->myData(ws);

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

    void testGetDataSets()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      ITableWorkspace_sptr ws;
      std::string id = "";
      compositeCatalog->getDataSets(id,ws);

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

    void testGetDataFiles()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      ITableWorkspace_sptr ws;
      std::string id = "";
      compositeCatalog->getDataFiles(id,ws);

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

    void testListInstruments()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      std::vector<std::string> vector;
      compositeCatalog->listInstruments(vector);

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

    void testListInvestigationTypes()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      std::vector<std::string> vector;
      compositeCatalog->listInvestigationTypes(vector);

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

    void testGetFileLocation()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      const long long temp = 0;
      std::string fileLocation = "";
      compositeCatalog->getFileLocation(temp,fileLocation);

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

    void testGetDownloadURL()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      const long long temp = 0;
      std::string url = "";
      compositeCatalog->getDownloadURL(temp, url);

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

    void testGetUploadURL()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      std::string temp = "";
      compositeCatalog->getUploadURL(temp,temp,temp);

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

    void testKeepAlive()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      compositeCatalog->keepAlive();

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

    void testKeepAliveinminutes()
    {
      CompositeCatalog* compositeCatalog = new CompositeCatalog();
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      compositeCatalog->keepAliveinminutes();

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);

      delete compositeCatalog;
    }

};

#endif /* MANTID_ICAT_COMPOSITECATALOGTEST_H_ */
