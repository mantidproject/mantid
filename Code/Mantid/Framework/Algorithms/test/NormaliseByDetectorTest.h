#ifndef MANTID_ALGORITHMS_NORMALISEBYDETECTORTEST_H_
#define MANTID_ALGORITHMS_NORMALISEBYDETECTORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadParameterFile.h"
#include <iostream>
#include <iomanip>
#include <fstream>

#include "MantidAlgorithms/NormaliseByDetector.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

class NormaliseByDetectorTest : public CxxTest::TestSuite
{

private:

  MatrixWorkspace_sptr create_workspace_with_no_fitting_functions()
  {
    IAlgorithm* loadalg = FrameworkManager::Instance().createAlgorithm("Load");
    loadalg->setRethrows(true);
    loadalg->initialize();
    loadalg->setPropertyValue("Filename", "POLREF00004699.nxs");
    loadalg->setPropertyValue("OutputWorkspace", "testws");
    loadalg->execute();
    
    // Convert units to wavelength
    IAlgorithm* unitsalg = FrameworkManager::Instance().createAlgorithm("ConvertUnits");
    unitsalg->initialize();
    unitsalg->setPropertyValue("InputWorkspace", "testws");
    unitsalg->setPropertyValue("OutputWorkspace", "testws");
    unitsalg->setPropertyValue("Target", "Wavelength");
    unitsalg->execute();

    // Convert the specturm axis ot signed_theta
    IAlgorithm* specaxisalg = FrameworkManager::Instance().createAlgorithm("ConvertSpectrumAxis");
    specaxisalg->initialize();
    specaxisalg->setPropertyValue("InputWorkspace", "testws");
    specaxisalg->setPropertyValue("OutputWorkspace", "testws");
    specaxisalg->setPropertyValue("Target", "signed_theta");
    specaxisalg->execute();
    
    WorkspaceGroup_sptr group = API::AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("testws");
    return boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
  }

  MatrixWorkspace_sptr create_workspace_with_fitting_functions()
  {
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

    // Create a parameter file, with a root equation that will apply to all detectors.
    const std::string parameterFileContents = std::string("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n") +
    "<parameter-file instrument = \"POLREF\" date = \"2012-01-31T00:00:00\">\n" +
    "<component-link name=\"POLREF\">\n" +
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

    FileObject file(parameterFileContents, "POLREF_Parameters.xml");
    MatrixWorkspace_sptr ws = create_workspace_with_no_fitting_functions();

    LoadParameterFile loadParameterAlg;
    loadParameterAlg.setRethrows(true);
    loadParameterAlg.initialize();
    loadParameterAlg.setPropertyValue("Filename", file.getFileName());
    loadParameterAlg.setProperty("Workspace", ws);
    loadParameterAlg.execute();
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
      int i;
    std::cin >> i;
    MatrixWorkspace_sptr inputWS = create_workspace_with_no_fitting_functions();

    NormaliseByDetector alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.setProperty("InputWorkspace", inputWS);
    alg.execute();
  }

  void test_applies_instrument_function_to_child_detectors()
  {
    int i;
    std::cin >> i;
    // Fitting function applied to whole instrument, so no detectors should complain.
    MatrixWorkspace_sptr inputWS = create_workspace_with_fitting_functions();
    NormaliseByDetector alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.setProperty("InputWorkspace", inputWS);
    TSM_ASSERT_THROWS_NOTHING("Instrument wide, fitting function applied. Should not throw.", alg.execute());
  }

  

};


#endif /* MANTID_ALGORITHMS_NORMALISEBYDETECTORTEST_H_ */