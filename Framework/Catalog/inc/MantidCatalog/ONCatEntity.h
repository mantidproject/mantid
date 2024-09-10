// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCatalog/DllConfig.h"
#include "MantidCatalog/Exception.h"

#include <memory>
#include <string>
#include <vector>

#include <json/json.h>
#include <optional>

namespace Mantid {
namespace Catalog {
namespace ONCat {

using Content = Json::Value;
using Content_uptr = std::unique_ptr<Content>;
using Mantid::Catalog::Exception::ContentError;

/**
 * A class to encapsulate the "entity" responses received from the ONCat API.
 *
 * An ONCatEntity object (or a vector of objects) can be constructed when
 * given an istream, which is assumed to contain JSON information as defined
 * in the API documentation at https://oncat.ornl.gov/#/build.
 *
 * Note that there are only two fields shared across all API entity types:
 * "id" and "type".  Further, all other fields can be optionally disabled
 * through the use of "projections", and a certain subset of fields may even
 * be completely missing for a given file because of the dynamic nature of
 * metadata resulting from Data Acquisition software changes.
 *
 * For this reason, all other metadata will be retrieved in a way that
 * forces you to deal with the case where the field in question is not there.
 * There are two ways of doing this: the first is to specify a default value
 * to be used when a value is not present, and the second is to check for a
 * result on a std::optional.
 *
 * However, if your projection is such that you *know* a field will be present
 * (note that most fields on API resources will always be returned as long
 * as they are requested as part of a projection, for example the "location"
 * field of the Datafile resource), then feel free to assume it will be
 * there and resolve the std::optional without checking for a result.
 *
 * @author Peter Parker
 * @date 2018
 */
class MANTID_CATALOG_DLL ONCatEntity {
public:
  ONCatEntity() = delete;
  ONCatEntity(const ONCatEntity &);
  ONCatEntity &operator=(ONCatEntity other);
  ~ONCatEntity();

  // These are the only two fields the ONCat API guarantees will be there
  // across *all* entity types.
  std::string id() const;
  std::string type() const;

  // For all other fields, you can either supply a default value for when a
  // value does not exist ...
  template <typename T> T get(const std::string &path, T defaultValue) const {
    try {
      return getNestedContentValueAsType<T>(*m_content, path);
    } catch (ContentError &) {
      return defaultValue;
    }
  }

  // ... or, write conditional logic around boost's optional results.
  template <typename T> std::optional<T> get(const std::string &path) const {
    try {
      return std::make_optional(getNestedContentValueAsType<T>(*m_content, path));
    } catch (ContentError &) {
      return std::nullopt;
    }
  }

  std::string toString() const;

  static ONCatEntity fromJSONStream(std::istream &streamContent);
  static std::vector<ONCatEntity> vectorFromJSONStream(std::istream &streamContent);

private:
  ONCatEntity(std::string id, std::string type, Content_uptr content);

  template <typename T> T getNestedContentValueAsType(const Content &content, const std::string &path) const;

  Content getNestedContent(const Content &content, const std::string &path) const;

  std::string m_id;
  std::string m_type;
  Content_uptr m_content;
};

template <>
MANTID_CATALOG_DLL std::string ONCatEntity::getNestedContentValueAsType(const Content &content,
                                                                        const std::string &path) const;
template <>
MANTID_CATALOG_DLL int ONCatEntity::getNestedContentValueAsType(const Content &content, const std::string &path) const;
template <>
MANTID_CATALOG_DLL float ONCatEntity::getNestedContentValueAsType(const Content &content,
                                                                  const std::string &path) const;
template <>
MANTID_CATALOG_DLL double ONCatEntity::getNestedContentValueAsType(const Content &content,
                                                                   const std::string &path) const;
template <>
MANTID_CATALOG_DLL bool ONCatEntity::getNestedContentValueAsType(const Content &content, const std::string &path) const;

} // namespace ONCat
} // namespace Catalog
} // namespace Mantid
