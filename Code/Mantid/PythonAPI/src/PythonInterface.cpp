#include <iostream>

#include "boost/algorithm/string.hpp"
#include "boost/pointer_cast.hpp"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"

#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidPythonAPI/PythonInterface.h"

using namespace Mantid::API;

namespace Mantid
{
namespace PythonAPI
{	
	
	
PythonInterface::PythonInterface() : fwMgr(0)
{
}

PythonInterface::~PythonInterface()
{
}

void PythonInterface::InitialiseFrameworkManager()
{
		fwMgr = FrameworkManager::Instance();
		fwMgr->initialize();
}

bool PythonInterface::CreateAlgorithm(const std::string& algName)
{
		 fwMgr->createAlgorithm(algName);
		
		return true;
}

bool PythonInterface::ExecuteAlgorithm(const std::string& algName, const std::string& properties)
{
		fwMgr->exec(algName, properties);
		
		return true;
}

bool PythonInterface::LoadNexusFile(const std::string& fileName, const std::string& workspaceName)
{
	return false;
}

int PythonInterface::LoadIsisRawFile(const std::string& fileName, const std::string& workspaceName)
{
	fwMgr->createAlgorithm("LoadRaw");
	
	std::string properties = "Filename:" + fileName + ",OutputWorkspace:" + workspaceName;
	
	fwMgr->exec("LoadRaw", properties);
	
	//Retrieve workspace
	AnalysisDataService *ads = AnalysisDataService::Instance();
	
	Workspace_sptr output = ads->retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	std::cout << output2D->getHistogramNumber() << std::endl;
	
	//Return the number of histograms
	return output2D->getHistogramNumber();
}

std::vector<double> PythonInterface::GetXData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	AnalysisDataService *ads = AnalysisDataService::Instance();
	
	Workspace_sptr output = ads->retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return output2D->dataX(index);	
}

std::vector<double> PythonInterface::GetYData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	AnalysisDataService *ads = AnalysisDataService::Instance();
	
	Workspace_sptr output = ads->retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return output2D->dataY(index);	
}

unsigned long PythonInterface::GetAddressXData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	AnalysisDataService *ads = AnalysisDataService::Instance();
	
	Workspace_sptr output = ads->retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return reinterpret_cast<unsigned long>(&output2D->dataX(index)[0]);	
}

unsigned long PythonInterface::GetAddressYData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	AnalysisDataService *ads = AnalysisDataService::Instance();
	
	Workspace_sptr output = ads->retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return reinterpret_cast<unsigned long>(&output2D->dataY(index)[0]);	
}


}
}

