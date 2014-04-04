#include "MantidKernel/UnitLabelTypes.h"

namespace Mantid { namespace Kernel
{
  namespace Units
  {
    //=========================================================================
    // Empty label
    //=========================================================================
    /// @copydoc UnitLabel
    const std::string EmptyLabel::ascii() const { return ""; }
    /// @copydoc UnitLabel
    const std::wstring EmptyLabel::utf8() const { return L""; }

    //=========================================================================
    // Microseconds
    //=========================================================================
    /// @copydoc UnitLabel
    const std::string Microseconds::ascii() const { return "microseconds"; }
    /// @copydoc UnitLabel
    const std::wstring Microseconds::utf8() const { return L"\u03bcs"; }

  } //namespace Units
}} // namespace Mantid::Kernel
