// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCatalog/ONCatEntity.h"
#include "MantidCatalog/Exception.h"
#include "MantidKernel/StringTokenizer.h"

#include <sstream>
#include <utility>

namespace Mantid::Catalog::ONCat {

using Mantid::Catalog::Exception::MalformedRepresentationError;
using Mantid::Kernel::StringTokenizer;

ONCatEntity::ONCatEntity(std::string id, std::string type, Content_uptr content)
    : m_id(std::move(id)), m_type(std::move(type)), m_content(std::move(content)) {}

ONCatEntity::ONCatEntity(const ONCatEntity &other)
    : m_id(other.m_id), m_type(other.m_type), m_content(std::make_unique<Content>(*other.m_content)) {}

ONCatEntity &ONCatEntity::operator=(ONCatEntity other) {
  std::swap(this->m_id, other.m_id);
  std::swap(this->m_type, other.m_type);
  std::swap(this->m_content, other.m_content);
  return *this;
}

ONCatEntity::~ONCatEntity() = default;

std::string ONCatEntity::id() const { return m_id; }

std::string ONCatEntity::type() const { return m_type; }

std::string ONCatEntity::toString() const { return m_content->toStyledString(); }

ONCatEntity ONCatEntity::fromJSONStream(std::istream &streamContent) {
  auto content = std::make_unique<Content>();

  try {
    streamContent >> *content;
  } catch (Json::Exception &je) {
    throw MalformedRepresentationError(je.what());
  }

  const auto idStr = content->get("id", "").asString();
  const auto typeStr = content->get("type", "").asString();

  if (idStr == "" || typeStr == "") {
    throw MalformedRepresentationError("Expected \"id\" and \"type\" attributes from ONCat API, but these "
                                       "were not found.");
  }

  return ONCatEntity(idStr, typeStr, std::move(content));
}

std::vector<ONCatEntity> ONCatEntity::vectorFromJSONStream(std::istream &streamContent) {
  auto content = std::make_unique<Content>();

  try {
    streamContent >> *content;
  } catch (Json::Exception &je) {
    throw MalformedRepresentationError(je.what());
  }

  if (!content->isArray()) {
    throw MalformedRepresentationError("Expected JSON representation to be an array of entities.");
  }

  std::vector<ONCatEntity> entities;

  for (const auto &subContent : *content) {
    const auto idStr = subContent.get("id", "").asString();
    const auto typeStr = subContent.get("type", "").asString();

    if (idStr == "" || typeStr == "") {
      throw MalformedRepresentationError("Expected \"id\" and \"type\" attributes from ONCat API, but these "
                                         "were not found.");
    }

    entities.emplace_back(ONCatEntity(idStr, typeStr, std::make_unique<Content>(subContent)));
  }

  return entities;
}

template <>
std::string ONCatEntity::getNestedContentValueAsType(const Content &content, const std::string &path) const {
  return getNestedContent(content, path).asString();
}
template <> int ONCatEntity::getNestedContentValueAsType(const Content &content, const std::string &path) const {
  return getNestedContent(content, path).asInt();
}
template <> float ONCatEntity::getNestedContentValueAsType(const Content &content, const std::string &path) const {
  return getNestedContent(content, path).asFloat();
}
template <> double ONCatEntity::getNestedContentValueAsType(const Content &content, const std::string &path) const {
  return getNestedContent(content, path).asDouble();
}
template <> bool ONCatEntity::getNestedContentValueAsType(const Content &content, const std::string &path) const {
  return getNestedContent(content, path).asBool();
}

Content ONCatEntity::getNestedContent(const Content &content, const std::string &path) const {
  const auto pathTokens = StringTokenizer(path, ".", Mantid::Kernel::StringTokenizer::TOK_TRIM);

  auto currentNode = content;

  // Use the path tokens to drill down through the JSON nodes.
  for (const auto &pathToken : pathTokens) {
    if (!currentNode.isMember(pathToken)) {
      throw ContentError("");
    }
    currentNode = currentNode[pathToken];
  }

  return currentNode;
}

} // namespace Mantid::Catalog::ONCat
