#include <iostream>

#include "boost/algorithm/string.hpp"
#include "boost/pointer_cast.hpp"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"

#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidPythonAPI/PythonInterface.h"

using namespace Mantid::API;

namespace Mantid
{
namespace PythonAPI
{

///Constructor
PythonInterface::PythonInterface()
{
}

///Destructor
PythonInterface::~PythonInterface()
{
}

///Initialises the FrameworkManager.
void PythonInterface::InitialiseFrameworkManager()
{
	FrameworkManager::Instance().initialize();
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \return Pointer to algorithm.
 **/
IAlgorithm* PythonInterface::CreateAlgorithm(const std::string& algName)
{
	IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm(algName);

	return alg;
}

/**
 * Loads a standard ISIS raw file into Mantid, using the LoadRaw algorithm.
 * \param fileName :: The filepath of the raw file to be opened.
 * \param workspaceName :: The name under which the workspace is to be stored in Mantid.
 * \return Shared pointer to the workspace.
 **/
Workspace_sptr PythonInterface::LoadIsisRawFile(const std::string& fileName,
		const std::string& workspaceName)
{
	//Check workspace does not exist
	if (!AnalysisDataService::Instance().doesExist(workspaceName))
	{
		IAlgorithm* alg = CreateAlgorithm("LoadRaw");
		alg->setPropertyValue("Filename", fileName);
		alg->setPropertyValue("OutputWorkspace", workspaceName);

		alg->execute();

		Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);

		return output;
	}
	
	Workspace_sptr empty;
	
	return empty;
}

/**
 * Retrieves a workspace by name.
 * \param workspaceName :: The name under which the workspace is stored in Mantid.
 * \return Shared pointer to the workspace.
 **/
Workspace_sptr PythonInterface::RetrieveWorkspace(const std::string& workspaceName)
{
	//Check workspace does exist
	if (AnalysisDataService::Instance().doesExist(workspaceName))
	{
		Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);

		return output;
	}
	
	Workspace_sptr empty;
	
	return empty;
}

/**
 * Delete a workspace by name.
 * \param workspaceName :: The name under which the workspace is stored in Mantid.
 * \return Boolean result.
 **/
bool PythonInterface::DeleteWorkspace(const std::string& workspaceName)
{
	return FrameworkManager::Instance().deleteWorkspace(workspaceName);
}

/**
 * Returns the number of histograms in a workspace.
 * \param workspaceName :: The name under which the workspace is stored in Mantid.
 * \return int.
 **/
int PythonInterface::GetHistogramNumber(const std::string& workspaceName)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	//Mantid::DataObjects::Workspace2D_sptr output2D =
	//boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return output->getHistogramNumber();
}

/**
 * Returns the number of bins in a workspace.
 * \param workspaceName :: The name under which the workspace is stored in Mantid.
 * \return int.
 **/
int PythonInterface::GetBinNumber(const std::string& workspaceName)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	//Mantid::DataObjects::Workspace2D_sptr output2D =
	//boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return output->dataX(0).size();
}

/**
 * Returns the name of all the workspaces.
 * \return Vector of strings.
 **/
std::vector<std::string> PythonInterface::GetWorkspaceNames()
{
	return AnalysisDataService::Instance().getObjectNames();
}

/**
 * Returns the name of all the algorithms.
 * \return Vector of strings.
 **/
std::vector<std::string> PythonInterface::GetAlgorithmNames()
{
	return AlgorithmFactory::Instance().getKeys();
}

/**
 * Gives Python access to the X data of a specified spectra of a chosen workspace.
 * The pointer returned to Python may become invalid if the workspace is subsequently altered;
 * for example, if the workspace was closed in Mantid, the Python pointer would be left hanging.
 * \param workspaceName :: The name under which the workspace is stored in Mantid.
 * \param index :: The spectra number to return.
 * \return Pointer to a std::vector<double>.
 **/
std::vector<double>* PythonInterface::GetXData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	//Mantid::DataObjects::Workspace2D_sptr output2D =
	//boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);

	return &output->dataX(index);
}

/**
 * Gives Python access to the Y data of a specified spectra of a chosen workspace.
 * The pointer returned to Python may become invalid if the workspace is subsequently altered;
 * for example, if the workspace was closed in Mantid, the Python pointer would be left hanging.
 * \param workspaceName :: The name under which the workspace is stored in Mantid.
 * \param index :: The spectra number to return.
 * \return Pointer to a std::vector<double>.
 **/
std::vector<double>* PythonInterface::GetYData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	//Mantid::DataObjects::Workspace2D_sptr output2D =
	//boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);

	return &output->dataY(index);
}

/**
 * Gives Python access to the E data of a specified spectra of a chosen workspace.
 * The pointer returned to Python may become invalid if the workspace is subsequently altered;
 * for example, if the workspace was closed in Mantid, the Python pointer would be left hanging.
 * \param workspaceName :: The name under which the workspace is stored in Mantid.
 * \param index :: The spectra number to return.
 * \return Pointer to a std::vector<double>.
 **/
std::vector<double>* PythonInterface::GetEData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	//Mantid::DataObjects::Workspace2D_sptr output2D =
	//boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);

	return &output->dataE(index);
}

/**
 * Gives Python access to the E2 data of a specified spectra of a chosen workspace.
 * The pointer returned to Python may become invalid if the workspace is subsequently altered;
 * for example, if the workspace was closed in Mantid, the Python pointer would be left hanging.
 * \param workspaceName :: The name under which the workspace is stored in Mantid.
 * \param index :: The spectra number to return.
 * \return Pointer to a std::vector<double>.
 **/
std::vector<double>* PythonInterface::GetE2Data(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	//Mantid::DataObjects::Workspace2D_sptr output2D =
	//boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);

	return &output->dataE2(index);
}

/**
 * Gives Python the address of the X data of a specified spectra of a chosen workspace.
 * This is used for passing an address to the data from Mantid to a another programme
 * via Python. For example, it has been used with QtiPlot.
 * \param workspaceName :: The name under which the workspace is stored in Mantid.
 * \param index :: The spectra number to return.
 * \return Unsigned long containing the memory address
 **/
unsigned long PythonInterface::GetAddressXData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	//Mantid::DataObjects::Workspace2D_sptr output2D =
	//boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);

	return reinterpret_cast<unsigned long>(&output->dataX(index)[0]);
}

/**
 * Gives Python the address of the Y data of a specified spectra of a chosen workspace.
 * This is used for passing an address to the data from Mantid to a another programme
 * via Python. For example, it has been used with QtiPlot.
 * \param workspaceName :: The name under which the workspace is stored in Mantid.
 * \param index :: The spectra number to return.
 * \return Unsigned long containing the memory address
 **/
unsigned long PythonInterface::GetAddressYData(const std::string& workspaceName, int const index)
{
	//Retrieve workspace
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName);
	//Mantid::DataObjects::Workspace2D_sptr output2D =
	//boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);

	return reinterpret_cast<unsigned long>(&output->dataY(index)[0]);
}

}
}

