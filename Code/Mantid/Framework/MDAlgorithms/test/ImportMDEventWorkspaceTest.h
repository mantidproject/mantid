#ifndef MANTID_MDEVENTS_IMPORTMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_IMPORTMDEVENTWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include "MantidMDAlgorithms/ImportMDEventWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/Path.h>


using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Geometry;

/*
This builder type provides a convenient way to create and change the contents of a virtual file of the type expected 
by the ImportMDEventWorkspace algorithm.

This type is particularly useful when generating corrupt file contents, as it allows individual apects of the file contents to be modified independently.
*/
class FileContentsBuilder
{
private:
  std::string m_DimensionBlock;
  std::string m_MDEventsBlock;
  std::string m_DimensionEntries;
  std::string m_MDEventEntries;
public:
  FileContentsBuilder() : 
      m_DimensionBlock(ImportMDEventWorkspace::DimensionBlockFlag()), 
        m_MDEventsBlock(ImportMDEventWorkspace::MDEventBlockFlag()),
        m_DimensionEntries("a A U 10\nb B U 11"),
        m_MDEventEntries("1 1 1 1")
      {
      }

      void setDimensionBlock(const std::string& value)
      {
        m_DimensionBlock = value;
      }

      void setMDEventBlock(const std::string& value)
      {
        m_MDEventsBlock = value;
      }

      void setDimensionEntries(const std::string& value)
      {
        m_DimensionEntries = value;
      }

      void setMDEventEntries(const std::string& value)
      {
        m_MDEventEntries = value;
      }

      std::string create() const
      {
        const std::string newline = "\n";
        return m_DimensionBlock + newline + m_DimensionEntries + newline + m_MDEventsBlock + newline + m_MDEventEntries + newline;
      }
};


/**
Helper type. Creates a test file, and also manages the resource to ensure that the file is closed and removed, no matter what the outcome of the test.

Uses a 
*/
class MDFileObject
{
public:

  /// Create a simple input file.
  MDFileObject(const FileContentsBuilder& builder = FileContentsBuilder(), std::string filename="test_import_md_event_workspace_file.txt") 
  {
    Poco::Path path(Mantid::Kernel::ConfigService::Instance().getTempDir().c_str());
    path.append(filename);
    m_filename = path.toString();
    m_file.open (m_filename.c_str(), std::ios_base::out);
    // Invoke the builder to create the contents of the file.
    m_file << builder.create();
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

class ImportMDEventWorkspaceTest : public CxxTest::TestSuite
{
private:

/**
Helper method runs tests that should throw and invalid argument when the algorithm is executed.
*/
void do_check_throws_invalid_alg_upon_execution(const MDFileObject& infile)
{
  ImportMDEventWorkspace alg;
  alg.initialize();
  alg.setRethrows(true);
  alg.setPropertyValue("Filename", infile.getFileName());
  alg.setPropertyValue("OutputWorkspace", "test_out");
  TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
}

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImportMDEventWorkspaceTest *createSuite() { return new ImportMDEventWorkspaceTest(); }
  static void destroySuite( ImportMDEventWorkspaceTest *suite ) { delete suite; }

  void test_catagory()
  {
    ImportMDEventWorkspace alg;
    TS_ASSERT_EQUALS("MDAlgorithms", alg.category());
  }

  void test_name()
  {
    ImportMDEventWorkspace alg;
    TS_ASSERT_EQUALS("ImportMDEventWorkspace", alg.name());
  }

  void test_Init()
  {
    ImportMDEventWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_missing_dimension_block_throws()
  {
    // Setup the corrupt file
    FileContentsBuilder fileContents;
    fileContents.setDimensionBlock("");
    MDFileObject infile(fileContents);
    // Run the test.
    do_check_throws_invalid_alg_upon_execution(infile);
  }

  void test_missing_mdevents_block_throws()
  {
    // Setup the corrupt file 
    FileContentsBuilder fileContents;
    fileContents.setMDEventBlock("");
    MDFileObject infile(fileContents);
    // Run the test.
    do_check_throws_invalid_alg_upon_execution(infile);
  }

  void test_mdevent_block_declared_before_dimension_block_throws()
  {
    // Setup the corrupt file. Notice that the DimensionBlockFlag and the MDEventBlockFlag arguments have been swapped over.
    FileContentsBuilder fileContents;
    fileContents.setDimensionBlock(ImportMDEventWorkspace::MDEventBlockFlag());
    fileContents.setMDEventBlock(ImportMDEventWorkspace::DimensionBlockFlag());
    MDFileObject infile(fileContents);
    // Run the test.
    do_check_throws_invalid_alg_upon_execution(infile);
  }

  void test_dimension_block_has_corrupted_entries_throws()
  {
    // Setup the corrupt file. 
    FileContentsBuilder fileContents;
    std::string dim1 = "a A U 10\n";
    std::string dim2 = "b B U 11\n";
    std::string dim3 = "b B U\n"; // Ooops, forgot to put in the number of bins for this dimension.
    fileContents.setDimensionEntries(dim1 + dim2 + dim3);
    MDFileObject infile(fileContents);
    // Run the test.
    do_check_throws_invalid_alg_upon_execution(infile);
  }

  void test_type_of_entries_in_dimension_block_is_wrong()
  {
    // Setup the corrupt file. 
    FileContentsBuilder fileContents;
    std::string dim1 = "a A U 10\n";
    std::string dim2 = "b B U 11\n";
    std::string dim3 = "b B U x\n"; // Ooops, correct number of entries, but nbins set to be x!
    fileContents.setDimensionEntries(dim1 + dim2 + dim3);
    MDFileObject infile(fileContents);
    // Run the test.
    do_check_throws_invalid_alg_upon_execution(infile);
  }

  void test_event_type_not_specified_and_mdevent_block_wrong_size_throws()
  {
    // Setup the corrupt file. 
    FileContentsBuilder fileContents;
    fileContents.setMDEventEntries("1 1 1 1 1"); // Should have 4 or 6 entries, but 5 given.
    MDFileObject infile(fileContents);
    // Run the test.
    do_check_throws_invalid_alg_upon_execution(infile);
  }

  void test_mdevent_block_contains_wrong_types_throws()
  {
    // Setup the corrupt file. 
    FileContentsBuilder fileContents;
    fileContents.setMDEventEntries("1.0 1.0 2.1 2.1 1.0 1.0"); // The 3rd and 4th entries relate to run_no and detector_no, these should not be doubles!
    MDFileObject infile(fileContents);
    // Run the test.
    do_check_throws_invalid_alg_upon_execution(infile);
  }

  void test_loaded_dimensionality()
  {
    // Setup the corrupt file. 
    FileContentsBuilder fileContents; 
    fileContents.setMDEventEntries("1 1 -1 -2\n1 1 2 3"); // mins -1, -2, maxs 2, 3
    MDFileObject infile(fileContents);
    // Run the algorithm.
    ImportMDEventWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("Filename", infile.getFileName());
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    IMDEventWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("test_out");

    TS_ASSERT_EQUALS(2, outWS->getNumDims());
    IMDDimension_const_sptr dim1 = outWS->getDimension(0);
    IMDDimension_const_sptr dim2 = outWS->getDimension(1);

    TS_ASSERT_EQUALS("a", dim1->getName());    
    TS_ASSERT_EQUALS("A", dim1->getDimensionId());
    TS_ASSERT_EQUALS("U", dim1->getUnits().ascii());
    TS_ASSERT_EQUALS(-1, dim1->getMinimum());
    TS_ASSERT_EQUALS(2, dim1->getMaximum());

    TS_ASSERT_EQUALS("b", dim2->getName());   
    TS_ASSERT_EQUALS("B", dim2->getDimensionId());
    TS_ASSERT_EQUALS("U", dim2->getUnits().ascii());
    TS_ASSERT_EQUALS(-2, dim2->getMinimum());
    TS_ASSERT_EQUALS(3, dim2->getMaximum());
  }

  void test_load_lean_mdevents_2d()
  {
    // Setup the corrupt file. 
    FileContentsBuilder fileContents; 
    fileContents.setMDEventEntries("1 1 -1 -2\n1 1 2 3"); // mins -1, -2, maxs 2, 3
    MDFileObject infile(fileContents);
    // Run the algorithm.
    ImportMDEventWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("Filename", infile.getFileName());
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    IMDEventWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("test_out");

    TS_ASSERT(alg.isExecuted());
    TS_ASSERT_EQUALS(2, outWS->getNumDims());

    TS_ASSERT_EQUALS(2, outWS->getNPoints());
    TS_ASSERT_EQUALS("MDLeanEvent", outWS->getEventTypeName());
  }

  void test_load_full_mdevents_2d()
  {
    // Setup the corrupt file. 
    FileContentsBuilder fileContents; 
    fileContents.setMDEventEntries("1 1 1 2 -1 -2\n1 1 2 3 2 3\n1 1 3 4 5 6"); // mins -1, -2, maxs 2, 3
    MDFileObject infile(fileContents);
    // Run the algorithm.
    ImportMDEventWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("Filename", infile.getFileName());
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    
    IMDEventWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("test_out");
    TS_ASSERT_EQUALS(2, outWS->getNumDims());

    TS_ASSERT_EQUALS(3, outWS->getNPoints());
    TS_ASSERT_EQUALS("MDEvent", outWS->getEventTypeName());
  }

  void test_load_full_mdevents_3d()
  {
    // Setup the corrupt file.
    FileContentsBuilder fileContents;

    const std::string dim1 = "a A U 10\n";
    const std::string dim2 = "b B U 11\n";
    const std::string dim3 = "c C U 12\n"; // Ooops, forgot to put in the number of bins for this dimension.

    fileContents.setDimensionEntries(dim1 + dim2 + dim3);
    fileContents.setMDEventEntries("1 1 1 2 -1 -2 3\n1 1 2 3 2 3 3\n1 1 3 4 5 6 3"); // mins -1, -2, maxs 2, 3
    MDFileObject infile(fileContents);
    // Run the algorithm.
    ImportMDEventWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("Filename", infile.getFileName());
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    IMDEventWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("test_out");
    TS_ASSERT_EQUALS(3, outWS->getNumDims());

    TS_ASSERT_EQUALS(3, outWS->getNPoints());
    TS_ASSERT_EQUALS("MDEvent", outWS->getEventTypeName());
  }

  void test_ignore_comment_lines()
  {
    // Setup the basic file.
    FileContentsBuilder fileContents; 
    // Insert a few comment blocks into the file.
    fileContents.setDimensionBlock("# Some Comment!\n" + ImportMDEventWorkspace::DimensionBlockFlag());
    fileContents.setMDEventBlock("# Some Comment!\n" + ImportMDEventWorkspace::MDEventBlockFlag());

    MDFileObject infile(fileContents);
    // Run the algorithm.
    ImportMDEventWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("Filename", infile.getFileName());
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    // These comment blocks are not being considered if execution completes without throwing
    TS_ASSERT(alg.isExecuted());
    
    // Sanity check the defaults for the FileContentsBuilder construction.
    IMDEventWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("test_out");
    TS_ASSERT_EQUALS(2, outWS->getNumDims());
    TS_ASSERT_EQUALS(1, outWS->getNPoints());
    TS_ASSERT_EQUALS("MDLeanEvent", outWS->getEventTypeName());
  }
};

/**
Performance Tests
*/
class ImportMDEventWorkspaceTestPerformance : public CxxTest::TestSuite
{
private:

  const size_t nRows;
  boost::shared_ptr<MDFileObject> infile;

public:
  static ImportMDEventWorkspaceTestPerformance *createSuite() { return new ImportMDEventWorkspaceTestPerformance(); }
  static void destroySuite( ImportMDEventWorkspaceTestPerformance *suite ) { delete suite; }

  ImportMDEventWorkspaceTestPerformance() : nRows(10000)
  {
  }

  void setUp()
  {
    // Create the file contents.
    FileContentsBuilder fileContents;
    std::string mdData;
    for(size_t i = 0; i < nRows; ++i)
    {
      // Create MDEvents
      std::stringstream stream;
      stream << i << " " << i << " " << i << " " << i << " " << i << " " << i << "\n"; 
      mdData += stream.str();
    }
    // Create a file from the contents.
    fileContents.setMDEventEntries(mdData); 
    infile = boost::make_shared<MDFileObject>(fileContents);
  }

  void testRead()
  {
    ImportMDEventWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("Filename", infile->getFileName());
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    
    IMDEventWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("test_out");
    TS_ASSERT_EQUALS(2, outWS->getNumDims());

    TS_ASSERT_EQUALS(nRows, outWS->getNPoints());
    TS_ASSERT_EQUALS("MDEvent", outWS->getEventTypeName());
  }
};


#endif /* MANTID_MDEVENTS_IMPORTMDEVENTWORKSPACETEST_H_ */
