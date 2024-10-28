// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

template <typename Validated, typename Error = boost::blank> class ValidationResult {
public:
  explicit ValidationResult(Validated validItem);
  explicit ValidationResult(Error validationErrors);
  ValidationResult();

  bool isValid() const;
  bool isError() const;
  Validated const &assertValid() const;
  Error const &assertError() const;
  boost::optional<Validated> validElseNone() const;

private:
  boost::variant<Validated, Error> m_innerResult;
};

template <typename Validated, typename Error>
ValidationResult<Validated, Error>::ValidationResult() : m_innerResult(boost::blank()) {}

template <typename Validated, typename Error>
ValidationResult<Validated, Error>::ValidationResult(Validated validItem) : m_innerResult(std::move(validItem)) {}

template <typename Validated, typename Error>
ValidationResult<Validated, Error>::ValidationResult(Error validationErrors)
    : m_innerResult(std::move(validationErrors)) {}

template <typename Validated, typename Error> bool ValidationResult<Validated, Error>::isValid() const {
  return m_innerResult.which() == 0;
}

template <typename Validated, typename Error> bool ValidationResult<Validated, Error>::isError() const {
  return m_innerResult.which() == 1;
}

template <typename Validated, typename Error> Validated const &ValidationResult<Validated, Error>::assertValid() const {
  return boost::get<Validated>(m_innerResult);
}

template <typename Validated, typename Error> Error const &ValidationResult<Validated, Error>::assertError() const {
  return boost::get<Error>(m_innerResult);
}

template <typename Validated, typename Error>
boost::optional<Validated> ValidationResult<Validated, Error>::validElseNone() const {
  if (isValid())
    return assertValid();
  else
    return boost::none;
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
