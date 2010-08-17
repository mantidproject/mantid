// If you get the message  “This application has failed to start because MSVCR80.dll was not found. Re-installing the application may fix this problem.”
// when running to run this main.cpp in debug mode then try to uncomment the line below (see also http://blogs.msdn.com/dsvc/archive/2008/08/07/part-2-troubleshooting-vc-side-by-side-problems.aspx for more details)
//#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.VC80.CRT' version='8.0.50608.0' processorArchitecture='X86' publicKeyToken='1fc8b3b9a1e18e3b' \"") 

#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <fstream>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmObserver.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ConfigService.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/Exception.h"

#include <muParser.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
//using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
//using namespace Mantid::Geometry;
//using namespace Mantid::NeXus;

typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;
WS_type mkWS(int nSpec,int nY, bool isHist=false);
WS_type mkWS(int nSpec,double x0,double x1,double dx, const std::string& formula,bool isHist=false);
void plotWS(WS_type ws,const std::string& fname);
void storeWS(WS_type ws,const std::string& name);
void removeWS(const std::string& name);
WS_type getWS(const std::string& name);
TWS_type getTWS(const std::string& name);
WS_type loadNexus(const std::string& fName,const std::string& wsName);
void saveNexus(const std::string& fName,const std::string& wsName);
WS_type loadRaw(const std::string& fName,const std::string& wsName);
void addNoise(WS_type ws,double noise);
#define _TRY_ try{std::cerr<<'\n';
#define _CATCH_   std::cerr<<"\n\nTests OK\n\n";}\
  catch(std::exception& e)\
  {\
    std::cerr<<"\nTests stopped with error: "<<e.what()<<'\n';\
  }\
  catch(...)\
  {\
    std::cerr<<"\nUnknown exception\n\n";\
  }

const std::string DataDir = "C:\\Mantid\\Test\\Data\\";
const std::string Desktop = "C:\\Documents and Settings\\hqs74821\\Desktop\\";
const std::string TmpDir = Desktop + "tmp\\";

//-------------------------------------------------------------//
//-------------------------------------------------------------//

int main()
{

  _TRY_;
  FrameworkManager::Instance();

  IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("TestAlgorithm");
  alg->initialize();
  alg->execute();

  _CATCH_;

}

Mantid::DataObjects::Workspace2D_sptr mkWS(int nSpec,int nY, bool isHist)
{
  FrameworkManager::Instance();
  int nX = nY + (isHist?1:0);
  Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
    (WorkspaceFactory::Instance().create("Workspace2D",nSpec,nX,nY));
  return ws;
}

WS_type mkWS(int nSpec,double x0,double x1,double dx, const std::string& formula,bool isHist)
{
  int nX = int(x1 - x0)/dx + 1;
  int nY = nX - (isHist?1:0);
  if (nY <= 0)
    throw std::invalid_argument("Cannot create an empty workspace");

  Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
    (WorkspaceFactory::Instance().create("Workspace2D",nSpec,nX,nY));

  double spec;
  double x;

  mu::Parser parser;
  parser.SetExpr(formula);
  parser.DefineVar("i",&spec);
  parser.DefineVar("x",&x);

  for(int iSpec=0;iSpec<nSpec;iSpec++)
  {
    spec = iSpec;
    Mantid::MantidVec& X = ws->dataX(iSpec);
    Mantid::MantidVec& Y = ws->dataY(iSpec);
    Mantid::MantidVec& E = ws->dataE(iSpec);
    for(int i=0;i<nY;i++)
    {
      x = x0 + dx*i;
      X[i] = x;
      Y[i] = parser.Eval();
      E[i] = 1;
    }
    if (isHist)
      X.back() = X[nY-1] + dx;
  }
  return ws;
}

void plotWS(WS_type ws,const std::string& fname)
{
  std::string fn = "C:\\Documents and Settings\\hqs74821\\Desktop\\tmp\\" + fname;
  std::ofstream fil(fn.c_str());
  char sep = '\t';
  for(int i=0;i<ws->blocksize();i++)
  {
    fil << ws->readX(0)[i];
    for(int j=0;j<ws->getNumberHistograms();j++)
      fil << sep << ws->readY(j)[i] << sep << ws->readE(j)[i];
    fil << '\n';
  }
  fil.close();
}

void storeWS(WS_type ws,const std::string& name)
{
  AnalysisDataService::Instance().add(name,ws);
}

void removeWS(const std::string& name)
{
  AnalysisDataService::Instance().remove(name);
}

WS_type getWS(const std::string& name)
{
  return boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve(name));
}

TWS_type getTWS(const std::string& name)
{
  return boost::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve(name));
}

void addNoise(WS_type ws,double noise)
{
  for(int iSpec=0;iSpec<ws->getNumberHistograms();iSpec++)
  {
    Mantid::MantidVec& Y = ws->dataY(iSpec);
    Mantid::MantidVec& E = ws->dataE(iSpec);
    for(int i=0;i<Y.size();i++)
    {
      Y[i] += noise*(-.5 + double(rand())/RAND_MAX);
      E[i] += noise;
    }
  }
}

WS_type loadNexus(const std::string& fName,const std::string& wsName)
{
  IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("LoadNexus");
  alg->initialize();
  alg->setPropertyValue("FileName",fName);
  alg->setPropertyValue("OutputWorkspace",wsName);
  //alg->setPropertyValue("Autogroup","1");
  alg->execute();
  return getWS(wsName);
}

void saveNexus(const std::string& fName,const std::string& wsName)
{
  IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("SaveNexus");
  alg->initialize();
  alg->setPropertyValue("FileName",fName);
  alg->setPropertyValue("InputWorkspace",wsName);
  alg->execute();
}

WS_type loadRaw(const std::string& fName,const std::string& wsName)
{
  IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("LoadRaw");
  alg->initialize();
  alg->setPropertyValue("FileName",fName);
  alg->setPropertyValue("OutputWorkspace",wsName);
  alg->execute();
  return getWS(wsName);
}
