#ifndef INSTRUMENTCREATIONHELPER_H_
#define INSTRUMENTCREATIONHELPER_H_

#include "MantidTestHelpers/ComponentCreationHelper.h"

namespace Mantid {
namespace API {
class MatrixWorkspace;
}
} // namespace Mantid

namespace InstrumentCreationHelper {

void addFullInstrumentToWorkspace(Mantid::API::MatrixWorkspace &workspace,
                                  bool includeMonitors, bool startYNegative,
                                  const std::string &instrumentName);
}

#endif /* INSTRUMENTCREATIONHELPER_H_ */
