//-----------------------------------------
// Includes
//-----------------------------------------
#include "MantidDataHandling/LoadNexusGeometry.h"

#include "MantidAPI/RegisterFileLoader.h"

namespace Mantid {
namespace DataHandling{

//Register the algorithm into the algorithm factory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadNexusGeometry)

//Empty Default Constructor
LoadNexusGeometry::LoadNexusGeometry(){}

void LoadNexusGeometry::init(){}
void LoadNexusGeometry::exec(){}

int LoadNexusGeometry::confidence (Kernel::NexusDescriptor &descriptor) const{return 0;}

}
}
