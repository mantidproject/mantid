#include "MantidKernel/UnitLabel.h"

namespace Mantid
{
  namespace Kernel
  {

    /**
     */
    UnitLabel::~UnitLabel()
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
     * Returns the results of the ascii() method
     */
    UnitLabel::operator std::string() const
    {
      return this->ascii();
    }

  } // namespace Kernel
} // namespace Mantid
