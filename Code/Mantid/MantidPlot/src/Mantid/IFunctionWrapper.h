#ifndef IFUNCTIONWRAPPER_H
#define IFUNCTIONWRAPPER_H

#include <QObject>

namespace Mantid
{
  namespace API
  {
    class IFunction;
    class CompositeFunction;
    class IPeakFunction;
  }
}

/**
 * IFunctionWrapper is a wrapper for IFunction pointer which is a QObject
 * and can send and recieve signals.
 */
class IFunctionWrapper: public QObject
{
  Q_OBJECT
public:

  IFunctionWrapper():m_function(NULL),m_compositeFunction(NULL),m_peakFunction(NULL){}

  /// IFunction pointer
  Mantid::API::IFunction* function(){return m_function;}
  Mantid::API::CompositeFunction* compositeFunction(){return m_compositeFunction;}
  Mantid::API::IPeakFunction* peakFunction(){return m_peakFunction;}

  /// Set a new function from a string
  void setFunction(const QString& name);
  /// Set a new function from a pointer
  void setFunction(Mantid::API::IFunction* function);

private:
  /// Pointer to the function
  Mantid::API::IFunction* m_function;
  Mantid::API::CompositeFunction* m_compositeFunction;
  Mantid::API::IPeakFunction* m_peakFunction;
};

#endif /* IFUNCTIONWRAPPER_H */
