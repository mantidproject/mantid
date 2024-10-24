// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidGeometry/Instrument.h"
#include <boost/variant.hpp>
#include <string>
#include <vector>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
template <typename T> class InstrumentParameter;

template <> class InstrumentParameter<std::string> {
public:
  static std::vector<std::string> get(const Mantid::Geometry::Instrument_const_sptr &instrument,
                                      std::string const &parameterName);
};

template <> class InstrumentParameter<double> {
public:
  static std::vector<double> get(const Mantid::Geometry::Instrument_const_sptr &instrument,
                                 std::string const &parameterName);
};

template <> class InstrumentParameter<int> {
public:
  static std::vector<int> get(const Mantid::Geometry::Instrument_const_sptr &instrument,
                              std::string const &parameterName);
};

template <> class InstrumentParameter<bool> {
public:
  static std::vector<bool> get(const Mantid::Geometry::Instrument_const_sptr &instrument,
                               std::string const &parameterName);
};

class InstrumentParameterTypeMissmatch : public std::runtime_error {
public:
  InstrumentParameterTypeMissmatch(const std::string &parameterName, const std::string &expectedType,
                                   const std::runtime_error &ex);

  std::string const &parameterName() const;
  std::string const &expectedType() const;
  std::string const &originalMessage() const;

private:
  std::string m_parameterName;
  std::string m_expectedType;
  std::string m_originalMessage;
};

/**
 * Get's a parameter which may hold a value of one of two different types.
 * Tries the type T1 and then T2.
 *
 * Returns a variant containing an empty vector of T2 in the
 * if the parameter does not exist in the file.
 *
 * Throws a InstrumentParameterTypeMissmatch in the event that the
 * parameter is not any of the specified types. The expected type string will
 * be a the two types separated by " or a " e.g. "int or a string".
 */
template <typename T1, typename T2> class InstrumentParameter<boost::variant<T1, T2>> {
public:
  static boost::variant<std::vector<T1>, std::vector<T2>> get(const Mantid::Geometry::Instrument_const_sptr &instrument,
                                                              std::string const &parameterName) {
    try {
      return InstrumentParameter<T1>::get(instrument, parameterName);
    } catch (InstrumentParameterTypeMissmatch const &t1ex) {
      try {
        return InstrumentParameter<T2>::get(instrument, parameterName);
      } catch (InstrumentParameterTypeMissmatch const &t2ex) {
        throw InstrumentParameterTypeMissmatch(parameterName, t1ex.expectedType() + " or a " + t2ex.expectedType(),
                                               t2ex);
      }
    }
  }
};

/**
 * Get's a parameter which may hold a value of one of 3 or more types.
 * Tries the types in the left to right order as specified.
 *
 * Returns a variant containing an empty vector of the last type in the
 * list if the parameter
 * does not exist in the file.
 *
 * Throws a InstrumentParameterTypeMissmatch in the event that the
 * parameter is not any of the specified types. The expected type string will
 * be a list of possible types separated by " or a ".
 */
template <typename T1, typename T2, typename T3, typename... Ts>
class InstrumentParameter<boost::variant<T1, T2, T3, Ts...>> {
public:
  static boost::variant<std::vector<T1>, std::vector<T2>, std::vector<T3>, std::vector<Ts>...>
  get(const Mantid::Geometry::Instrument_const_sptr &instrument, std::string const &parameterName) {
    try {
      return InstrumentParameter<T1>::get(instrument, parameterName);
    } catch (InstrumentParameterTypeMissmatch const &t1ex) {
      try {
        return InstrumentParameter<boost::variant<T2, T3, Ts...>>::get(instrument, parameterName);
      } catch (InstrumentParameterTypeMissmatch const &t2ex) {
        throw InstrumentParameterTypeMissmatch(parameterName, t1ex.expectedType() + " or a " + t2ex.expectedType(),
                                               t2ex);
      }
    }
  }
};

template <typename T>
auto getInstrumentParameter(const Mantid::Geometry::Instrument_const_sptr &instrument, std::string const &parameterName)
    -> decltype(InstrumentParameter<T>::get(std::declval<Mantid::Geometry::Instrument_const_sptr>(),
                                            std::declval<std::string const &>())) {
  return InstrumentParameter<T>::get(instrument, parameterName);
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
