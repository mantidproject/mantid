#include "MantidCatalog/Exception.h"
#include "MantidCatalog/ONCatEntity.h"
#include "MantidKernel/StringTokenizer.h"

#include <iostream>
#include <sstream>
#include <json/json.h>

namespace Mantid {
namespace Catalog {
namespace ONCat {

using Mantid::Kernel::StringTokenizer;
using Mantid::Kernel::make_unique;
using Mantid::Catalog::Exception::MalformedRepresentationError;

//----------------------------------------------------------------------
// Anonymous Helpers
//----------------------------------------------------------------------
namespace {

class ContentError : public std::runtime_error {
public:
  explicit ContentError(const std::string &message)
      : std::runtime_error(message) {}
};

Content getNestedContent(const Content &content, const std::string &path) {
  const auto pathTokens =
      StringTokenizer(path, ".", Mantid::Kernel::StringTokenizer::TOK_TRIM);

  auto currentNode = content;

  // Use the path tokens to drill down through the JSON nodes.
  for (auto pathToken = pathTokens.cbegin(); pathToken != pathTokens.cend();
       ++pathToken) {
    if (!currentNode.isMember(*pathToken)) {
      throw ContentError("");
    }
    currentNode = currentNode[*pathToken];
  }

  return currentNode;
}

template <typename T>
T getNestedContentValueAsType(const Content &content, const std::string &path);
template <>
std::string getNestedContentValueAsType(const Content &content,
                                        const std::string &path) {
  return getNestedContent(content, path).asString();
}
template <>
int getNestedContentValueAsType(const Content &content,
                                const std::string &path) {
  return getNestedContent(content, path).asInt();
}
template <>
float getNestedContentValueAsType(const Content &content,
                                  const std::string &path) {
  return getNestedContent(content, path).asFloat();
}
template <>
double getNestedContentValueAsType(const Content &content,
                                   const std::string &path) {
  return getNestedContent(content, path).asDouble();
}
template <>
bool getNestedContentValueAsType(const Content &content,
                                 const std::string &path) {
  return getNestedContent(content, path).asBool();
}

template <typename T>
T getNestedContentValueElseDefault(const Content &content,
                                   const std::string &path, T defaultValue) {
  try {
    return getNestedContentValueAsType<T>(content, path);
  } catch (ContentError &) {
    return defaultValue;
  }
}

template <typename T>
boost::optional<T> getNestedContentValueIfPresent(const Content &content,
                                                  const std::string &path) {
  try {
    return boost::make_optional(getNestedContentValueAsType<T>(content, path));
  } catch (ContentError &) {
    return boost::none;
  }
}
}

//----------------------------------------------------------------------
// ONCatEntity
//----------------------------------------------------------------------

ONCatEntity::ONCatEntity(const std::string &id, const std::string &type,
                         Content_uptr content)
    : m_id(id), m_type(type), m_content(std::move(content)) {}

ONCatEntity::ONCatEntity(const ONCatEntity &other)
    : m_id(other.m_id), m_type(other.m_type),
      m_content(make_unique<Content>(*other.m_content)) {}

ONCatEntity::~ONCatEntity() {}

std::string ONCatEntity::id() const { return m_id; }

std::string ONCatEntity::type() const { return m_type; }

std::string ONCatEntity::asString(const std::string &path,
                                  const std::string defaultValue) const {
  return getNestedContentValueElseDefault(*m_content, path, defaultValue);
}

int ONCatEntity::asInt(const std::string &path, int defaultValue) const {
  return getNestedContentValueElseDefault(*m_content, path, defaultValue);
}

float ONCatEntity::asFloat(const std::string &path, float defaultValue) const {
  return getNestedContentValueElseDefault(*m_content, path, defaultValue);
}

double ONCatEntity::asDouble(const std::string &path,
                             double defaultValue) const {
  return getNestedContentValueElseDefault(*m_content, path, defaultValue);
}

bool ONCatEntity::asBool(const std::string &path, bool defaultValue) const {
  return getNestedContentValueElseDefault(*m_content, path, defaultValue);
}

boost::optional<std::string>
ONCatEntity::asString(const std::string &path) const {
  return getNestedContentValueIfPresent<std::string>(*m_content, path);
}

boost::optional<int> ONCatEntity::asInt(const std::string &path) const {
  return getNestedContentValueIfPresent<int>(*m_content, path);
}

boost::optional<float> ONCatEntity::asFloat(const std::string &path) const {
  return getNestedContentValueIfPresent<float>(*m_content, path);
}

boost::optional<double> ONCatEntity::asDouble(const std::string &path) const {
  return getNestedContentValueIfPresent<double>(*m_content, path);
}

boost::optional<bool> ONCatEntity::asBool(const std::string &path) const {
  return getNestedContentValueIfPresent<bool>(*m_content, path);
}

std::string ONCatEntity::toString() const {
  return m_content->toStyledString();
}

ONCatEntity ONCatEntity::fromJSONStream(std::istream &streamContent) {
  auto content = make_unique<Content>();

  try {
    streamContent >> *content;
  } catch (Json::Exception &je) {
    throw MalformedRepresentationError(je.what());
  }

  const auto id = content->get("id", "").asString();
  const auto type = content->get("type", "").asString();

  if (id == "" || type == "") {
    throw MalformedRepresentationError(
        "Expected \"id\" and \"type\" attributes from ONCat API, but these "
        "were not found.");
  }

  return ONCatEntity(id, type, std::move(content));
}

std::vector<ONCatEntity>
ONCatEntity::vectorFromJSONStream(std::istream &streamContent) {
  auto content = make_unique<Content>();

  try {
    streamContent >> *content;
  } catch (Json::Exception &je) {
    throw MalformedRepresentationError(je.what());
  }

  if (!content->isArray()) {
    throw MalformedRepresentationError(
        "Expected JSON representation to be an array of entities.");
  }

  std::vector<ONCatEntity> entities;

  for (const auto &subContent : *content) {
    const auto id = subContent.get("id", "").asString();
    const auto type = subContent.get("type", "").asString();

    if (id == "" || type == "") {
      throw MalformedRepresentationError(
          "Expected \"id\" and \"type\" attributes from ONCat API, but these "
          "were not found.");
    }

    entities.push_back(ONCatEntity(id, type, make_unique<Content>(subContent)));
  }

  return entities;
}

} // namespace ONCat
} // namespace Catalog
} // namespace Mantid
