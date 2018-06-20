#ifndef MANTID_CATALOG_ONCATENTITY_H_
#define MANTID_CATALOG_ONCATENTITY_H_

#include "MantidCatalog/DllConfig.h"
#include "MantidKernel/make_unique.h"

#include <string>
#include <vector>

#include <boost/optional.hpp>

namespace Json {
class Value;
}

namespace Mantid {
namespace Catalog {
namespace ONCat {

using Content = Json::Value;
using Content_uptr = std::unique_ptr<Content>;

/**
  A class to encapsulate the "entity" responses received from the ONCat API.

  An ONCatEntity object (or a vector of objects) can be constructed when
  given an istream, which is assumed to contain JSON information as defined
  in the API documentation at https://oncat.ornl.gov/#/build.

  Note that there are only two fields shared across all API entity types:
  "id" and "type".  Further, all other fields can be optionally disabled
  through the use of "projections", and a certain subset of fields may even
  be completely missing for a given file because of the dynamic nature of
  metadata resulting from Data Acquisition software changes.

  For this reason, all other metadata will be retrieved in a way that
  forces you to deal with the case where the field in question is not there.
  There are two ways of doing this: the first is to specify a default value
  to be used when a value is not present, and the second is to check for a
  result on a boost::optional.

  However, if your projection is such that you *know* a field will be present
  (note that most fields on API resources will always be returned as long
  as they are requested as part of a projection, for example the "location"
  field of the Datafile resource), then feel free to assume it will be
  there and resolve the boost::optional without checking for a result.

  @author Peter Parker
  @date 2018

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_CATALOG_DLL ONCatEntity {
public:
  ONCatEntity() = delete;
  ONCatEntity(const ONCatEntity &);
  ~ONCatEntity();

  // These are the only two fields the ONCat API guarantees will be
  // there across *all* entity types.
  std::string id() const;
  std::string type() const;

  // So, you can either supply a default value for when the field you
  // want is not there ...
  std::string asString(const std::string &path,
                       const std::string defaultValue) const;
  int asInt(const std::string &path, int defaultValue) const;
  float asFloat(const std::string &path, float defaultValue) const;
  double asDouble(const std::string &path, double defaultValue) const;
  bool asBool(const std::string &path, bool defaultValue) const;

  // ... or, write conditional logic around boost's optional results.
  boost::optional<std::string> asString(const std::string &path) const;
  boost::optional<int> asInt(const std::string &path) const;
  boost::optional<float> asFloat(const std::string &path) const;
  boost::optional<double> asDouble(const std::string &path) const;
  boost::optional<bool> asBool(const std::string &path) const;

  std::string toString() const;

  static ONCatEntity fromJSONStream(std::istream &streamContent);
  static std::vector<ONCatEntity>
  vectorFromJSONStream(std::istream &streamContent);

private:
  ONCatEntity(const std::string &id, const std::string &type,
              Content_uptr content);

  std::string m_id;
  std::string m_type;
  Content_uptr m_content;
};

} // namespace ONCat
} // namespace Catalog
} // namespace Mantid

#endif /* MANTID_CATALOG_ONCATENTITY_H_ */
