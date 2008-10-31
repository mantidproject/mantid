// SaveNeXus
// @author Freddie Akeroyd, STFC ISIS Faility
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/SaveNeXus.h"
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
  DECLARE_ALGORITHM(SaveNeXus)

  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  Logger& SaveNeXus::g_log = Logger::get("SaveNeXus");

  /// Empty default constructor
  SaveNeXus::SaveNeXus()
  {
  }

  /** Initialisation method.
   *
   */
  void SaveNeXus::init()
  {
    declareProperty("FileName","",new MandatoryValidator<std::string>);
    declareProperty("EntryName","",new MandatoryValidator<std::string>);
    declareProperty("DataName","",new MandatoryValidator<std::string>);
    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input));
  }

  /** Executes the algorithm. Reading in the file and creating and populating
   *  the output workspace
   *
   *  @throw runtime_error Thrown if algorithm cannot execute
   */
  void SaveNeXus::exec()
  {
    // Retrieve the filename from the properties
    m_filename = getPropertyValue("FileName");
    m_entryname = getPropertyValue("EntryName");
	m_dataname = getPropertyValue("DataName");
    m_inputWorkspace = getProperty("InputWorkspace");

    const std::string workspaceID = m_inputWorkspace->id();
    NeXusUtils *nexusFile= new NeXusUtils();
    if (workspaceID == "Workspace1D")
    {
        const Workspace1D_sptr localworkspace = boost::dynamic_pointer_cast<Workspace1D>(m_inputWorkspace);
        const std::vector<double>& xValue = localworkspace->dataX();
        const std::vector<double>& yValue = localworkspace->dataY();
        const std::vector<double>& eValue = localworkspace->dataE();
	    nexusFile->writeEntry1D(m_filename, m_entryname, m_dataname, xValue, yValue, eValue);
    }
    else if (workspaceID == "Workspace2D")
    {
        const Workspace2D_sptr localworkspace = boost::dynamic_pointer_cast<Workspace2D>(m_inputWorkspace);
        const int numberOfHist = localworkspace->getNumberHistograms();
	    for(int i=0; i<numberOfHist; i++)
	    {
            std::ostringstream oss;
	        oss << m_dataname << "_" << i << std::ends;
            const std::vector<double>& xValue = localworkspace->dataX(i);
            const std::vector<double>& yValue = localworkspace->dataY(i);
            const std::vector<double>& eValue = localworkspace->dataE(i);
	        nexusFile->writeEntry1D(m_filename, m_entryname, oss.str(), xValue, yValue, eValue);
	    }
    }
    else
    {
        throw Exception::NotImplementedError("SaveNeXus passed invalid workspaces.");
    }

    return;
  }

} // namespace NeXus
} // namespace Mantid
