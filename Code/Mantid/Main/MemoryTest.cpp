//-----------------------------------
//Includes
//-----------------------------------
#include "MemoryTest.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MemoryManager.h"


#include<string>
#include<iostream>

using namespace Mantid::API;
using namespace Mantid::Kernel;

/// Get a refernce to the logger
Logger& MemoryTest::g_log = Logger::get("MemoryTest");


//-----------------------------------
// Public methods
//-----------------------------------

/**
 * Run the tests to check memory usage
 */
void MemoryTest::runMemoryTests() const
{
  FrameworkManagerImpl& fmgr = FrameworkManager::Instance();
 
  MemoryManagerImpl& memMan = MemoryManager::Instance();
  long mem_start = memMan.getMemoryInfo().availMemory;
  std::cerr << "\nStarted memory tests with " << mem_start << " KB of memory available\n";
  
  //First load some GEM data

  IAlgorithm* alg = fmgr.createAlgorithm("LoadRaw");
  alg->setPropertyValue("Filename", "../../../../Test/Data/GEM38370.raw");
  std::string loadraw ("GEM38370");
  alg->setPropertyValue("OutputWorkspace", loadraw);    
  alg->execute();

  std::cerr << "Loaded GEM data. " << memMan.getMemoryInfo().availMemory 
	    << " KB of memory available\n";
   
  //Convert units to dspacing
  alg = fmgr.createAlgorithm("ConvertUnits");
  alg->setPropertyValue("InputWorkspace", loadraw);
  std::string target("dspacing_GEM");
  alg->setPropertyValue("OutputWorkspace", target);
  alg->setPropertyValue("Target","dSpacing");
  alg->execute();

  std::cerr << "Converted units to dSpacing. " << memMan.getMemoryInfo().availMemory 
	    << " KB of memory available\n";

  //Convert units to wavelength
  alg = fmgr.createAlgorithm("ConvertUnits");
  alg->setPropertyValue("InputWorkspace", loadraw);
  target = "wavelength_GEM";
  alg->setPropertyValue("OutputWorkspace", target);
  alg->setPropertyValue("Target","Wavelength");
  alg->execute();

  long mem_step = memMan.getMemoryInfo().availMemory;
  std::cerr << "Converted units to wavelength. " << mem_step
	    << " KB of memory available\n";
  
  std::vector<std::string> wkspNames = AnalysisDataService::Instance().getObjectNames();
  std::cerr << "Currently there are " << wkspNames.size()  << " workspaces:\n";
  std::vector<std::string>::const_iterator sIter;
  for( sIter = wkspNames.begin(); sIter != wkspNames.end(); ++sIter)
  {
    std::cerr << "\t" << *sIter << "\n";
  }
  std::cerr << "After creating above workspaces, we have " 
	    << mem_step << " KB of memory available.\n";

  std::cerr << "Now removing GEM38370 and dspacing from service\n";
  
  //Remove w1 and w2 from framework
  fmgr.deleteWorkspace("GEM38370");
  fmgr.deleteWorkspace("dspacing_GEM");
  
  std::cerr << "Workspaces available:\n"; 
  wkspNames = AnalysisDataService::Instance().getObjectNames();
  for( sIter = wkspNames.begin(); sIter != wkspNames.end(); ++sIter)
  {
    std::cerr << "\t" << *sIter << "\n";
  }

  mem_step = memMan.getMemoryInfo().availMemory;

  std::cerr << "After removal we (apparently) have " << mem_step 
	    << " KB of memory available.\n";
  
  std::cerr << "Load some HET data into a new workspace\n";
  //Load some HET data
  alg = fmgr.createAlgorithm("LoadRaw");
  alg->setPropertyValue("Filename", "../../../../Test/Data/HET15869.RAW");
  loadraw = "HET15869";
  alg->setPropertyValue("OutputWorkspace", loadraw);    
  alg->execute();

  mem_step = memMan.getMemoryInfo().availMemory;
  std::cerr << "Memory available: "  << mem_step << " KB\n";

  std::cerr << "Convert units of new HET15869 workspace\n";
  alg = fmgr.createAlgorithm("ConvertUnits");
  alg->setPropertyValue("InputWorkspace", loadraw);
  target = "dspacing_HET";
  alg->setPropertyValue("OutputWorkspace", target);
  alg->setPropertyValue("Target","dSpacing");
  alg->execute();

  wkspNames = AnalysisDataService::Instance().getObjectNames();
  std::cerr << "Workspaces available:\n";
  for( sIter = wkspNames.begin(); sIter != wkspNames.end(); ++sIter)
  {
    std::cerr << "\t" << *sIter << "\n";
  }
  std::cerr << "Memory available: "  << mem_step << " KB\n";
  
  std::cerr << "\n";
}
