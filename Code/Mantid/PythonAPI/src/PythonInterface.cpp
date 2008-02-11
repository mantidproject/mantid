#include "boost/algorithm/string.hpp"
#include "boost/pointer_cast.hpp"

#include "MantidPythonAPI/PythonInterface.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadRaw.h"

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
	if (!fwMgr)
	{
		fwMgr = FrameworkManager::Instance();
		fwMgr->initialize();
	}
}

bool PythonInterface::CreateAlgorithm(const std::string& algName)
{
	if (fwMgr)
	{
		std::string algNameLower = boost::algorithm::to_lower_copy(algName);
		
		if (algs.find(algNameLower) == algs.end())
		{
			IAlgorithm* temp = fwMgr->createAlgorithm(algName);
			boost::shared_ptr<IAlgorithm> pAlg(temp); 
			algs.insert( std::pair< std::string, boost::shared_ptr<IAlgorithm> >(algNameLower, pAlg) ); 
		}
		
		return true;
	}
	
	return false;
}

bool PythonInterface::ExecuteAlgorithm(const std::string& algName)
{
	if (fwMgr)
	{
		std::string algNameLower = boost::algorithm::to_lower_copy(algName);
		
		algMap::iterator val = algs.find(algNameLower) ;
		
		if (val != algs.end())
		{
			val->second->execute();
			
			return true;
		}
	}
	
	return false;
}

int PythonInterface::LoadIsisRawFile(const std::string& fileName, const std::string& workspaceName)
{
	//Load the data into a workspace
	Mantid::DataHandling::LoadRaw loader;
	loader.initialize();
	
	loader.setPropertyValue("Filename", fileName);
	loader.setPropertyValue("OutputWorkspace",  workspaceName);
	
	loader.execute();
	
	//Retrieve workspace
	AnalysisDataService *ads = AnalysisDataService::Instance();
	
	Workspace_sptr output = ads->retrieve(workspaceName);
	Mantid::DataObjects::Workspace2D_sptr output2D = 
		boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
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


}
}

