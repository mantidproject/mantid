#ifndef MANTID_ICAT_COMPOSITECATALOGTEST_H_
#define MANTID_ICAT_COMPOSITECATALOGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidICat/CatalogSearchParam.h"
#include "MantidICat/CompositeCatalog.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

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
      std::string temp = "";
      // This will attempt to login to each dummy catalog
      // that has been added to the composite created.
      createCompositeCatalog()->login(temp,temp,temp);
      // Verify that the composite catalog login method works as expected.
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void testLogout()
    {
      createCompositeCatalog()->logout();
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void testSearch()
    {
      CatalogSearchParam params;
      ITableWorkspace_sptr ws;
      int limit = 0;
      int offset = 0;
      createCompositeCatalog()->search(params,ws,offset,limit);
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void testGetNumberOfSearchResults()
    {
      CatalogSearchParam params;
      createCompositeCatalog()->getNumberOfSearchResults(params);
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void testMyData()
    {
      ITableWorkspace_sptr ws;
      createCompositeCatalog()->myData(ws);

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void testGetDataSets()
    {
      ITableWorkspace_sptr ws;
      std::string id = "";
      createCompositeCatalog()->getDataSets(id,ws);
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void testGetDataFiles()
    {
      ITableWorkspace_sptr ws;
      std::string id = "";
      createCompositeCatalog()->getDataFiles(id,ws);
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void testListInstruments()
    {
      std::vector<std::string> vector;
      createCompositeCatalog()->listInstruments(vector);

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void testListInvestigationTypes()
    {
      std::vector<std::string> vector;
      createCompositeCatalog()->listInvestigationTypes(vector);
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void testGetFileLocation()
    {
      const long long temp = 0;
      std::string fileLocation = "";
      createCompositeCatalog()->getFileLocation(temp,fileLocation);
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void xtestGetDownloadURL()
    {
      const long long temp = 0;
      std::string url = "";
      createCompositeCatalog()->getDownloadURL(temp, url);
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void xtestGetUploadURL()
    {
      std::string temp = "";
      createCompositeCatalog()->getUploadURL(temp,temp,temp);
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void testKeepAlive()
    {
      createCompositeCatalog()->keepAlive();
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

    void testKeepAliveinminutes()
    {
      createCompositeCatalog()->keepAliveinminutes();
      TS_ASSERT_EQUALS(DummyCatalog::m_counter,2);
    }

  private:

    /**
     * Create a compositeCatalog and add two DummyCatalogs to it.
     * @return A shared pointer to a CompositeCatalog.
     */
    boost::shared_ptr<CompositeCatalog> createCompositeCatalog()
    {
      boost::shared_ptr<CompositeCatalog>compositeCatalog(new CompositeCatalog());
      DummyCatalog::m_counter = 0;

      compositeCatalog->add(boost::make_shared<DummyCatalog>());
      compositeCatalog->add(boost::make_shared<DummyCatalog>());

      TS_ASSERT_EQUALS(DummyCatalog::m_counter,0);

      return compositeCatalog;
    }

};

#endif /* MANTID_ICAT_COMPOSITECATALOGTEST_H_ */
