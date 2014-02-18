/*WIKI*


== How to use algorithm with other algorithms ==
This algorithm is designed to work with other algorithms to
proceed POLDI data. The introductions can be found in the
wiki page of [[PoldiProjectRun]].


 *WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/PoldiAutoCorrelation5.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "MantidSINQ/PoldiDetectorFactory.h"
#include "MantidSINQ/PoldiDeadWireDecorator.h"
#include "MantidSINQ/PoldiAutoCorrelationCore.h"
#include "MantidSINQ/PoldiChopperFactory.h"

#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace Poldi
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PoldiAutoCorrelation5)

/// Sets documentation strings for this algorithm
void PoldiAutoCorrelation5::initDocs()
{
	this->setWikiSummary("Proceed to autocorrelation on Poldi data.");
	this->setOptionalMessage("Proceed to autocorrelation on Poldi data.");
}


using namespace Kernel;
using namespace API;
using namespace PhysicalConstants;

/// Initialisation method.
void PoldiAutoCorrelation5::init()
{

	// Input workspace containing the raw data.
	declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("InputWorkspace", "", Direction::InOut),
			"Input workspace containing the raw data.");
	// Input workspace containing the log data.
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiSampleLogs", "PoldiSampleLogs", Direction::InOut),
			"Input workspace containing the log data.");
	// Input workspace containing the dead wires data.
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiDeadWires", "PoldiDeadWires", Direction::InOut),
			"Input workspace containing the dead wires data.");
	// Input workspace containing the choppers' slits data.
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiChopperSlits", "PoldiChopperSlits", Direction::InOut),
			"Input workspace containing the choppers' slits data.");
	// Input workspace containing the Poldi caracteristic spectra.
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiSpectra", "PoldiSpectra", Direction::InOut),
			"Input workspace containing the Poldi caracteristic spectra.");
	// Input workspace containing the Poldi setup data.
	declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("PoldiIPP", "PoldiIPP", Direction::InOut),
			"Input workspace containing the Poldi setup data.");

	// the minimal value of the wavelength to consider
	declareProperty("wlenmin", 1.1, "minimal wavelength considered" , Direction::Input);
	// the maximal value of the wavelength to consider
	declareProperty("wlenmax", 5.0, "maximal wavelength considered" , Direction::Input);

	// The output Workspace2D containing the Poldi data autocorrelation function.
	declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output),
			"The output Workspace2D"
			"containing the Poldi data autocorrelation function."
			"Index 1 and 2 ws will be used later by the peak detection algorithm.");



    m_core = boost::shared_ptr<PoldiAutoCorrelationCore>(new PoldiAutoCorrelationCore);

}

/** ***************************************************************** */



/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::NotFoundError Error when saving the PoldiDeadWires Results data to Workspace
 *  @throw std::runtime_error Error when saving the PoldiDeadWires Results data to Workspace
 */
void PoldiAutoCorrelation5::exec()
{

	g_log.information() << "_Poldi  start conf --------------  "  << std::endl;

    // Loading workspaces containing configuration and meta-data
    DataObjects::Workspace2D_sptr localWorkspace = this->getProperty("InputWorkspace");
	DataObjects::TableWorkspace_sptr ws_sample_logs = this->getProperty("PoldiSampleLogs");
	DataObjects::TableWorkspace_sptr ws_poldi_chopper_slits = this->getProperty("PoldiChopperSlits");
	DataObjects::TableWorkspace_sptr ws_poldi_dead_wires = this->getProperty("PoldiDeadWires");
	DataObjects::TableWorkspace_sptr ws_poldi_IPP = this->getProperty("PoldiIPP");


	g_log.information() << "_Poldi ws loaded --------------  "  << std::endl;

	double wlen_min = this->getProperty("wlenmin");
	double wlen_max = this->getProperty("wlenmax");

	// Chopper configuration
    PoldiChopperFactory chopperFactory;
    boost::shared_ptr<PoldiAbstractChopper> chopper(chopperFactory.createChopper(std::string("default-chopper")));
    chopper->loadConfiguration(ws_poldi_IPP, ws_poldi_chopper_slits, ws_sample_logs);

	g_log.information() << "____________________________________________________ "  << std::endl;
	g_log.information() << "_Poldi  chopper conf ------------------------------  "  << std::endl;
    g_log.information() << "_Poldi -     Chopper speed:   " << chopper->rotationSpeed() << " rpm" << std::endl;
    g_log.information() << "_Poldi -     Number of slits: " << chopper->slitPositions().size() << std::endl;
    g_log.information() << "_Poldi -     Cycle time:      " << chopper->cycleTime() << " µs" << std::endl;
    g_log.information() << "_Poldi -     Conf(t0):        " << chopper->zeroOffset() << " µs" << std::endl;
    g_log.information() << "_Poldi -     Distance:        " << chopper->distanceFromSample()  << " mm" << std::endl;


    if(g_log.is(Poco::Message::PRIO_DEBUG)) {
        for(size_t i = 0; i < chopper->slitPositions().size(); ++i) {
            g_log.debug()   << "_Poldi -     Slits: " << i
                            << ": Position = " << chopper->slitPositions()[i]
                               << "\t Time = " << chopper->slitTimes()[i] << " µs" << std::endl;
        }
    }

	// Detector configuration
    PoldiDetectorFactory detectorFactory;
    boost::shared_ptr<PoldiAbstractDetector> detector(detectorFactory.createDetector(std::string("helium3-detector")));
    detector->loadConfiguration(ws_poldi_IPP);

    // Removing dead wires with decorator
    std::vector<int> deadWireVector = ws_poldi_dead_wires->getColVector<int>(std::string("DeadWires"));
    std::set<int> deadWireSet(deadWireVector.begin(), deadWireVector.end());

    boost::shared_ptr<PoldiDeadWireDecorator> cleanDetector(new PoldiDeadWireDecorator(deadWireSet, detector));

    // putting together POLDI instrument for calculations
    m_core->setInstrument(cleanDetector, chopper);
    m_core->setWavelengthRange(wlen_min, wlen_max);

	try
	{
        Mantid::DataObjects::Workspace2D_sptr outputws = m_core->calculate(localWorkspace);

		setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(outputws));

	}
	catch(Mantid::Kernel::Exception::NotFoundError& )
	{
		throw std::runtime_error("Error when saving the PoldiIPP Results data to Workspace : NotFoundError");
	}
	catch(std::runtime_error &)
	{
		throw std::runtime_error("Error when saving the PoldiIPP Results data to Workspace : runtime_error");
	}
}

} // namespace Poldi
} // namespace Mantid
