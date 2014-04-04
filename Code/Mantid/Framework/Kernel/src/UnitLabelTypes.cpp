#include "MantidKernel/UnitLabelTypes.h"

namespace Mantid { namespace Kernel
{
  namespace Units
  {
#define DEFINE_UNIT_LABEL(ClassName, AsciiText, Utf8Text)\
  ClassName * ClassName::clone() const { return new ClassName(*this); }\
\
  const std::string ClassName::ascii() const { return AsciiText; }\
\
  const std::wstring ClassName::utf8() const { return L##Utf8Text; }

    //-------------------------------------------------------------------------
    // Empty label
    DEFINE_UNIT_LABEL(EmptyLabel, "", "");
    // Second
    DEFINE_UNIT_LABEL(Second, "s", "s");
    // Microsecond
    DEFINE_UNIT_LABEL(Microsecond, "microsecond", "\u03bcs");
    /// Nanosecond
    DEFINE_UNIT_LABEL(Nanosecond, "ns", "ns");
    // Angstrom
    DEFINE_UNIT_LABEL(Angstrom, "Angstrom", "\u212b");
    // InverseAngstrom
    DEFINE_UNIT_LABEL(InverseAngstrom, "Angstrom^-1", "\u212b\u207b\u00b9");
    // InverseAngstromSq
    DEFINE_UNIT_LABEL(InverseAngstromSq, "Angstrom^-2", "\u212b\u207b\u00b2");
    /// MilliElectronVolts
    DEFINE_UNIT_LABEL(MilliElectronVolts, "meV", "meV");
    /// Metre
    DEFINE_UNIT_LABEL(Metre, "m", "m");
    /// Nanometre
    DEFINE_UNIT_LABEL(Nanometre, "nm", "nm");
    /// Inverse centimeters
    DEFINE_UNIT_LABEL(InverseCM, "cm^-1", "cm\u207b\u00b9");

    //-------------------------------------------------------------------------
    // Non-default constructor types
    //-------------------------------------------------------------------------

    //=========================================================================
    // Text label

    /**
     * Construct object with plain text and utf-8 encoded label
     * @param ascii Plain-text label
     * @param utf8 UTF-8 encoded label
     */
    TextLabel::TextLabel(const std::string &ascii, const std::wstring &utf8)
      : m_ascii(ascii), m_utf8(utf8)
    {
    }

    /// @copydoc UnitLabel
    TextLabel * TextLabel::clone() const { return new TextLabel(*this); }

    /// @copydoc UnitLabel
    const std::string TextLabel::ascii() const { return m_ascii; }

    /// @copydoc UnitLabel
    const std::wstring TextLabel::utf8() const { return m_utf8; }

    //=========================================================================


  } //namespace Units
}} // namespace Mantid::Kernel
