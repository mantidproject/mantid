#ifndef FITPARAMETERTIE_H
#define FITPARAMETERTIE_H

#include <QString>
#include <QList>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace API
  {
    class CompositeFunction;
  }
}

class QtProperty;

/**
 * Class FitParameterTie is for editing parameter ties in Mantid functions.
 */
class FitParameterTie
{
public:
  /// Constructor
  FitParameterTie(boost::shared_ptr<Mantid::API::CompositeFunction> cf);
  /// Destructor
  ~FitParameterTie();
  /// Set the tying expression, e.g. "f1.Sigma = 2*f0.Sigma + 1"
  void set(const QString& estr);
  /// The tying expression
  QString expr(bool removePrefix=false)const;
  /// The parameter name
  QString parName()const;
  /// Returns the right-hand side of the expression
  QString exprRHS()const;
  /// Mofifies the function indeces in response to insertion of a new function into 
  /// the composite function
  void functionInserted(int i);
  /// Mofifies the function indeces in response to deletion of a function from
  /// the composite function
  bool functionDeleted(int i);
  /// Set property
  void setProperty(QtProperty* prop){m_prop=prop;}
  /// Get property
  QtProperty* getProperty()const{return m_prop;}
private:
  /// The tying expression
  QString m_expr;
  /// Function indeces used in the expression
  QList<int> m_iFunctions;
  /// A copy of the edited function
  boost::shared_ptr<Mantid::API::CompositeFunction> m_compositeFunction;
  /// The property
  QtProperty* m_prop;
};

#endif /* FITPARAMETERTIE_H */
