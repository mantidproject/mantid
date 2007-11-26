//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Instrument.h"

#include "Detector.h"
#include "CompAssembly.h"
#include "Component.h"

#include <string>
#include <sstream>
#include <fstream>


namespace Mantid
{
namespace DataHandling
{

	DECLARE_ALGORITHM(LoadInstrument)

	using namespace Kernel;
	using DataObjects::Workspace2D;

	Logger& LoadInstrument::g_log = Logger::get("LoadInstrument");

	/// Empty default constructor
	LoadInstrument::LoadInstrument() { }


	/** Initialisation method. Does nothing at present.
	* 
	*  @return A StatusCode object indicating whether the operation was successful
	*/
	StatusCode LoadInstrument::init()
	{
		declareProperty("Filename","");

		return StatusCode::SUCCESS;
	}

	/** Executes the algorithm. Reading in the file and creating and populating
	*  the output workspace
	* 
	*  @return A StatusCode object indicating whether the operation was successful
	*/
	StatusCode LoadInstrument::exec()
	{
		
	// Retrieve the filename from the properties
	try
	{
		m_filename = getPropertyValue("Filename");
	}
	catch (Kernel::Exception::NotFoundError& ex)
	{
	  g_log.error("Filename property has not been set.");
	  return StatusCode::FAILURE;
	}
	
	std::string inputWorkspaceName;
	std::string outputWorkspaceName;
	// Retrieve the ws names from the properties
	try
	{
		inputWorkspaceName = getPropertyValue("InputWorkspace");
	}
	catch (Kernel::Exception::NotFoundError& ex)
	{
	  g_log.debug("InputWorkspace has not been set.");
	}

	try
	{
		outputWorkspaceName = getPropertyValue("OutputWorkspace");
	}
	catch (Kernel::Exception::NotFoundError& ex)
	{
	  g_log.error("OutputWorkspace has not been set.");
	  return StatusCode::FAILURE;
	}

	
	// Create the 2D workspace for the output
	// Get a pointer to the workspace factory (later will be shared)
	if (inputWorkspaceName != outputWorkspaceName)
	{
		API::WorkspaceFactory *factory = API::WorkspaceFactory::Instance();
		m_outputWorkspace = factory->create("Workspace2D");
	}
	else
	{
		m_outputWorkspace = m_inputWorkspace;
	}
	Workspace2D *localWorkspace = dynamic_cast<Workspace2D*>(m_outputWorkspace);

	///Geometry components
	API::Instrument& instrument = localWorkspace->getInstrument();
	Geometry::ObjComponent  *source = new Geometry::ObjComponent;
	Geometry::ObjComponent *samplepos = new Geometry::ObjComponent;
	Geometry::Detector tube;
	Geometry::CompAssembly *bank = new Geometry::CompAssembly;

	g_log.warning("Loading file" + m_filename + " and assuming it is a HET file");
	// This is reading the definition from a file 
	// containing the detector ID number, followed by
	// the position of the detectors in spherical coordinates
	// I choose  a convention based on the Busing-Levy convention
	// (crystallography). The y-axis is along the beam and z up
	// The coordinate system is right-handed.
	std::fstream in;
	in.open(m_filename.c_str(),std::ios::in);
	double R, theta, phi;
	if (in.is_open())
	{
		instrument.setName("HET");
		source->setName("Source");
		source->setParent(&instrument);
		samplepos->setName("SamplePos");
		samplepos->setParent(&instrument);
		samplepos->setPos(Geometry::V3D(0.0,10.0,0.0));
		bank->setName("Detectors");
		instrument.add(bank);
		instrument.add(source);
		instrument.add(samplepos);
		tube.setName("PSD");
		int i=0;
		int det_id=400;
		Geometry::V3D pos;
		double c1,c2,c3; //garbage values
		do
		{
			det_id++;
			in >>  R >> theta >> phi >> c1 >> c2 >> c3;
			pos.spherical(R,theta,phi);
			bank->addCopy(&tube);
			(*bank)[i]->setPos(pos);
			Geometry::Detector* temp=dynamic_cast<Geometry::Detector*>((*bank)[i]);
			if (temp) temp->setID(det_id);	
			//if (g_log.is(g_log.PRIO_DEBUG))
			//{
			//	char string_buf[400];
			//	sprintf(string_buf, "i= %d, det_id= %d, R= %f, theta= %f, phi= %f", i, det_id, R, theta, phi);
			//	std::string message(string_buf);
			//	g_log.debug(message);
			//}
			i++;
		}while(!in.eof()); 

		if (g_log.is(g_log.PRIO_DEBUG))
		{
			std::ostringstream oss;
			oss << "Instrument file loaded max detector_id = " << det_id;
			g_log.debug(oss.str());
		}
	}
	else
	{
		g_log.error("Error opening file" + m_filename);
		throw Kernel::Exception::FileError("Error opening instrument file",m_filename);
	}
		
	return StatusCode::SUCCESS;
	}

	/** Finalisation method. Does nothing at present.
	*
	*  @return A StatusCode object indicating whether the operation was successful
	*/
	StatusCode LoadInstrument::final()
	{
	return StatusCode::SUCCESS;
	}

} // namespace DataHandling
} // namespace Mantid
