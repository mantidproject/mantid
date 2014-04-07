#include "MantidKernel/UnitLabel.h"

namespace Mantid
{
  namespace Kernel
  {

    /**
     * @param ascii A plain-text label containing only ascii characters
     * @param unicode A label that can contain unicode characters
     */
    UnitLabel::UnitLabel(const std::string &ascii, const std::wstring &unicode)
      : m_ascii(ascii), m_utf8(unicode)
    {
    }

    /**
     * Test if two objects are considered equal
     * @param rhs A second label object
     * @return True if they are conisdered equal, false otherwise
     */
    bool UnitLabel::operator==(const UnitLabel &rhs) const
    {
      return (this->ascii() == rhs.ascii() &&
              this->utf8() == rhs.utf8());
    }

    /**
     * Test if this object is considered equal to another std::string.
     * It compares to result of ascii()
     * @param rhs A string to compare
     * @return True if they are conisdered equal, false otherwise
     */
    bool UnitLabel::operator==(const std::string &rhs) const
    {
      return (this->ascii() == rhs);
    }

    /**
     * Test if this object is considered equal to another std::wstring.
     * It compares to result of utf8()
     * @param rhs A string to compare
     * @return True if they are conisdered equal, false otherwise
     */
    bool UnitLabel::operator==(const std::wstring &rhs) const
    {
      return (this->utf8() == rhs);
    }

    /**
     * @return A std::string containing the plain-text label
     */
    const UnitLabel::AsciiString & UnitLabel::ascii() const
    {
      return m_ascii;
    }

    /**
     * @return A UnitLabel::utf8string containing the unicode label
     */
    const UnitLabel::Utf8String & UnitLabel::utf8() const
    {
      return m_utf8;
    }

    /**
     * Returns the results of the ascii() method
     */
    UnitLabel::operator std::string() const
    {
      return this->ascii();
    }

  } // namespace Kernel
} // namespace Mantid
