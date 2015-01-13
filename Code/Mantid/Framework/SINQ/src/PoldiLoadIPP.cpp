//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/PoldiLoadIPP.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FileProperty.h"
#include <boost/shared_ptr.hpp>

using namespace std;

namespace Mantid {
namespace Poldi {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PoldiLoadIPP)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;
// using namespace::NeXus;
// using namespace NeXus;

///// Empty default constructor
// PoldiLoadChopperSlits::PoldiLoadChopperSlits() :
//	{}

/// Initialisation method.
void PoldiLoadIPP::init() {

  // Data
  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(
                      "InputWorkspace", "", Direction::InOut),
                  "Input workspace containing the data to treat.");
  // Data
  declareProperty(
      new WorkspaceProperty<ITableWorkspace>("PoldiIPP", "", Direction::Output),
      "The output Tableworkspace"
      "with columns containing key summary information about the Poldi "
      "spectra.");
}

/** ***************************************************************** */

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::FileError If the Nexus file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid
 *values
 */
void PoldiLoadIPP::exec() {

  ////////////////////////////////////////////////////////////////////////
  // About the workspace
  ////////////////////////////////////////////////////////////////////////

  DataObjects::Workspace2D_sptr localWorkspace =
      this->getProperty("InputWorkspace");

  ////////////////////////////////////////////////////////////////////////
  // Load the data into the workspace
  ////////////////////////////////////////////////////////////////////////

  try {
    ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable();

    outputws->addColumn("str", "param");
    outputws->addColumn("str", "unit");
    outputws->addColumn("double", "value");

    Geometry::Instrument_const_sptr inst = localWorkspace->getInstrument();

    double distChopSampl = localWorkspace->getInstrument()->getNumberParameter(
        "dist-chopper-sample")[0];
    g_log.debug() << "_poldi : param "
                  << "dist-chopper-sample"
                  << " : " << distChopSampl << std::endl;
    TableRow t0 = outputws->appendRow();
    t0 << "dist-chopper-sample"
       << "[mm]" << distChopSampl;

    double distSamplDet = localWorkspace->getInstrument()->getNumberParameter(
        "dist-sample-detector")[0];
    g_log.debug() << "_poldi : param "
                  << "dist-sample-detector"
                  << " : " << distSamplDet << std::endl;
    TableRow t1 = outputws->appendRow();
    t1 << "dist-sample-detector"
       << "[mm]" << distSamplDet;

    double x0det =
        localWorkspace->getInstrument()->getNumberParameter("x0det")[0];
    g_log.debug() << "_poldi : param "
                  << "x0det"
                  << " : " << x0det << std::endl;
    TableRow t2 = outputws->appendRow();
    t2 << "x0det"
       << "[mm]" << x0det;

    double y0det =
        localWorkspace->getInstrument()->getNumberParameter("y0det")[0];
    g_log.debug() << "_poldi : param "
                  << "y0det"
                  << " : " << y0det << std::endl;
    TableRow t3 = outputws->appendRow();
    t3 << "y0det"
       << "[mm]" << y0det;

    double twotheta =
        localWorkspace->getInstrument()->getNumberParameter("twothet")[0];
    g_log.debug() << "_poldi : param "
                  << "twothet"
                  << " : " << twotheta << std::endl;
    TableRow t4 = outputws->appendRow();
    t4 << "twothet"
       << "[deg]" << twotheta;

    double tps0 = localWorkspace->getInstrument()->getNumberParameter("t0")[0];
    g_log.debug() << "_poldi : param "
                  << "t0"
                  << " : " << tps0 << std::endl;
    TableRow t5 = outputws->appendRow();
    t5 << "t0"
       << "[mysec]" << tps0;

    double tcycle =
        localWorkspace->getInstrument()->getNumberParameter("tconst")[0];
    g_log.debug() << "_poldi : param "
                  << "tconst"
                  << " : " << tcycle << std::endl;
    TableRow t6 = outputws->appendRow();
    t6 << "tconst"
       << "[mysec]" << tcycle;

    double det_radius =
        localWorkspace->getInstrument()->getNumberParameter("det_radius")[0];
    g_log.debug() << "_poldi : param "
                  << "det_radius"
                  << " : " << det_radius << std::endl;
    TableRow t8 = outputws->appendRow();
    t8 << "det_radius"
       << "[mm]" << det_radius;

    double det_nb_channel = localWorkspace->getInstrument()->getNumberParameter(
        "det_nb_channel")[0];
    g_log.debug() << "_poldi : param "
                  << "det_nb_channel"
                  << " : " << det_nb_channel << std::endl;
    TableRow t9 = outputws->appendRow();
    t9 << "det_nb_channel"
       << "[]" << det_nb_channel;

    double det_channel_resolution =
        localWorkspace->getInstrument()->getNumberParameter(
            "det_channel_resolution")[0];
    g_log.debug() << "_poldi : param "
                  << "det_channel_resolution"
                  << " : " << det_channel_resolution << std::endl;
    TableRow t10 = outputws->appendRow();
    t10 << "det_channel_resolution"
        << "[mm]" << det_channel_resolution;

    setProperty("PoldiIPP", outputws);

  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    throw std::runtime_error("Error when saving the PoldiIPP Results data to "
                             "Workspace : NotFoundError");
  } catch (std::runtime_error &) {
    throw std::runtime_error("Error when saving the PoldiIPP Results data to "
                             "Workspace : runtime_error");
  }
}

} // namespace DataHandling
} // namespace Mantid
