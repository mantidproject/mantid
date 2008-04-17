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
 * \return Boolean.
 **/
bool PythonInterface::CreateAlgorithm(const std::string& algName)
{
	FrameworkManager::Instance().createAlgorithm(algName);

	return true;
}

/**
 * Executes a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param properties :: The properties for the algorithm (as std::string).
 * \return Boolean.
 **/
bool PythonInterface::ExecuteAlgorithm(const std::string& algName,
		const std::string& properties)
{
	FrameworkManager::Instance().exec(algName, properties);

	return true;
}

/**
 * Loads a standard ISIS raw file into Mantid, using the LoadRaw algorithm.
 * \param fileName :: The filepath of the raw file to be opened.
 * \param workspaceName :: The name under which the workspace is to be stored in Mantid.
 * \return Number of spectra.
 **/
int PythonInterface::LoadIsisRawFile(const std::string& fileName,
		const std::string& workspaceName)
{
	//Check workspace does not exist
	if (!AnalysisDataService::Instance().doesWorkspaceExist(workspaceName))
	{
		CreateAlgorithm("LoadRaw");

		std::string properties = "Filename:" + fileName + ",OutputWorkspace:"
			+ workspaceName;

		ExecuteAlgorithm("LoadRaw", properties);

		//Return the number of histograms
		return GetHistogramNumber(workspaceName);
	}
	
	return -1;
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
	Mantid::DataObjects::Workspace2D_sptr output2D =
	boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return output2D->getHistogramNumber();
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
	Mantid::DataObjects::Workspace2D_sptr output2D =
	boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);
	
	return output2D->dataX(0).size();
}

/**
 * Returns the name of all the workspaces.
 * \return Vector of strings.
 **/
std::vector<std::string> PythonInterface::GetWorkspaceNames()
{
	return AnalysisDataService::Instance().getWorkspaceNames();
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
	Mantid::DataObjects::Workspace2D_sptr output2D =
	boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);

	return &output2D->dataX(index);
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
	Mantid::DataObjects::Workspace2D_sptr output2D =
	boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);

	return &output2D->dataY(index);
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
	Mantid::DataObjects::Workspace2D_sptr output2D =
	boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);

	return &output2D->dataE(index);
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
	Mantid::DataObjects::Workspace2D_sptr output2D =
	boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);

	return &output2D->dataE2(index);
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
	Mantid::DataObjects::Workspace2D_sptr output2D =
	boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);

	return reinterpret_cast<unsigned long>(&output2D->dataX(index)[0]);
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
	Mantid::DataObjects::Workspace2D_sptr output2D =
	boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(output);

	return reinterpret_cast<unsigned long>(&output2D->dataY(index)[0]);
}

}
}

