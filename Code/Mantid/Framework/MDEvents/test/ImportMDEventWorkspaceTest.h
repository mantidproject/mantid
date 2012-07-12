#ifndef MANTID_MDEVENTS_IMPORTMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_IMPORTMDEVENTWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <fstream>
#include <iomanip>

#include "MantidMDEvents/ImportMDEventWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class ImportMDEventWorkspaceTest : public CxxTest::TestSuite
{
private:

  /**
Helper type. Creates a test file, and also manages the resource to ensure that the file is closed and removed, no matter what the outcome of the test.
*/
class MDFileObject
{
public:

  /// Create a simple input file.
  MDFileObject(const std::string& filename, const size_t& size, const std::string dimensions_block="DIMENSIONS", const std::string mdevents_block="MDEVENTS") : m_filename(filename)
  {
    m_file.open (filename.c_str());
    m_file << dimensions_block << std::endl;
    m_file << "a, A, U, 10" << std::endl;
    m_file << "b, B, U, 10" << std::endl;
    m_file << mdevents_block << std::endl;
    for(size_t i=1; i<size+1;++i)
    {
      m_file << i << "\t" << i+1 << std::endl;
    }
    m_file.close();
  }

  std::string getFileName() const
  {
    return m_filename;
  }

  /// Free up resources.
  ~MDFileObject()
  {
    m_file.close();
    if( remove( m_filename.c_str() ) != 0 )
      throw std::runtime_error("cannot remove " + m_filename);
  }

private:
  std::string m_filename;
  std::ofstream m_file;
  // Following methods keeps us from being able to put objects of this type on the heap.
  void *operator new(size_t);
  void *operator new[](size_t);
};


public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImportMDEventWorkspaceTest *createSuite() { return new ImportMDEventWorkspaceTest(); }
  static void destroySuite( ImportMDEventWorkspaceTest *suite ) { delete suite; }


  void test_Init()
  {
    ImportMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void testMissingDimensions()
  {
    MDFileObject infile("test_import_mdeventworkspace_test_file.txt", 1, "_");

    ImportMDEventWorkspace alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Filename", infile.getFileName());
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }

  void testMissingMDEventsBlock()
  {
    MDFileObject infile("test_import_mdeventworkspace_test_file.txt", 1, "DIMENSIONS", "_");

    ImportMDEventWorkspace alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setPropertyValue("Filename", infile.getFileName());
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }


};


#endif /* MANTID_MDEVENTS_IMPORTMDEVENTWORKSPACETEST_H_ */