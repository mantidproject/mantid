#ifndef SAVEFOCUSSEDXYETEST_H_
#define SAVEFOCUSSEDXYETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveFocusedXYE.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/Axis.h"
#include "MantidDataHandling/SaveGSS.h"

#include <Poco/File.h>
#include <string>
#include <fstream>
#include <sstream>

#include <cmath>

class SaveFocussedXYETest : public CxxTest::TestSuite
{
public:

  SaveFocussedXYETest() : m_tol(1e-08) {}

  //
  void testHistogram()
  {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    Workspace2D_sptr workspace = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 1.0);
    workspace->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    TS_ASSERT_DIFFERS(workspace, boost::shared_ptr<Workspace2D>());
    std::string resultWS("result");
    AnalysisDataService::Instance().add(resultWS, workspace);

    Mantid::DataHandling::SaveFocusedXYE saveXYE;
    TS_ASSERT_THROWS_NOTHING(saveXYE.initialize());
    TS_ASSERT_EQUALS(saveXYE.isInitialized(), true);

    saveXYE.setPropertyValue("InputWorkspace", resultWS);
    std::string filename("focussed.test");
    saveXYE.setPropertyValue("Filename", filename);
    filename = saveXYE.getPropertyValue("Filename"); //absolute path
    saveXYE.setPropertyValue("SplitFiles", "False");

    TS_ASSERT_THROWS_NOTHING(saveXYE.execute());

    Poco::File focusfile(filename);
    TS_ASSERT_EQUALS( focusfile.exists(), true );

    std::ifstream filestrm(filename.c_str());
    std::string line;
    int bin_no(1);
    while( getline(filestrm, line) )
    {
      if( line[0] == '#' ) continue;
      double x(0.0), y(0.0), e(0.);
      std::istringstream is(line);
      is >> x >> y >> e;
      switch( bin_no )
      {
      case 1:
        TS_ASSERT_DELTA(x, 1.5, m_tol);
        TS_ASSERT_DELTA(y, 2.0, m_tol);
        TS_ASSERT_DELTA(e, sqrt(2.0), m_tol);
        break;
      case 2:
        TS_ASSERT_DELTA(x, 2.5, m_tol);
        TS_ASSERT_DELTA(y, 2.0, m_tol);
        TS_ASSERT_DELTA(e, sqrt(2.0), m_tol);
        break;
      case 3:
        TS_ASSERT_DELTA(x, 3.5, m_tol);
        TS_ASSERT_DELTA(y, 2.0, m_tol);
        TS_ASSERT_DELTA(e, sqrt(2.0), m_tol);
        break;
      default:
        TS_ASSERT( false );
      }
      ++bin_no;
    }
    filestrm.close();
    focusfile.remove();
    AnalysisDataService::Instance().remove(resultWS);
  }
  void testSaveFocusedXYEWorkspaceGroups()
  {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    Workspace2D_sptr workspace = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 1.0);
    workspace->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    Workspace2D_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 1.0);
    work_in1->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    Workspace2D_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 1.0);
    work_in2->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    Workspace2D_sptr work_in3 = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 1.0);
    work_in3->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    Workspace2D_sptr work_in4 = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 1.0);
    work_in4->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    WorkspaceGroup_sptr wsSptr= WorkspaceGroup_sptr(new WorkspaceGroup);
    if(wsSptr)
    {
      AnalysisDataService::Instance().add("test_in", wsSptr);
      AnalysisDataService::Instance().add("test_in_1", work_in1);
      wsSptr->add("test_in_1");
      AnalysisDataService::Instance().add("test_in_2", work_in2);
      wsSptr->add("test_in_2");
      AnalysisDataService::Instance().add("test_in_3", work_in3);
      wsSptr->add("test_in_3");
      AnalysisDataService::Instance().add("test_in_4", work_in4);
      wsSptr->add("test_in_4");
    }
    Mantid::DataHandling::SaveFocusedXYE saveXYE;
    TS_ASSERT_THROWS_NOTHING(saveXYE.initialize());
    TS_ASSERT_EQUALS(saveXYE.isInitialized(), true);

    saveXYE.setPropertyValue("InputWorkspace", "test_in");
    std::string filename("focussed.txt");
    saveXYE.setPropertyValue("Filename", filename);
    filename = saveXYE.getPropertyValue("Filename"); //get the absolute path
    saveXYE.setPropertyValue("SplitFiles", "False");
    saveXYE.setPropertyValue("Append", "0");

    TS_ASSERT_THROWS_NOTHING(saveXYE.execute());
    Poco::File focusfile(filename);
    TS_ASSERT_EQUALS( focusfile.exists(), true );

    std::ifstream filestrm(filename.c_str());
    std::string line;
    int bin_no(1);
    while( getline(filestrm, line) )
    {
      if( line[0] == '#' ) continue;
      double x(0.0), y(0.0), e(0.);
      std::istringstream is(line);
      is >> x >> y >> e;
      switch( bin_no )
      {
      case 1:
        TS_ASSERT_DELTA(x, 1.5, m_tol);
        TS_ASSERT_DELTA(y, 2.0, m_tol);
        TS_ASSERT_DELTA(e, sqrt(2.0), m_tol);
        break;
      case 2:
        TS_ASSERT_DELTA(x, 2.5, m_tol);
        TS_ASSERT_DELTA(y, 2.0, m_tol);
        TS_ASSERT_DELTA(e, sqrt(2.0), m_tol);
        break;
      case 3:
        TS_ASSERT_DELTA(x, 3.5, m_tol);
        TS_ASSERT_DELTA(y, 2.0, m_tol);
        TS_ASSERT_DELTA(e, sqrt(2.0), m_tol);
        break;
      default:
        TS_ASSERT( false );
      }
      ++bin_no;
      if(bin_no==4)
        bin_no=1;
    }
    filestrm.close();
    focusfile.remove();
    AnalysisDataService::Instance().remove("test_in");
    AnalysisDataService::Instance().remove("test_in_1");
    AnalysisDataService::Instance().remove("test_in_2");
    AnalysisDataService::Instance().remove("test_in_3");
    AnalysisDataService::Instance().remove("test_in_4");
  }

  void testSaveGSSWorkspaceGroups()
  {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    Workspace2D_sptr workspace = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 2.0);
    workspace->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    Workspace2D_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 2.0);
    work_in1->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    Workspace2D_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 2.0);
    work_in2->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    Workspace2D_sptr work_in3 = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 2.0);
    work_in3->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    Workspace2D_sptr work_in4 = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 2.0);
    work_in4->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    WorkspaceGroup_sptr wsSptr= WorkspaceGroup_sptr(new WorkspaceGroup);
    if(wsSptr)
    {
      AnalysisDataService::Instance().add("test_in", wsSptr);
      AnalysisDataService::Instance().add("test_in_1", work_in1);
      wsSptr->add("test_in_1");
      AnalysisDataService::Instance().add("test_in_2", work_in2);
      wsSptr->add("test_in_2");
      AnalysisDataService::Instance().add("test_in_3", work_in3);
      wsSptr->add("test_in_3");
      AnalysisDataService::Instance().add("test_in_4", work_in4);
      wsSptr->add("test_in_4");
    }
    Mantid::DataHandling::SaveGSS saveGSS;
    TS_ASSERT_THROWS_NOTHING( saveGSS.initialize());
    TS_ASSERT_EQUALS( saveGSS.isInitialized(), true);

    saveGSS.setPropertyValue("InputWorkspace", "test_in");
    std::string filename("SaveGSS.txt");
    saveGSS.setPropertyValue("Filename", filename);
    filename = saveGSS.getPropertyValue("Filename"); //absolute path
    saveGSS.setPropertyValue("SplitFiles", "False");
    saveGSS.setPropertyValue("Append", "0");
    saveGSS.setPropertyValue("MultiplyByBinWidth", "1");

    TS_ASSERT_THROWS_NOTHING( saveGSS.execute());
    Poco::File focusfile(filename);
    TS_ASSERT_EQUALS( focusfile.exists(), true );

    std::ifstream filestrm(filename.c_str());
    std::string line;
    int bin_no(1);
    while( getline(filestrm, line) )
    {
      if(line.empty()) continue;
      if( line[0] == '#' ) continue;
      std::string str=line.substr(0,4);
      if(str=="BANK") continue;
      double x(0.0), y(0.0), e(0.);
      std::istringstream is(line);
      is >> x >> y >> e;
      switch( bin_no )
      {
      case 1:
        TS_ASSERT_DELTA(x, 2.0, m_tol); //center of the bin
        TS_ASSERT_DELTA(y, 4.0, m_tol); // width (2.0) * value (2.0)
        TS_ASSERT_DELTA(e, 1.41421356*2.0, m_tol); //error (sqrt(2) * bin width (2.0)
        break;
      case 2:
        TS_ASSERT_DELTA(x, 4.0, m_tol);
        TS_ASSERT_DELTA(y, 4.0, m_tol);
        TS_ASSERT_DELTA(e, 1.41421356*2.0, m_tol);
        break;
      case 3:
        TS_ASSERT_DELTA(x, 6.0, m_tol);
        TS_ASSERT_DELTA(y, 4.0, m_tol);
        TS_ASSERT_DELTA(e, 1.41421356*2.0, m_tol);
        break;
      default:
        TS_ASSERT( false );
      }
      ++bin_no;
      if(bin_no==4)
        bin_no=1;
    }
    filestrm.close();
    focusfile.remove();
    AnalysisDataService::Instance().remove("test_in");
    AnalysisDataService::Instance().remove("test_in_1");
    AnalysisDataService::Instance().remove("test_in_2");
    AnalysisDataService::Instance().remove("test_in_3");
    AnalysisDataService::Instance().remove("test_in_4");
  }



  void testSaveGSSWorkspaceGroups_dont_multiply_bin_width()
  {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    Workspace2D_sptr workspace = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 2.0);
    workspace->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    Workspace2D_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 3, 1.0, 2.0);
    work_in1->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    WorkspaceGroup_sptr wsSptr= WorkspaceGroup_sptr(new WorkspaceGroup);
    if(wsSptr)
    {
      AnalysisDataService::Instance().add("test_in", wsSptr);
      AnalysisDataService::Instance().add("test_in_1", work_in1);
      wsSptr->add("test_in_1");
    }
    Mantid::DataHandling::SaveGSS saveGSS;
    TS_ASSERT_THROWS_NOTHING( saveGSS.initialize());
    TS_ASSERT_EQUALS( saveGSS.isInitialized(), true);

    saveGSS.setPropertyValue("InputWorkspace", "test_in");
    std::string filename("SaveGSS.txt");
    saveGSS.setPropertyValue("Filename", filename);
    filename = saveGSS.getPropertyValue("Filename"); //absolute path
    saveGSS.setPropertyValue("SplitFiles", "False");
    saveGSS.setPropertyValue("Append", "0");
    saveGSS.setPropertyValue("MultiplyByBinWidth", "0");

    TS_ASSERT_THROWS_NOTHING( saveGSS.execute());
    Poco::File focusfile(filename);
    TS_ASSERT_EQUALS( focusfile.exists(), true );

    std::ifstream filestrm(filename.c_str());
    std::string line;
    int bin_no(1);
    while( getline(filestrm, line) )
    {
      if(line.empty()) continue;
      if( line[0] == '#' ) continue;
      std::string str=line.substr(0,4);
      if(str=="BANK") continue;
      double x(0.0), y(0.0), e(0.);
      std::istringstream is(line);
      is >> x >> y >> e;
      switch( bin_no )
      {
      case 1:
        TS_ASSERT_DELTA(x, 2.0, m_tol); //center of the bin
        TS_ASSERT_DELTA(y, 2.0, m_tol); // width (2.0)
        TS_ASSERT_DELTA(e, 1.41421356*1.0, m_tol); //error (sqrt(2)
        break;
      case 2:
        TS_ASSERT_DELTA(x, 4.0, m_tol);
        TS_ASSERT_DELTA(y, 2.0, m_tol);
        TS_ASSERT_DELTA(e, 1.41421356*1.0, m_tol);
        break;
      case 3:
        TS_ASSERT_DELTA(x, 6.0, m_tol);
        TS_ASSERT_DELTA(y, 2.0, m_tol);
        TS_ASSERT_DELTA(e, 1.41421356*1.0, m_tol);
        break;
      default:
        TS_ASSERT( false );
      }
      ++bin_no;
      if(bin_no==4)
        bin_no=1;
    }
    filestrm.close();
    focusfile.remove();
    AnalysisDataService::Instance().remove("test_in");
  }



  void testDistribution()
  {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    Workspace2D_sptr workspace = WorkspaceCreationHelper::Create2DWorkspace154(3, 1, false);
    workspace->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");

    TS_ASSERT_DIFFERS(workspace, boost::shared_ptr<Workspace2D>());
    std::string resultWS("result");
    AnalysisDataService::Instance().add(resultWS, workspace);

    Mantid::DataHandling::SaveFocusedXYE saveXYE;
    TS_ASSERT_THROWS_NOTHING(saveXYE.initialize());
    TS_ASSERT_EQUALS(saveXYE.isInitialized(), true);

    saveXYE.setPropertyValue("InputWorkspace", resultWS);
    std::string filename("focussed.test");
    saveXYE.setPropertyValue("Filename", filename);
    filename = saveXYE.getPropertyValue("Filename"); //absolute path
    saveXYE.setPropertyValue("SplitFiles", "False");

    TS_ASSERT_THROWS_NOTHING(saveXYE.execute());

    Poco::File focusfile(filename);
    TS_ASSERT_EQUALS( focusfile.exists(), true );

    std::ifstream filestrm(filename.c_str());
    std::string line;
    int bin_no(1);
    while( getline(filestrm, line) )
    {
      if( line[0] == '#' ) continue;
      double x(0.0), y(0.0), e(0.);
      std::istringstream is(line);
      is >> x >> y >> e;
      switch( bin_no )
      {
      case 1:
      case 2:
      case 3:
        TS_ASSERT_DELTA(x, 1.0, m_tol);
        TS_ASSERT_DELTA(y, 5.0, m_tol);
        TS_ASSERT_DELTA(e, 4.0, m_tol);
        break;
      default:
        TS_ASSERT( false );
      }
      ++bin_no;
    }
    filestrm.close();
    focusfile.remove();
    AnalysisDataService::Instance().remove(resultWS);
  }

private:
  const double m_tol;
};

#endif //SAVEFOCUSSEDXYETEST_H_
