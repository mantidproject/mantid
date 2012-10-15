#ifndef MANTID_GEOMETRY_IDFOBJECT_H_
#define MANTID_GEOMETRY_IDFOBJECT_H_

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <Poco/Timestamp.h>
#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid
{
namespace Geometry
{

  /** IDFObject : File object wrapper over an IDF file.

  This is essentially an adapter for a Poco::File, with some extra convenience methods allowing easy access to the parent directory path.
  This type has the last modified date and the exists method as a virtual methods to facilite testing.
    
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport IDFObject 
  {
  public:
    static const std::string expectedExtension();

    IDFObject(const std::string& fileName);
    const Poco::Path& getParentDirectory() const;
    const Poco::Path& getFileFullPath() const;
    std::string getFileNameOnly() const;
    std::string getExtension() const;
    virtual Poco::Timestamp  getLastModified() const;
    virtual bool exists() const;
    virtual ~IDFObject();

  private:
    IDFObject(const IDFObject&);
    IDFObject & operator=(const IDFObject&);
    const Poco::File m_defFile;
    const bool m_hasFileName;
    const Poco::Path m_cachePath;
    const Poco::Path m_cacheParentDirectory;
  };

  typedef boost::shared_ptr<IDFObject> IDFObject_sptr;
  typedef boost::shared_ptr<const IDFObject> IDFObject_const_sptr;


} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_IDFOBJECT_H_ */
