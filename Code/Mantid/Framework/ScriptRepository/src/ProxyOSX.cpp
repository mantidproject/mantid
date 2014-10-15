#include "MantidScriptRepository/ProxyOSX.h"

namespace Mantid
{
  namespace ScriptRepository
  {
    ProxyInfo ProxyOSX::getHttpProxy(const std::string& targetURL)
    {
      return ProxyInfo();
    }

  } // namespace ScriptRepository
} // namespace Mantid
