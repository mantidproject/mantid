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
	
	
PythonInterface::PythonInterface()
{
}

PythonInterface::~PythonInterface()
{
}

void PythonInterface::InitialiseFrameworkManager()
{
	FrameworkManager::Instance().initialize();
}

bool PythonInterface::CreateAlgorithm(const std::string& algName)
{
	FrameworkManager::Instance().createAlgorithm(algName);
		
	return true;
}

bool PythonInterface::ExecuteAlgorithm(const std::string& algName, const std::string& properties)
{
	FrameworkManager::Instance().exec(algName, properties);
		
	return true;
}

int PythonInterface::LoadIsisRawFile(const std::string& fileName, const std::string& workspaceName)
{
	CreateAlgorithm("LoadRaw");
	
	std::string properties = "Filename:" + fileName + ",OutputWorkspace:" + workspaceName;
	
	ExecuteAlgorithm("LoadRaw", properties);
		
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	std::cout << output2D->getHistogramNumber() << std::endl;
	
	//Return the number of histograms
	return output2D->getHistogramNumber();
}

std::vector<double> PythonInterface::GetXData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return output2D->dataX(index);	
}

std::vector<double> PythonInterface::GetYData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return output2D->dataY(index);	
}

std::vector<double> PythonInterface::GetEData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return output2D->dataE(index);	
}

std::vector<double> PythonInterface::GetE2Data(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return output2D->dataE2(index);	
}

unsigned long PythonInterface::GetAddressXData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return reinterpret_cast<unsigned long>(&output2D->dataX(index)[0]);	
}

unsigned long PythonInterface::GetAddressYData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return reinterpret_cast<unsigned long>(&output2D->dataY(index)[0]);	
}


}
}

