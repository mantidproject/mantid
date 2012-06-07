#ifndef MANTID_API_MULTIPLEFILEPROPERTY_H_
#define MANTID_API_MULTIPLEFILEPROPERTY_H_

#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/System.h"
#include "MantidKernel/MultiFileNameParser.h"
#include <vector>
#include <set>

namespace Mantid
{
namespace API
{

  /** A property to allow a user to select multiple files to load.
    
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
  class DLLExport MultipleFileProperty : public Kernel::PropertyWithValue< std::vector< std::vector< std::string> > >
  {
  public:
    ///Constructor
    MultipleFileProperty(const std::string & name,const std::vector<std::string> & exts = std::vector<std::string>());

    ~MultipleFileProperty();

    /// 'Virtual copy constructor
    virtual MultipleFileProperty* clone() { return new MultipleFileProperty(*this); }

    /// Overridden setValue method
    virtual std::string setValue(const std::string & propValue);

    /// @return the vector of suggested extensions. For use in GUIs showing files.
    std::set<std::string> getExts() const
    { return std::set<std::string>(m_exts.begin(), m_exts.end()); }
    /// @return the vector of ws names.  For use by loading algorithm to name multiple workspaces, especially summed workspaces.
    std::vector<std::vector<unsigned int> > getRuns() const
    { return m_parser.runs(); }
    
    /// Returns the main file extension that's used 
    std::string getDefaultExt() const {return m_defaultExt;}

    // Unhide the PropertyWithValue assignment operator
    using Kernel::PropertyWithValue< std::vector< std::vector< std::string> > >::operator=;

    /// Return a "flattened" vector with the contents of the given vector of vectors.
    static std::vector<std::string> flattenFileNames(const std::vector<std::vector<std::string> > & fileNames);

  private:
    /// Suggested extensions
    std::vector<std::string> m_exts;
    /// Parser used to parse multi-file strings.
    Kernel::MultiFileNameParsing::Parser m_parser;
    ///The default file extension associated with the type of file this property will handle
    std::string m_defaultExt;
  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_MULTIPLEFILEPROPERTY_H_ */
