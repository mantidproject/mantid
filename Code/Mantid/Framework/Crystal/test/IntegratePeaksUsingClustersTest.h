#ifndef MANTID_CRYSTAL_INTEGRATEPEAKSUSINGCLUSTERSTEST_H_
#define MANTID_CRYSTAL_INTEGRATEPEAKSUSINGCLUSTERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/IntegratePeaksUsingClusters.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/V3D.h"
#include "MantidDataObjects/PeaksWorkspace.h"

#include <boost/assign/list_of.hpp>
#include <boost/tuple/tuple.hpp>

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::MDEvents;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

class IntegratePeaksUsingClustersTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegratePeaksUsingClustersTest *createSuite() { return new IntegratePeaksUsingClustersTest(); }
  static void destroySuite( IntegratePeaksUsingClustersTest *suite ) { delete suite; }

  IntegratePeaksUsingClustersTest()
  {
    FrameworkManager::Instance();
  }

  void test_Init()
  {
    IntegratePeaksUsingClusters alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_peaks_workspace_mandatory()
  {
    IMDHistoWorkspace_sptr mdws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1,1);

    IntegratePeaksUsingClusters alg; 
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", mdws);
    alg.setPropertyValue("OutputWorkspaceMD", "out_md");
    alg.setPropertyValue("OutputWorkspace", "out_peaks");
    TSM_ASSERT_THROWS("PeaksWorkspace required", alg.execute(), std::runtime_error&);
  }

  void test_input_md_workspace_mandatory()
  {
    auto peaksws = WorkspaceCreationHelper::createPeaksWorkspace();

    IntegratePeaksUsingClusters alg; 
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("PeaksWorkspace", peaksws);
    alg.setPropertyValue("OutputWorkspaceMD", "out_md");
    alg.setPropertyValue("OutputWorkspace", "out_peaks");
    TSM_ASSERT_THROWS("InputWorkspace required", alg.execute(), std::runtime_error&);
  }

  void add_fake_md_peak(Workspace_sptr mdws, const size_t& nEvents, const double& h, const double& k, const double& l, const double& radius)
  {
    auto fakeMDEventDataAlg = AlgorithmManager::Instance().create("FakeMDEventData");
    fakeMDEventDataAlg->setChild(true);
    fakeMDEventDataAlg->initialize();
    fakeMDEventDataAlg->setProperty("InputWorkspace", mdws);
    std::stringstream peakstream;
    peakstream  << nEvents << ", " << h << ", " << k << ", " << l << ", " << radius;
    fakeMDEventDataAlg->setPropertyValue("PeakParams", peakstream.str());
    fakeMDEventDataAlg->execute();
  }

  boost::tuple<IMDHistoWorkspace_sptr, IPeaksWorkspace_sptr> 
    make_peak_and_md_ws(const std::vector<V3D>& hklValues, 
    const double& min, const double& max, const double& peakRadius=1, 
    const size_t nEventsInPeak=1000, const size_t& nBins=100)
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);

    auto mdworkspaceAlg = AlgorithmManager::Instance().create("CreateMDWorkspace");
    mdworkspaceAlg->setChild(true);
    mdworkspaceAlg->initialize();
    mdworkspaceAlg->setProperty("Dimensions", 3);
    std::vector<double> extents = boost::assign::list_of(min)(max)(min)(max)(min)(max).convert_to_container<std::vector<double> >();
    mdworkspaceAlg->setProperty("Extents", extents);
    mdworkspaceAlg->setPropertyValue("Names", "H,K,L");
    mdworkspaceAlg->setPropertyValue("Units", "-,-,-");
    mdworkspaceAlg->setPropertyValue("OutputWorkspace", "IntegratePeaksMDTest_MDEWS");
    mdworkspaceAlg->execute();
    Workspace_sptr mdws = mdworkspaceAlg->getProperty("OutputWorkspace");

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS(new PeaksWorkspace());
    peakWS->setInstrument(inst);

    for(size_t i = 0; i<hklValues.size(); ++i)
    {
      Peak peak(inst, 15050, 1.0);

      const double h = hklValues[i][0];
      const double k = hklValues[i][1];
      const double l = hklValues[i][2];

      peak.setHKL(h, k, l);
      peakWS->addPeak(peak);

      add_fake_md_peak(mdws, nEventsInPeak, h, k, l, peakRadius);
    }

    auto binMDAlg = AlgorithmManager::Instance().create("BinMD");
    binMDAlg->setChild(true);
    binMDAlg->initialize();
    binMDAlg->setProperty("InputWorkspace", mdws);
    binMDAlg->setPropertyValue("OutputWorkspace", "output_ws");
    binMDAlg->setProperty("AxisAligned", true);

    std::stringstream dimensionstring;
    dimensionstring  << "," << min << ", " << max << "," << nBins ;

    binMDAlg->setPropertyValue("AlignedDim0", "H" + dimensionstring.str());
    binMDAlg->setPropertyValue("AlignedDim1", "K" + dimensionstring.str());
    binMDAlg->setPropertyValue("AlignedDim2", "L" + dimensionstring.str());
    binMDAlg->execute();

    Workspace_sptr temp = binMDAlg->getProperty("OutputWorkspace");
    IMDHistoWorkspace_sptr outMDWS = boost::dynamic_pointer_cast<IMDHistoWorkspace>(temp);
    return boost::tuple<IMDHistoWorkspace_sptr, IPeaksWorkspace_sptr>(outMDWS, peakWS);
  }


  void test_integrateSinglePeak()
  {
    std::vector<V3D> hklValues;
    hklValues.push_back(V3D(1,1,1));
    auto inputWorkspaces = make_peak_and_md_ws(hklValues, -10, 10);

    auto mdWS = inputWorkspaces.get<0>();
    auto peaksWS = inputWorkspaces.get<1>();

    IntegratePeaksUsingClusters alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", mdWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    alg.setProperty("Threshold", 1000);
    alg.setProperty("RadiusEstimate", 1.1);
    alg.setPropertyValue("OutputWorkspace", "out_ws");
    alg.setPropertyValue("OutputWorkspaceMD", "out_ws_md");
    alg.execute();
    IPeaksWorkspace_sptr out_peaks = alg.getProperty("OutputWorkspace");
    IMDHistoWorkspace_sptr out_clusters = alg.getProperty("OutputWorkspaceMD");
  }

};


#endif /* MANTID_CRYSTAL_INTEGRATEPEAKSUSINGCLUSTERSTEST_H_ */