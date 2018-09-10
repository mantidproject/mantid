#ifndef CONVERTTOMATRIXWORKSPACETEST_H_
#define CONVERTTOMATRIXWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/ConvertToMatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::HistogramData::detail;

class ConvertToMatrixWorkspaceTest : public CxxTest::TestSuite {
public:
  void testName() {
    Mantid::Algorithms::ConvertToMatrixWorkspace cloner;
    TS_ASSERT_EQUALS(cloner.name(), "ConvertToMatrixWorkspace")
  }

  void testVersion() {
    Mantid::Algorithms::ConvertToMatrixWorkspace cloner;
    TS_ASSERT_EQUALS(cloner.version(), 1)
  }

  void testInit() {
    Mantid::Algorithms::ConvertToMatrixWorkspace cloner;
    TS_ASSERT_THROWS_NOTHING(cloner.initialize())
    TS_ASSERT(cloner.isInitialized())
  }

  void testExec_2D_to_2D() {
    Mantid::Algorithms::ConvertToMatrixWorkspace cloner;
    cloner.setChild(true);
    cloner.initialize();
    // create 2D input workspace
    Mantid::API::MatrixWorkspace_sptr in =
        WorkspaceCreationHelper::create2DWorkspace(5, 10);
    // add instance to variable 'in'

    Mantid::API::MatrixWorkspace_sptr out;

    TS_ASSERT_THROWS_NOTHING(cloner.setProperty("InputWorkspace", in))
    TS_ASSERT_THROWS_NOTHING(cloner.setProperty("OutputWorkspace", "out"))
    TS_ASSERT(cloner.execute())

    // retrieve OutputWorkspace produced by execute and set it to out
    TS_ASSERT_THROWS_NOTHING(out = cloner.getProperty("OutputWorkspace"));
    TS_ASSERT(out);
    if (!out)
      return;

    // Best way to test this is to use the CompareWorkspaces algorithm
    Mantid::Algorithms::CompareWorkspaces checker;
    checker.initialize();
    checker.setProperty("Workspace1", in);
    checker.setProperty("Workspace2", out);
    checker.execute();

    TS_ASSERT(checker.getProperty("Result"));
  }

  void testExec_Event_to_2D() {
    Mantid::Algorithms::ConvertToMatrixWorkspace cloner;
    cloner.setChild(true);
    cloner.initialize();
    Mantid::DataObjects::EventWorkspace_sptr in =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 10);

    TS_ASSERT_THROWS_NOTHING(cloner.setProperty("InputWorkspace", in))
    TS_ASSERT_THROWS_NOTHING(cloner.setPropertyValue("OutputWorkspace", "out"))
    TS_ASSERT(cloner.execute())

    Mantid::API::MatrixWorkspace_sptr out;
    TS_ASSERT_THROWS_NOTHING(out = cloner.getProperty("OutputWorkspace"));
    TS_ASSERT(out);
    if (!out)
      return;

    TS_ASSERT_EQUALS(in->getNumberHistograms(), out->getNumberHistograms());
    TS_ASSERT_EQUALS(in->getInstrument()->getName(),
                     out->getInstrument()->getName());
    TS_ASSERT_EQUALS(in->getInstrument()->isParametrized(),
                     out->getInstrument()->isParametrized());
    for (size_t i = 0; i < out->getNumberHistograms(); i++) {
      const auto &inSpec = in->getSpectrum(i);
      const auto &outSpec = out->getSpectrum(i);
      TSM_ASSERT_EQUALS("Failed on comparing Spectrum Number for Histogram: " +
                            boost::lexical_cast<std::string>(i),
                        inSpec.getSpectrumNo(), outSpec.getSpectrumNo());
      TSM_ASSERT_EQUALS("Failed on comparing Detector ID for Histogram: " +
                            boost::lexical_cast<std::string>(i),
                        *inSpec.getDetectorIDs().begin(),
                        *outSpec.getDetectorIDs().begin());
      TSM_ASSERT_EQUALS("Failed on readX for Histogram: " +
                            boost::lexical_cast<std::string>(i),
                        in->x(i), out->x(i));
      TSM_ASSERT_EQUALS("Failed on readY for Histogram: " +
                            boost::lexical_cast<std::string>(i),
                        in->y(i), out->y(i));
      TSM_ASSERT_EQUALS("Failed on readE for Histogram: " +
                            boost::lexical_cast<std::string>(i),
                        in->e(i), out->e(i));
    }
  }
};

#endif /*CONVERTTOMATRIXWORKSPACETEST_H_*/
