// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <stdexcept>

namespace Mantid {
namespace Catalog {
namespace Exception {

class CatalogError : public std::runtime_error {
public:
  explicit CatalogError(const std::string &message) : std::runtime_error(message) {}
};

class InvalidCredentialsError final : public CatalogError {
public:
  explicit InvalidCredentialsError(const std::string &message) : CatalogError(message) {}
};

class TokenRejectedError final : public CatalogError {
public:
  explicit TokenRejectedError(const std::string &message) : CatalogError(message) {}
};

class TokenParsingError final : public CatalogError {
public:
  explicit TokenParsingError(const std::string &message) : CatalogError(message) {}
};

class InvalidRefreshTokenError final : public CatalogError {
public:
  explicit InvalidRefreshTokenError(const std::string &message) : CatalogError(message) {}
};

class MalformedRepresentationError final : public CatalogError {
public:
  explicit MalformedRepresentationError(const std::string &message) : CatalogError(message) {}
};

class ContentError : public CatalogError {
public:
  explicit ContentError(const std::string &message) : CatalogError(message) {}
};

} // namespace Exception
} // namespace Catalog
} // namespace Mantid
