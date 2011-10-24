#ifndef MANTID_API_MULTIPLEFILEPROPERTY_H_
#define MANTID_API_MULTIPLEFILEPROPERTY_H_
    
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"


namespace Mantid
{
namespace API
{

  /** A property to allow a user to select multiple
   * files to load.
    
    @author Janik Zikovsky
    @date 2011-08-17

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport MultipleFileProperty : public Kernel::ArrayProperty<std::string>
  {
  public:
    MultipleFileProperty(const std::string & name, const std::vector<std::string> & exts = std::vector<std::string>(),
        bool optional=false);

    ~MultipleFileProperty();

    ///Overridden setValue method
    virtual std::string setValue(const std::string & propValue);
    /// Set a property value via a DataItem
    virtual std::string setValue(const boost::shared_ptr<Kernel::DataItem> data);

    /// @return the vector of suggested extensions. For use in GUIs showing files.
    const std::set<std::string> & getExts() const
    { return m_exts; }

  private:
    /// Suggested extensions
    std::set<std::string> m_exts;

    /// Is the file optional?
    bool m_optional;

  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_MULTIPLEFILEPROPERTY_H_ */
