#ifndef MANTID_CUSTOMINTERFACES_PROCESSINGINSTRUCTIONS_H_
#define MANTID_CUSTOMINTERFACES_PROCESSINGINSTRUCTIONS_H_

#include "../DllConfig.h"
#include <boost/optional.hpp>
namespace MantidQt {
namespace CustomInterfaces {

// For now processing instructions are just a string but we expect them to
// become more complicated so this can be changed to a class in the future
using ProcessingInstructions = std::string;
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_PROCESSINGINSTRUCTIONS_H_
