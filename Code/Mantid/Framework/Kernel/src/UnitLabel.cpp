#include "MantidKernel/UnitLabel.h"
#include <cstring>

namespace Mantid {
namespace Kernel {

/**
 * @param ascii A plain-text label containing only ascii characters
 * @param unicode A label that can contain unicode characters
 * @param latex A text label containg the ascii characters with latex formatting
 */
UnitLabel::UnitLabel(const AsciiString &ascii, const Utf8String &unicode,
                     const AsciiString &latex)
    : m_ascii(ascii), m_utf8(unicode), m_latex(latex) {}

/**
 * Use an ASCII string for the unicode variant too
 * @param ascii A plain-text label containing only ascii characters
 */
UnitLabel::UnitLabel(const UnitLabel::AsciiString &ascii)
    : m_ascii(ascii), m_utf8(ascii.begin(), ascii.end()), m_latex(ascii) {}

/**
 * Use an ASCII string for the unicode variant too, given
 * as a C-style string
 * @param ascii A plain-text label
 */
UnitLabel::UnitLabel(const char *ascii)
    : m_ascii(ascii), m_utf8(m_ascii.begin(), m_ascii.end()), m_latex(ascii) {}

/**
 * Test if two objects are considered equal
 * @param rhs A second label object
 * @return True if they are conisdered equal, false otherwise
 */
bool UnitLabel::operator==(const UnitLabel &rhs) const {
  return (this->ascii() == rhs.ascii() && this->utf8() == rhs.utf8());
}

/**
 * Test if this object is considered equal to another std::string.
 * It compares to result of ascii()
 * @param rhs A string to compare
 * @return True if they are conisdered equal, false otherwise
 */
bool UnitLabel::operator==(const std::string &rhs) const {
  return (this->ascii() == rhs);
}

/**
 * Test if this object is considered equal to another c-style string.
 * It compares to result of ascii()
 * @param rhs A string to compare
 * @return True if they are conisdered equal, false otherwise
 */
bool UnitLabel::operator==(const char *rhs) const {
  return (strcmp(ascii().c_str(), rhs) == 0);
}

/**
 * Test if this object is considered equal to another std::wstring.
 * It compares to result of utf8()
 * @param rhs A string to compare
 * @return True if they are conisdered equal, false otherwise
 */
bool UnitLabel::operator==(const std::wstring &rhs) const {
  return (this->utf8() == rhs);
}

/**
 * Test if two objects are not considered equal
 * @param rhs A second label object
 * @return True if they are conisdered equal, false otherwise
 */
bool UnitLabel::operator!=(const UnitLabel &rhs) const {
  return !(*this == rhs);
}

/**
 * Test if this object is not considered equal to another std::string.
 * It compares to result of ascii()
 * @param rhs A string to compare
 * @return True if they are conisdered equal, false otherwise
 */
bool UnitLabel::operator!=(const std::string &rhs) const {
  return !(*this == rhs);
}

/**
 * Test if this object is not considered equal to another c-style string
 * It compares to result of ascii()
 * @param rhs A string to compare
 * @return True if they are conisdered equal, false otherwise
 */
bool UnitLabel::operator!=(const char *rhs) const { return !(*this == rhs); }

/**
 * Test if this object is not considered equal to another std::wstring.
 * It compares to result of utf8()
 * @param rhs A string to compare
 * @return True if they are conisdered equal, false otherwise
 */
bool UnitLabel::operator!=(const std::wstring &rhs) const {
  return !(*this == rhs);
}

/**
 * @return A std::string containing the plain-text label
 */
const UnitLabel::AsciiString &UnitLabel::ascii() const { return m_ascii; }

/**
 * @return A UnitLabel::utf8string containing the unicode label
 */
const UnitLabel::Utf8String &UnitLabel::utf8() const { return m_utf8; }

/**
 * @return A std::string containing the latex label
 */
const UnitLabel::AsciiString &UnitLabel::latex() const { return m_latex; }

/**
 * Returns the results of the ascii() method
 */
UnitLabel::operator std::string() const { return this->ascii(); }

} // namespace Kernel
} // namespace Mantid
