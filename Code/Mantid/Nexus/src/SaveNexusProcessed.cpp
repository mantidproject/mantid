// SaveNexusProcessed
// @author Ronald Fowler, based on SaveNexus
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/SaveNexusProcessed.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NeXusUtils.h"

#include <cmath>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace NeXus
{

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(SaveNexusProcessed)

  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  Logger& SaveNexusProcessed::g_log = Logger::get("SaveNexusProcessed");

  /// Empty default constructor
  SaveNexusProcessed::SaveNexusProcessed()
  {
  }

  /** Initialisation method.
   *
   */
  void SaveNexusProcessed::init()
  {
    // Declare required input parameters for algorithm
    std::vector<std::string> exts;
    exts.push_back("NXS");
    exts.push_back("nxs");
    exts.push_back("nx5");
    exts.push_back("NX5");
    //declareProperty("FileName","",new FileValidator(exts));
    declareProperty("FileName","",new MandatoryValidator<std::string>);
    declareProperty("EntryName","",new MandatoryValidator<std::string>);
    declareProperty("Title","",new MandatoryValidator<std::string>);
    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input));
  }

  /** Executes the algorithm. Reading in the file and creating and populating
   *  the output workspace
   *
   *  @throw runtime_error Thrown if algorithm cannot execute
   */
  void SaveNexusProcessed::exec()
  {
    // Retrieve the filename from the properties
    m_filename = getPropertyValue("FileName");
    m_entryname = getPropertyValue("EntryName");
    m_title = getPropertyValue("Title");
    m_inputWorkspace = getProperty("InputWorkspace");

    if( writeNexusProcessedHeader( m_filename, m_entryname, m_title ) != 0 )
    {
       g_log.error("Failed to write file");
       throw Exception::FileError("Failed to write to file", m_filename);
    }

    boost::shared_ptr<Mantid::API::Sample> sample=m_inputWorkspace->getSample();
    if( writeNexusProcessedSample( m_filename, m_entryname, sample->getName(), sample) != 0 )

    {
       g_log.error("Failed to write NXsample");
       throw Exception::FileError("Failed to write NXsample", m_filename);
    }


    const std::string workspaceID = m_inputWorkspace->id();

    if (workspaceID == "Workspace1D")
    {
        const Workspace1D_sptr localworkspace = boost::dynamic_pointer_cast<Workspace1D>(m_inputWorkspace);
        const std::vector<double>& xValue = localworkspace->dataX();
        const std::vector<double>& yValue = localworkspace->dataY();
        const std::vector<double>& eValue = localworkspace->dataE();
//        writeEntry1D(m_filename, m_entryname, m_dataname, xValue, yValue, eValue);
    }
    else if (workspaceID == "Workspace2D")
    {
        const Workspace2D_sptr localworkspace = boost::dynamic_pointer_cast<Workspace2D>(m_inputWorkspace);
        const int numberOfHist = localworkspace->getNumberHistograms();
		// check if all X() are in fact the same array
		bool uniformSpectra=true;
        for(int i=1; i<numberOfHist; i++)
        {
			if( localworkspace->dataX(i) != localworkspace->dataX(1) )
			{
				uniformSpectra=false;
				break;
			}
        }
		writeNexusProcessedData(m_filename,m_entryname,localworkspace,uniformSpectra);
            //std::ostringstream oss;
            //oss << m_dataname << "_" << i << std::ends;
            //const std::vector<double>& yValue = localworkspace->dataY(i);
            //const std::vector<double>& eValue = localworkspace->dataE(i);
            //writeEntry1D(m_filename, m_entryname, oss.str(), xValue, yValue, eValue);
    }
    else
    {
        throw Exception::NotImplementedError("SaveNexusProcessed passed invalid workspaces.");
    }

    return;
  }

} // namespace NeXus
} // namespace Mantid
