#ifndef PARSING_ERRORS_H_
#define PARSING_ERRORS_H_

// Contains enum for possible parsing errors, and suggested error messages
// Allows error to be transmitted back to LoadNexusGeometry

//--------------------------------
// Includes
//--------------------------------
#include "MantidNexusGeometry/DllConfig.h"
#include <string>

namespace Mantid {
namespace NexusGeometry {

enum ParsingErrors {
  NO_ERROR,
  OPENING_FILE_ERROR,
  OPENING_ROOT_GROUP_ERROR,
  UNKNOWN_ERROR
};

class DLLExport ParseErrors {
public:
  ParseErrors() = default;
  ~ParseErrors() = default;
  std::string ParsingErrorMessages(ParsingErrors pError);
};
}
}

#endif // PARSING_ERRORS_H_
