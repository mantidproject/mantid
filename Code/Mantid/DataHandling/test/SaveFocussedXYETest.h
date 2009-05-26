#ifndef SAVEFOCUSSEDXYETEST_H_
#define SAVEFOCUSSEDXYETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveFocusedXYE.h"
#include "../../Algorithms/test/WorkspaceCreationHelper.hh"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/Axis.h"

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
