//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Instrument.h"

#include "Detector.h"
#include "CompAssembly.h"
#include "Component.h"

#include <fstream>

namespace Mantid
{
namespace DataHandling
{

DECLARE_ALGORITHM(LoadInstrument)

using namespace Kernel;
using namespace API;

Logger& LoadInstrument::g_log = Logger::get("LoadInstrument");

/// Empty default constructor
LoadInstrument::LoadInstrument()
{}

/// Initialisation method.
void LoadInstrument::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(new WorkspaceProperty<Workspace>("Workspace","Anonymous",Direction::InOut));
  declareProperty("Filename","",new MandatoryValidator);
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 * 
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void LoadInstrument::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  // Get the input workspace
  Workspace_sptr localWorkspace = getProperty("Workspace");
  
  ///Geometry components
  API::Instrument& instrument = localWorkspace->getInstrument();
  Geometry::ObjComponent *source = new Geometry::ObjComponent;
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
    bank->setPos(Geometry::V3D(0.0,10.0,0.0));
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
      in >> R >> theta >> phi >> c1 >> c2 >> c3;
      pos.spherical(R,theta,phi);
      bank->addCopy(&tube);
      (*bank)[i]->setPos(pos);
      Geometry::Detector* temp=dynamic_cast<Geometry::Detector*>((*bank)[i]);
      if (temp) temp->setID(det_id);
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
  //returns here if finished normally		
  return;
}

/** Finalisation method. Does nothing at present.
 *
 */
void LoadInstrument::final()
{
}

} // namespace DataHandling
} // namespace Mantid
