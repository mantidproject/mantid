/*WIKI*


== How to use algorithm with other algorithms ==
This algorithm is designed to work with other algorithms to
proceed POLDI data. The introductions can be found in the
wiki page of [[PoldiProjectRun]].


 *WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/PoldiLoadChopperSlits.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <boost/shared_ptr.hpp>



using namespace std;


namespace Mantid
{
namespace Poldi
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PoldiLoadChopperSlits)

// Sets documentation strings for this algorithm
void PoldiLoadChopperSlits::initDocs()
{
	this->setWikiSummary("Load Poldi chopper slits data file. ");
	this->setOptionalMessage("Load Poldi chopper slits data file.");
}


using namespace Kernel;
using namespace API;
using Geometry::Instrument;


/// Initialisation method.
void PoldiLoadChopperSlits::init()
{

	// Data
	declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("InputWorkspace", "", Direction::InOut),
			"Input workspace containing the data to treat.");
	// Data

	declareProperty(new WorkspaceProperty<ITableWorkspace>("PoldiChopperSlits","",Direction::Output),
			"The output Tableworkspace"
			"with columns containing key summary information about the PoldiDeadWires.");


	declareProperty("nbLoadedSlits", 0, "nb of loaded chopper slits", Direction::Output);



}

/** ***************************************************************** */



/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::FileError If the Nexus file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid values
 */
void PoldiLoadChopperSlits::exec()
{



	////////////////////////////////////////////////////////////////////////
	// About the workspace
	////////////////////////////////////////////////////////////////////////

	DataObjects::Workspace2D_sptr localWorkspace = this->getProperty("InputWorkspace");

	////////////////////////////////////////////////////////////////////////
	// Load the data into the workspace
	////////////////////////////////////////////////////////////////////////


	//create table workspace


	try
	{
		ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable();

		outputws->addColumn("int","slits");
		outputws->addColumn("double","position");

		boost::shared_ptr<const Mantid::Geometry::IComponent> comp = localWorkspace->getInstrument()->getComponentByName("chopper");
		boost::shared_ptr<const Mantid::Geometry::ICompAssembly> bank = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(comp);
		if (bank)
		{
			// Get a vector of children (recursively)
			std::vector<boost::shared_ptr<const Mantid::Geometry::IComponent> > children;
			bank->getChildren(children, true);
			g_log.debug() << "_poldi : slits children.size()" <<  children.size() <<  std::endl;

			int ewLine = 0;

			for (unsigned int it = 0; it < children.size(); ++it)
			{
				string wireName = children.at(it)->getName();
				std::vector<boost::shared_ptr<const Mantid::Geometry::IComponent> > tyty =
						localWorkspace.get()->getInstrument().get()->getAllComponentsWithName(wireName);

				std::vector<double> tempWire = tyty[0]->getNumberParameter("slitPos");
				if(tempWire.size()>0){
					double val = tempWire[0];
					ewLine++;
					g_log.debug() << "_poldi : slits " <<  ewLine << " at  "<< val <<  std::endl;


					TableRow t = outputws->appendRow();
					t << ewLine << val ;

				}
			}
			g_log.information() << "_poldi : chopper slits loaded (nb:" << ewLine << ")" <<  std::endl;
			setProperty("nbLoadedSlits",ewLine);
		}else{
			g_log.information() << "_poldi : no chopper slit loaded" <<  std::endl;
		}

		setProperty("PoldiChopperSlits",outputws);

	}
	catch(Mantid::Kernel::Exception::NotFoundError& )
	{
		throw std::runtime_error("Error when saving the PoldiDeadWires Results data to Workspace : NotFoundError");
	}
	catch(std::runtime_error &)
	{
		throw std::runtime_error("Error when saving the PoldiDeadWires Results data to Workspace : runtime_error");
	}


}





} // namespace DataHandling
} // namespace Mantid
