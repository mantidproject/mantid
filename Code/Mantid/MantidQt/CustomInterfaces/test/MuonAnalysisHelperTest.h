#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/MuonAnalysisHelper.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
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

  void test_getRunLabel_singleWs()
  {
    std::string label = getRunLabel(createWs("MUSR", 15189));
    TS_ASSERT_EQUALS(label, "MUSR00015189");
  }

  void test_getRunLabel_singleWs_tooBigRunNumber()
  {
    std::string label = getRunLabel(createWs("ARGUS", 999999999));
    TS_ASSERT_EQUALS(label, "ARGUS999999999");
  }

  void test_getRunLabel_wsList()
  {
    std::vector<Workspace_sptr> list;

    for (int i = 15189; i <= 15199; ++i)
    {
      list.push_back(createWs("MUSR", i));
    }

    std::string label = getRunLabel(list);
    TS_ASSERT_EQUALS(label, "MUSR00015189-99");
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
