#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/assign/list_of.hpp>

using namespace MantidQt::CustomInterfaces::MuonAnalysisHelper;

using namespace Mantid;
using namespace Mantid::API;

class MuonAnalysisHelperTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisHelperTest *createSuite() { return new MuonAnalysisHelperTest(); }
  static void destroySuite( MuonAnalysisHelperTest *suite ) { delete suite; }

  MuonAnalysisHelperTest()
  {
    FrameworkManager::Instance(); // So that framework is initialized
  }

  void test_getRunLabel_singleWs()
  {
    std::string label = getRunLabel(createWs("MUSR", 15189));
    TS_ASSERT_EQUALS(label, "MUSR00015189");
  }

  void test_getRunLabel_argus()
  {
    std::string label = getRunLabel(createWs("ARGUS", 26577));
    TS_ASSERT_EQUALS(label, "ARGUS0026577");
  }

  void test_getRunLabel_singleWs_tooBigRunNumber()
  {
    std::string label = getRunLabel(createWs("EMU", 999999999));
    TS_ASSERT_EQUALS(label, "EMU999999999");
  }

  void test_getRunLabel_wsList()
  {
    std::vector<Workspace_sptr> list;

    for (int i = 15189; i <= 15193; ++i)
    {
      list.push_back(createWs("MUSR", i));
    }

    std::string label = getRunLabel(list);
    TS_ASSERT_EQUALS(label, "MUSR00015189-93");
  }

  void test_getRunLabel_wsList_wrongOrder()
  {
    std::vector<int> runNumbers = boost::assign::list_of(10)(3)(5)(1)(6);
    std::vector<Workspace_sptr> list;

    for (auto it = runNumbers.begin(); it != runNumbers.end(); ++it)
    {
      list.push_back(createWs("EMU", *it));
    }

    std::string label = getRunLabel(list);
    TS_ASSERT_EQUALS(label, "EMU00000001-10");
  }

  void test_sumWorkspaces()
  {
    MatrixWorkspace_sptr ws1 = WorkspaceCreationHelper::Create2DWorkspace123(1, 3);
    MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(1, 3);
    MatrixWorkspace_sptr ws3 = WorkspaceCreationHelper::Create2DWorkspace123(1, 3);

    std::vector<Workspace_sptr> wsList = boost::assign::list_of(ws1)(ws2)(ws3);

    auto result = boost::dynamic_pointer_cast<MatrixWorkspace>(sumWorkspaces(wsList));

    TS_ASSERT(result);
    if (!result)
      return; // Nothing to check

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(result->blocksize(), 3);

    TS_ASSERT_EQUALS(result->readY(0)[0], 6);
    TS_ASSERT_EQUALS(result->readY(0)[1], 6);
    TS_ASSERT_EQUALS(result->readY(0)[2], 6);

    // Original workspaces shouldn't be touched
    TS_ASSERT_EQUALS(ws1->readY(0)[0], 2);
    TS_ASSERT_EQUALS(ws3->readY(0)[2], 2);
  }

private:

  // Creates a single-point workspace with instrument and runNumber set
  Workspace_sptr createWs(const std::string& instrName, int runNumber)
  {
    Geometry::Instrument_const_sptr instr = boost::make_shared<Geometry::Instrument>(instrName);

    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    ws->setInstrument(instr);

    ws->mutableRun().addProperty("run_number", runNumber);

    return ws;
  }

};


#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_ */
