#ifndef MANTID_ISISREFLECTOMETRY_GETINSTRUMENTPARAMETER_H
#define MANTID_ISISREFLECTOMETRY_GETINSTRUMENTPARAMETER_H
#include <string>
#include <vector>
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

using MissingInstrumentParameterValue = std::string;

template <typename T>
std::vector<T>
getInstrumentParameter(Mantid::Geometry::Instrument_const_sptr instrument,
                       std::string const &parameterName) {
  return InstrumentParameter<T>::get(instrument, parameterName);
}
}
}
#endif // MANTID_ISISREFLECTOMETRY_GETINSTRUMENTPARAMETER_H
