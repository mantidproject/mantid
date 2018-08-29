#ifndef IFUNCTIONWRAPPER_H
#define IFUNCTIONWRAPPER_H

#include <QObject>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {
class IFunction;
class CompositeFunction;
class IPeakFunction;
} // namespace API
} // namespace Mantid

/**
 * IFunctionWrapper is a wrapper for IFunction pointer which is a QObject
 * and can send and recieve signals.
 */
class IFunctionWrapper : public QObject {
  Q_OBJECT
public:
  IFunctionWrapper() : m_function(), m_compositeFunction(), m_peakFunction() {}

  /// IFunction pointer
  boost::shared_ptr<Mantid::API::IFunction> function() { return m_function; }
  boost::shared_ptr<Mantid::API::CompositeFunction> compositeFunction() {
    return m_compositeFunction;
  }
  boost::shared_ptr<Mantid::API::IPeakFunction> peakFunction() {
    return m_peakFunction;
  }

  /// Set a new function from a string
  void setFunction(const QString &name);
  /// Set a new function from a pointer
  void setFunction(boost::shared_ptr<Mantid::API::IFunction> function);

private:
  /// Pointer to the function
  boost::shared_ptr<Mantid::API::IFunction> m_function;
  boost::shared_ptr<Mantid::API::CompositeFunction> m_compositeFunction;
  boost::shared_ptr<Mantid::API::IPeakFunction> m_peakFunction;
};

#endif /* IFUNCTIONWRAPPER_H */
