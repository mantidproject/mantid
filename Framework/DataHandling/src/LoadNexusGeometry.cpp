//-----------------------------------------
// Includes
//-----------------------------------------
#include "MantidDataHandling/LoadNexusGeometry.h"

#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"

#include <iostream>

namespace Mantid {
namespace DataHandling{

//Register the algorithm into the algorithm factory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadNexusGeometry)

//Empty Default Constructor
LoadNexusGeometry::LoadNexusGeometry(){}

void LoadNexusGeometry::init(){

    declareProperty ("InstrumentName", " ", "Name of Instrument");

}
void LoadNexusGeometry::exec(){

    std::string m_instName = getPropertyValue("InstrumentName");

    //Load empty instrument
    Geometry::Instrument_sptr instrument;

    //Add name to instrument
    instrument = boost::make_shared<Geometry::Instrument>(m_instName);

    //Add source to instrument
    LoadNexusGeometry::addSource(instrument);

    API::InstrumentDataService::Instance ().add (m_instName, instrument);
}

int LoadNexusGeometry::confidence (Kernel::NexusDescriptor &descriptor) const{return 0;}

//Adds source to instrument
void LoadNexusGeometry::addSource (Geometry::Instrument_sptr &instrument)
{
    using namespace Geometry;

    ICompAssembly *source;
    source = new ObjCompAssembly("Source");
    instrument->markAsSource (source);



}

}
}
