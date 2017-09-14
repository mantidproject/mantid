//------------------------------
// Includes
//------------------------------

#include "MantidNexusGeometry/ParsingErrors.h"

namespace Mantid{
namespace NexusGeometry{

/**
 * Provides default error messages for the errors
 * generated by NexusGeometryParser
 * @param pError
 * @return suggested error message
 */
std::string ParsingErrorMessages(ParsingErrors pError){
    switch (pError){
        case NO_ERROR:
            return "Parsing Completed";
        case OPENING_FILE_ERROR:
            return "File failed to open or does not exist";
        case OPENING_ROOT_GROUP_ERROR:
            return "Failed to open file root \"/\"";
    }
}

}
};
