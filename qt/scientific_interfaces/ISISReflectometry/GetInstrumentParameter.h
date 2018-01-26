#ifndef MANTID_ISISREFLECTOMETRY_GETINSTRUMENTPARAMETER_H
#define MANTID_ISISREFLECTOMETRY_GETINSTRUMENTPARAMETER_H
#include <string>
#include <vector>
#include <boost/variant.hpp>
#include "MantidGeometry/Instrument.h"
namespace MantidQt {
namespace CustomInterfaces {
template <typename T> class InstrumentParameter;

template <> class InstrumentParameter<std::string> {
public:
  static std::vector<std::string>
  get(Mantid::Geometry::Instrument_const_sptr instrument,
      std::string const &parameterName);
};

template <> class InstrumentParameter<double> {
public:
  static std::vector<double>
  get(Mantid::Geometry::Instrument_const_sptr instrument,
      std::string const &parameterName);
};

template <> class InstrumentParameter<int> {
public:
  static std::vector<int>
  get(Mantid::Geometry::Instrument_const_sptr instrument,
      std::string const &parameterName);
};

template <> class InstrumentParameter<bool> {
public:
  static std::vector<bool>
  get(Mantid::Geometry::Instrument_const_sptr instrument,
      std::string const &parameterName);
};

class InstrumentParameterTypeMissmatch : public std::runtime_error {
public:
  InstrumentParameterTypeMissmatch(const std::string &parameterName,
                                   const std::string &expectedType,
                                   const std::runtime_error &ex);

  std::string const &parameterName() const;
  std::string const &expectedType() const;
  std::string const &originalMessage() const;

private:
  std::string m_parameterName;
  std::string m_expectedType;
  std::string m_originalMessage;
};

template <typename T1, typename T2>
class InstrumentParameter<boost::variant<T1, T2>> {
public:
  static boost::variant<std::vector<T1>, std::vector<T2>>
  get(Mantid::Geometry::Instrument_const_sptr instrument,
      std::string const &parameterName) {
    try {
      return InstrumentParameter<T1>::get(instrument, parameterName);
    } catch (InstrumentParameterTypeMissmatch const &t1ex) {
      try {
        return InstrumentParameter<T2>::get(instrument, parameterName);
      } catch (InstrumentParameterTypeMissmatch const &t2ex) {
        throw InstrumentParameterTypeMissmatch(
            parameterName, t1ex.expectedType() + " or a " + t2ex.expectedType(),
            t2ex);
      }
    }
  }
};

template <typename T1, typename T2, typename T3, typename... Ts>
class InstrumentParameter<boost::variant<T1, T2, T3, Ts...>> {
public:
  static boost::variant<std::vector<T1>, std::vector<T2>, std::vector<T3>,
                        std::vector<Ts>...>
  get(Mantid::Geometry::Instrument_const_sptr instrument,
      std::string const &parameterName) {
    try {
      return InstrumentParameter<T1>::get(instrument, parameterName);
    } catch (InstrumentParameterTypeMissmatch const &t1ex) {
      try {
        return InstrumentParameter<boost::variant<T2, T3, Ts...>>::get(
            instrument, parameterName);
      } catch (InstrumentParameterTypeMissmatch const &t2ex) {
        throw InstrumentParameterTypeMissmatch(
            parameterName, t1ex.expectedType() + " or a " + t2ex.expectedType(),
            t2ex);
      }
    }
  }
};

template <typename T>
auto getInstrumentParameter(Mantid::Geometry::Instrument_const_sptr instrument,
                            std::string const &parameterName)
    -> decltype(InstrumentParameter<T>::get(
        std::declval<Mantid::Geometry::Instrument_const_sptr>(),
        std::declval<std::string const &>())) {
  return InstrumentParameter<T>::get(instrument, parameterName);
}
}
}
#endif // MANTID_ISISREFLECTOMETRY_GETINSTRUMENTPARAMETER_H
