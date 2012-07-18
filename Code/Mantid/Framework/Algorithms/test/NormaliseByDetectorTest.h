#ifndef MANTID_ALGORITHMS_NORMALISEBYDETECTORTEST_H_
#define MANTID_ALGORITHMS_NORMALISEBYDETECTORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/FrameworkManager.h"
#include "boost/format.hpp"
#include "boost/algorithm/string.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>

#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidAlgorithms/NormaliseByDetector.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class NormaliseByDetectorTest : public CxxTest::TestSuite
{

private:

  /// File object type. Provides exception save file creation/destruction.
  class FileObject
  {
  public:

    /// Create a simple input file.
    FileObject(const std::string& fileContents, const std::string& filename) : m_filename(filename)
    {
      m_file.open (filename.c_str());
      m_file << fileContents;
      m_file.close();
    }

    std::string getFileName() const
    {
      return m_filename;
    }

    /// Free up resources.
    ~FileObject()
    {
      m_file.close();
      if( remove( m_filename.c_str() ) != 0 )
        throw std::runtime_error("cannot remove " + m_filename);
    }

  private:
    const std::string m_filename;
    std::ofstream m_file;
    // Following methods keeps us from being able to put objects of this type on the heap.
    void *operator new(size_t);
    void *operator new[](size_t);
  };

  /** Helper function, creates a histogram workspace with an instrument with 2 detectors, and 2 spectra.
      Y-values are flat accross the x bins. Which makes it easy to calculate the expected value for any fit function applied to the X-data.
  */
  MatrixWorkspace_sptr create_workspace_with_no_fitting_functions()
  {
    const std::string outWSName="test_ws";
    IAlgorithm* workspaceAlg = FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
    workspaceAlg->initialize();
    workspaceAlg->setPropertyValue("DataX", "1, 2, 3, 4"); // 4 bins.
    workspaceAlg->setPropertyValue("DataY", "1, 1, 1, 1, 1, 1"); // Each spectrum gets 3 Y values
    workspaceAlg->setPropertyValue("DataE", "1, 1, 1, 1, 1, 1"); // Each spectrum gets 3 E values
    workspaceAlg->setPropertyValue("NSpec", "2");
    workspaceAlg->setPropertyValue("UnitX", "Wavelength");
    workspaceAlg->setPropertyValue("OutputWorkspace", outWSName);
    workspaceAlg->execute();
    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    ws->setInstrument(ComponentCreationHelper::createTestInstrumentRectangular(6, 1, 0));
    return ws;
  }

  /**
  Helper function. Runs LoadParameterAlg, to get an instrument parameter definition from a file onto a workspace.
  */
  void apply_instrument_parameter_file_to_workspace(MatrixWorkspace_sptr ws, const FileObject& file)
  {
    // Load the Instrument Parameter file over the existing test workspace + instrument.
    using DataHandling::LoadParameterFile;
    LoadParameterFile loadParameterAlg;
    loadParameterAlg.setRethrows(true);
    loadParameterAlg.initialize();
    loadParameterAlg.setPropertyValue("Filename", file.getFileName());
    loadParameterAlg.setProperty("Workspace", ws);
    loadParameterAlg.execute();
  }

  /**
   Helper function, applies fit functions from a fabricated, fake instrument parameter file ontop of an existing instrument definition.
   The fit function is set at the instrument level.
  */
  MatrixWorkspace_sptr create_workspace_with_fitting_functions()
  {
    // Create a default workspace with no-fitting functions.
    MatrixWorkspace_sptr ws = create_workspace_with_no_fitting_functions();
    const std::string instrumentName = ws->getInstrument()->getName();

    // Create a parameter file, with a root equation that will apply to all detectors.
    const std::string parameterFileContents = std::string("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n") +
    "<parameter-file instrument = \"" + instrumentName + "\" date = \"2012-01-31T00:00:00\">\n" +
    "<component-link name=\"" + instrumentName + "\">\n" +
    "<parameter name=\"LinearBackground:A0\" type=\"fitting\">\n" +
    "  <formula eq=\"1.0\" result-unit=\"Wavelength\"/>\n" +
    "  <fixed />\n" + 
    "</parameter>\n" +   
    "<parameter name=\"LinearBackground:A1\" type=\"fitting\">\n" +
    "  <formula eq=\"2.0\" result-unit=\"Wavelength\"/>\n" +
    "  <fixed />\n" + 
    "</parameter>\n" +
    "</component-link>\n" +
    "</parameter-file>\n";

    // Create a temporary Instrument Parameter file.
    FileObject file(parameterFileContents, instrumentName + "_Parameters.xml");
    
    // Apply parameter file to workspace.
    apply_instrument_parameter_file_to_workspace(ws, file);

    return ws;
  }

   /**
   Helper function, applies fit functions from a fabricated, fake instrument parameter file ontop of an existing instrument definition.
   The fit function is different for every detector.
  */
  MatrixWorkspace_sptr create_workspace_with_detector_level_only_fit_functions()
  {
    // Create a default workspace with no-fitting functions.
    MatrixWorkspace_sptr ws = create_workspace_with_no_fitting_functions();
    const std::string instrumentName = ws->getInstrument()->getName();
   
    const double A1 = 1;
    std::string componentLinks = "";
    for(size_t wsIndex = 0; wsIndex < ws->getNumberHistograms(); ++wsIndex)
    {
      Geometry::IDetector_const_sptr det = ws->getDetector( wsIndex );

      // A0, will vary with workspace index, from detector to detector, A1 is constant = 1.
      componentLinks +=  boost::str(boost::format(
          "<component-link name=\"%1%\">\n\
           <parameter name=\"LinearBackground:A0\" type=\"fitting\">\n\
               <formula eq=\"%2%\" result-unit=\"Wavelength\"/>\n\
               <fixed />\n\
           </parameter>\n\
           <parameter name=\"LinearBackground:A1\" type=\"fitting\">\n\
               <formula eq=\"%3%\" result-unit=\"Wavelength\"/>\n\
               <fixed />\n\
           </parameter>\n\
           </component-link>\n") % det->getName() % wsIndex % A1);
    }

    // Create a parameter file, with a root equation that will apply to all detectors.
    const std::string parameterFileContents = std::string("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n") +
    "<parameter-file instrument = \"" + instrumentName + "\" date = \"2012-01-31T00:00:00\">\n" +
    componentLinks +
    "</parameter-file>\n";

    // Create a temporary Instrument Parameter file.
    FileObject file(parameterFileContents, instrumentName + "_Parameters.xml");
    
    // Apply parameter file to workspace.
    apply_instrument_parameter_file_to_workspace(ws, file);

    return ws;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NormaliseByDetectorTest *createSuite() { return new NormaliseByDetectorTest(); }
  static void destroySuite( NormaliseByDetectorTest *suite ) { delete suite; }


  void test_Init()
  {
    NormaliseByDetector alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_throws_when_no_fit_function_on_detector_tree()
  {
    MatrixWorkspace_sptr inputWS = create_workspace_with_no_fitting_functions();
    NormaliseByDetector alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.setProperty("InputWorkspace", inputWS);
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
  }

  void test_applies_instrument_function_to_child_detectors_throws_nothing()
  {
    // Linear function 2*x + 1 applied to each x-value.
    MatrixWorkspace_sptr inputWS = create_workspace_with_fitting_functions();
    NormaliseByDetector alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.setProperty("InputWorkspace", inputWS);
    TSM_ASSERT_THROWS_NOTHING("Instrument wide, fitting function applied. Should not throw.", alg.execute());
  }

  void test_applies_instrument_function_to_child_detectors_calculates_correctly()
  {
    const std::string outWSName = "normalised_ws";
    // Linear function 2*x + 1 applied to each x-value. INSTRUMENT LEVEL FIT FUNCTION ONLY.
    MatrixWorkspace_sptr inputWS = create_workspace_with_fitting_functions();
    NormaliseByDetector alg;
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.setProperty("InputWorkspace", inputWS);
    alg.execute();
    // Extract the output workspace so that we can verify the normalisation.
    MatrixWorkspace_sptr outWS =AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);

    // Output workspace should have 2 histograms.
    TS_ASSERT_EQUALS(2, outWS->getNumberHistograms());
    // Test the application of the linear function
    for(size_t wsIndex = 0; wsIndex < outWS->getNumberHistograms(); ++wsIndex)
    {
      const MantidVec& yValues = outWS->readY(wsIndex);
      const MantidVec& xValues = outWS->readX(wsIndex);
      const MantidVec& eValues = outWS->readE(wsIndex);

      TS_ASSERT_EQUALS(3, yValues.size());
      TS_ASSERT_EQUALS(3, eValues.size());
      TS_ASSERT_EQUALS(4, xValues.size());

      for(size_t binIndex = 0; binIndex < (xValues.size() - 1); ++binIndex)
      {
        const double wavelength = (xValues[binIndex] + xValues[binIndex+1])/2;
        const double expectedValue = (2*wavelength) + 1; // According to the equation written into the instrument parameter file for the instrument component link.
        TS_ASSERT_EQUALS(expectedValue, yValues[binIndex]);
      }
    }
  }

  void test_distribute_function_parameters_accross_object_hierachy()
  {
    const std::string outWSName = "normalised_ws";
    // Linear function 1*x + N applied to each x-value, where N is the workspace index. DETECTOR LEVEL FIT FUNCTIONS ONLY.
    MatrixWorkspace_sptr inputWS = create_workspace_with_detector_level_only_fit_functions();
    NormaliseByDetector alg;
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", outWSName);
    alg.setProperty("InputWorkspace", inputWS);
    alg.execute();
    // Extract the output workspace so that we can verify the normalisation.
    MatrixWorkspace_sptr outWS =AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);

    // Output workspace should have 2 histograms.
    TS_ASSERT_EQUALS(2, outWS->getNumberHistograms());
    // Test the application of the linear function
    for(size_t wsIndex = 0; wsIndex < outWS->getNumberHistograms(); ++wsIndex)
    {
      const MantidVec& yValues = outWS->readY(wsIndex);
      const MantidVec& xValues = outWS->readX(wsIndex);
      const MantidVec& eValues = outWS->readE(wsIndex);

      TS_ASSERT_EQUALS(3, yValues.size());
      TS_ASSERT_EQUALS(3, eValues.size());
      TS_ASSERT_EQUALS(4, xValues.size());

      for(size_t binIndex = 0; binIndex < (xValues.size() - 1); ++binIndex)
      {
        const double wavelength = (xValues[binIndex] + xValues[binIndex+1])/2;
        const double expectedValue = (1*wavelength) + wsIndex; // According to the equation written into the instrument parameter file for the detector component link.
        TS_ASSERT_EQUALS(expectedValue, yValues[binIndex]);
      }
    }
  }

  

};


#endif /* MANTID_ALGORITHMS_NORMALISEBYDETECTORTEST_H_ */