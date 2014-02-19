#ifndef MANTID_ICAT_COMPOSITECATALOGTEST_H_
#define MANTID_ICAT_COMPOSITECATALOGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/CompositeCatalog.h"

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

    void search(const Mantid::ICat::CatalogSearchParam&,
        ITableWorkspace_sptr&, const int &,const int &)
    {
      m_counter++;
    }

    int64_t getNumberOfSearchResults(const Mantid::ICat::CatalogSearchParam&)
    {
      m_counter++;
      return 0;
    }

    void myData(ITableWorkspace_sptr &)
    {
      m_counter++;
    }

    void getDataSets(const std::string& ,ITableWorkspace_sptr&)
    {
      m_counter++;
    }

    void getDataFiles(const std::string&,ITableWorkspace_sptr &)
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
    void testLogin() {}
};

#endif /* MANTID_ICAT_COMPOSITECATALOGTEST_H_ */
