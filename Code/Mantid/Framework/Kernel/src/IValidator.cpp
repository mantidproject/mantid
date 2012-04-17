#include "MantidKernel/IValidator.h"

namespace Mantid
{
  namespace Kernel
  {
    /// initialize static logger
    DLLExport Logger & IValidator::g_log = Logger::get("IValidator");
  }
}
