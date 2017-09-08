//
// Created by michael on 23/08/17.
//

#ifndef MANTID_NEXUS_GEOMETRY_PARSER_H_
#define MANTID_NEXUS_GEOMETRY_PARSER_H_

//---------------------------------
// Includes
//---------------------------------

#include "MantidNexusGeometry/ParsingErrors.h"

#include <H5Cpp.h>

#include <memory>

namespace Mantid {
namespace NexusGeometry {

class NexusGeometryParser
{
public:
    /// Constructor
    explicit NexusGeometryParser(const H5std_string &fileName, std::weak_ptr<InstrumentHandler> iHandler);

    /// Destructor
    ~NexusGeometryParser() = default;

private:
    H5::H5File nexusFile;
    std::weak_ptr<InstrumentHandler> iHandler;

};

}
}

#endif // MANTID_NEXUS_GEOMETRY_PARSER_H_
