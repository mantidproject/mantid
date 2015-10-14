//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/PoldiAutoCorrelation5.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "MantidSINQ/PoldiUtilities/PoldiDeadWireDecorator.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Poldi {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PoldiAutoCorrelation5)

using namespace Kernel;
using namespace API;
using namespace PhysicalConstants;
using namespace Geometry;

/// Initialisation method.
void PoldiAutoCorrelation5::init() {

  // Input workspace containing the raw data.
  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(
                      "InputWorkspace", "", Direction::InOut),
                  "Input workspace containing raw POLDI data.");

  // the minimal value of the wavelength to consider
  declareProperty("wlenmin", 1.1, "Minimum wavelength considered",
                  Direction::Input);
  // the maximal value of the wavelength to consider
  declareProperty("wlenmax", 5.0, "Maximum wavelength considered",
                  Direction::Input);

  // The output Workspace2D containing the Poldi data autocorrelation function.
  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output workspace containing the correlation spectrum.");

  /* Auto correlation core object which performs the actual calculation.
   * In future versions this will be replaced by a factory to cater for
   * slightly different variants of the algorithm as they are implemented
   * in the original fortran analysis software.
   */
  m_core = boost::shared_ptr<PoldiAutoCorrelationCore>(
      new PoldiAutoCorrelationCore(g_log));
}

/** ***************************************************************** */

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::NotFoundError Error when saving the PoldiDeadWires Results
 *data to Workspace
 *  @throw std::runtime_error Error when saving the PoldiDeadWires Results data
 *to Workspace
 */
void PoldiAutoCorrelation5::exec() {
  g_log.information() << "_Poldi  start conf --------------  " << std::endl;

  /* From localWorkspace three things are used:
   *      - Count data from POLDI experiment
   *      - POLDI instrument definition
   *      - Some data from the "Log" (for example chopper-speed)
   */
  DataObjects::Workspace2D_sptr localWorkspace =
      this->getProperty("InputWorkspace");

  g_log.information() << "_Poldi ws loaded --------------  " << std::endl;

  double wlen_min = this->getProperty("wlenmin");
  double wlen_max = this->getProperty("wlenmax");

  PoldiInstrumentAdapter instrumentAdapter(localWorkspace);
  PoldiAbstractChopper_sptr chopper = instrumentAdapter.chopper();

  PoldiAbstractDetector_sptr detector = instrumentAdapter.detector();
  boost::shared_ptr<PoldiDeadWireDecorator> cleanDetector(
      new PoldiDeadWireDecorator(localWorkspace->getInstrument(), detector));

  // log configuration information
  logConfigurationInformation(cleanDetector, chopper);

  // putting together POLDI instrument for calculations
  m_core->setInstrument(cleanDetector, chopper);
  m_core->setWavelengthRange(wlen_min, wlen_max);

  try {
    Mantid::DataObjects::Workspace2D_sptr outputws =
        m_core->calculate(localWorkspace);

    setProperty("OutputWorkspace",
                boost::dynamic_pointer_cast<Workspace>(outputws));

  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    throw std::runtime_error("Error when saving the PoldiIPP Results data to "
                             "Workspace : NotFoundError");
  } catch (std::runtime_error &) {
    throw std::runtime_error("Error when saving the PoldiIPP Results data to "
                             "Workspace : runtime_error");
  }
}

void PoldiAutoCorrelation5::logConfigurationInformation(
    boost::shared_ptr<PoldiDeadWireDecorator> cleanDetector,
    PoldiAbstractChopper_sptr chopper) {
  if (cleanDetector && chopper) {
    g_log.information()
        << "____________________________________________________ " << std::endl;
    g_log.information()
        << "_Poldi  chopper conf ------------------------------  " << std::endl;
    g_log.information() << "_Poldi -     Chopper speed:   "
                        << chopper->rotationSpeed() << " rpm" << std::endl;
    g_log.information() << "_Poldi -     Number of slits: "
                        << chopper->slitPositions().size() << std::endl;
    g_log.information() << "_Poldi -     Cycle time:      "
                        << chopper->cycleTime() << " µs" << std::endl;
    g_log.information() << "_Poldi -     Zero offset:     "
                        << chopper->zeroOffset() << " µs" << std::endl;
    g_log.information() << "_Poldi -     Distance:        "
                        << chopper->distanceFromSample() << " mm" << std::endl;

    if (g_log.is(Poco::Message::PRIO_DEBUG)) {
      for (size_t i = 0; i < chopper->slitPositions().size(); ++i) {
        g_log.information() << "_Poldi -     Slits: " << i
                            << ": Position = " << chopper->slitPositions()[i]
                            << "\t Time = " << chopper->slitTimes()[i] << " µs"
                            << std::endl;
      }
    }

    g_log.information()
        << "_Poldi  detector conf ------------------------------  "
        << std::endl;
    g_log.information() << "_Poldi -     Element count:     "
                        << cleanDetector->elementCount() << std::endl;
    g_log.information() << "_Poldi -     Central element:   "
                        << cleanDetector->centralElement() << std::endl;
    g_log.information() << "_Poldi -     2Theta(central):   "
                        << cleanDetector->twoTheta(199) / M_PI * 180.0 << "°"
                        << std::endl;
    g_log.information() << "_Poldi -     Distance(central): "
                        << cleanDetector->distanceFromSample(199) << " mm"
                        << std::endl;

    std::set<int> deadWires = cleanDetector->deadWires();
    g_log.information() << "_Poldi -     Number of dead wires: "
                        << deadWires.size() << std::endl;
    g_log.information() << "_Poldi -     Wire indices: ";
    for (std::set<int>::const_iterator dw = deadWires.begin();
         dw != deadWires.end(); ++dw) {
      g_log.information() << *dw << " ";
    }
    g_log.information() << std::endl;
  }
}

} // namespace Poldi
} // namespace Mantid
