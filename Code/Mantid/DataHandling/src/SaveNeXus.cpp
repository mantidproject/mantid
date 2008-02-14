// SaveNeXus
// @author Freddie Akeroyd, STFC ISIS Faility
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveNeXus.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "NeXusUtils/NeXusUtils.h"

#include <cmath>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace DataHandling
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
    declareProperty("FileName","",new MandatoryValidator);
    declareProperty("EntryName","",new MandatoryValidator);
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
    m_inputWorkspace = getProperty("InputWorkspace");

    const std::string workspaceID = m_inputWorkspace->id();

    if (workspaceID == "Workspace1D")
    {
        const Workspace1D_sptr localworkspace = boost::dynamic_pointer_cast<Workspace1D>(m_inputWorkspace);
        const std::vector<double>& xValue = localworkspace->dataX();
        const std::vector<double>& yValue = localworkspace->dataY();
        const std::vector<double>& eValue = localworkspace->dataE();
	writeEntry1D(m_filename, m_entryname, xValue, yValue, eValue);
    }
    else if (workspaceID == "Workspace2D")
    {
        const Workspace2D_sptr localworkspace = boost::dynamic_pointer_cast<Workspace2D>(m_inputWorkspace);
        const int numberOfHist = localworkspace->getHistogramNumber();
	for(int i=0; i<numberOfHist; i++)
	{
            std::ostringstream oss;
	    oss << m_entryname << "_" << i << std::ends;
            const std::vector<double>& xValue = localworkspace->dataX(i);
            const std::vector<double>& yValue = localworkspace->dataY(i);
            const std::vector<double>& eValue = localworkspace->dataE(i);
	    writeEntry1D(m_filename, oss.str(), xValue, yValue, eValue);
	}
    }
    else
    {
        throw Exception::NotImplementedError("SaveNeXus passed invalid workspaces.");
    }

    return;
  }

} // namespace DataHandling
} // namespace Mantid
