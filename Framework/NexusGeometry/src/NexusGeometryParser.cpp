//
// Created by michael on 23/08/17.
//

#include "MantidNexusGeometry/NexusGeometryParser.h"

namespace Mantid {
namespace NexusGeometry {

using namespace H5;

/// Constructor opens the nexus file
NexusGeometryParser::NexusGeometryParser(
    const H5std_string &fileName, std::weak_ptr<InstrumentHandler> iHandler) {

    // Disable automatic printing, so Load algorithm can deal with errors
    // appropriately
    Exception::dontPrint();
    try {
        this->nexusFile.openFile(fileName, H5F_ACC_RDONLY);
    } catch (FileIException e) {

    }
    this->iHandler = iHandler;
}
}
}