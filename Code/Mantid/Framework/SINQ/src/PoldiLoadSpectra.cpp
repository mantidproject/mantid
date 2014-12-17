//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/PoldiLoadSpectra.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <limits>
#include <cmath>
#include <boost/shared_ptr.hpp>

#include <iostream>

using namespace std;

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PoldiLoadSpectra)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;

///// Empty default constructor
// PoldiLoadChopperSlits::PoldiLoadChopperSlits() :
//	{}

/// Initialisation method.
void PoldiLoadSpectra::init() {

  // Data
  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(
                      "InputWorkspace", "", Direction::InOut),
                  "Input workspace containing the data to treat.");
  // Data

  declareProperty(new WorkspaceProperty<ITableWorkspace>("PoldiSpectra", "",
                                                         Direction::Output),
                  "The output Tableworkspace"
                  "with columns containing key summary information about the "
                  "Poldi spectra.");

  declareProperty("nbSpectraLoaded", 0, "nb of loaded chopper slits",
                  Direction::Output);
}

/** ***************************************************************** */

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::FileError If the Nexus file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid
 *values
 */
void PoldiLoadSpectra::exec() {

  ////////////////////////////////////////////////////////////////////////
  // About the workspace
  ////////////////////////////////////////////////////////////////////////

  DataObjects::Workspace2D_sptr localWorkspace =
      this->getProperty("InputWorkspace");

  ////////////////////////////////////////////////////////////////////////
  // Load the data into the workspace
  ////////////////////////////////////////////////////////////////////////

  // create table workspace

  try {
    ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable();

    outputws->addColumn("double", "lambda");
    outputws->addColumn("double", "intensity");

    boost::shared_ptr<const Mantid::Geometry::IComponent> comp =
        localWorkspace->getInstrument()->getComponentByName("sourceSp");
    boost::shared_ptr<const Mantid::Geometry::ICompAssembly> bank =
        boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(
            comp);
    if (bank) {
      // Get a vector of children (recursively)
      std::vector<boost::shared_ptr<const Mantid::Geometry::IComponent>>
          children;
      bank->getChildren(children, true);
      g_log.debug() << "_poldi : slits children.size()" << children.size()
                    << std::endl;

      int ewLine = 0;
      double lambda(0.0), intensity(0.0);

      for (unsigned int it = 0; it < children.size(); ++it) {
        string wireName = children.at(it)->getName();
        std::vector<boost::shared_ptr<const Mantid::Geometry::IComponent>>
            tyty = localWorkspace.get()
                       ->getInstrument()
                       .get()
                       ->getAllComponentsWithName(wireName);

        std::vector<double> tempWirea = tyty[0]->getNumberParameter("lambda");
        if (tempWirea.size() > 0) {
          lambda = tempWirea[0];
        }
        std::vector<double> tempWireb = tyty[0]->getNumberParameter("int");
        if (tempWireb.size() > 0) {
          intensity = tempWireb[0];
        }

        ewLine++;
        g_log.debug() << "_poldi : spectra " << ewLine << " lmbda  " << lambda
                      << " : " << intensity << std::endl;
        TableRow t = outputws->appendRow();
        t << lambda << intensity;
      }
      g_log.information() << "_poldi : spectra loaded (nb:" << ewLine << ")"
                          << std::endl;
      setProperty("nbSpectraLoaded", ewLine);
    } else {
      g_log.information() << "_poldi : no chopper slit loaded" << std::endl;
    }

    setProperty("PoldiSpectra", outputws);

  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    throw std::runtime_error("Error when saving the PoldiDeadWires Results "
                             "data to Workspace : NotFoundError");
  } catch (std::runtime_error &) {
    throw std::runtime_error("Error when saving the PoldiDeadWires Results "
                             "data to Workspace : runtime_error");
  }
}

} // namespace DataHandling
} // namespace Mantid
