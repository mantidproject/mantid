#include "FitParameterTie.h"
#include "MantidAPI/CompositeFunction.h"
#include <QRegExp>
#include <stdexcept>
#include <iostream>

/// Constructor
FitParameterTie::FitParameterTie(boost::shared_ptr<Mantid::API::CompositeFunction> cf)
:m_compositeFunction(cf),m_prop(0)
{}

/// Destructor
FitParameterTie::~FitParameterTie()
{
  if (m_prop)
  {
    //delete m_prop;
  }
}

/** Set the tying expression. The function names (f0,f1,f2,...) are changed to
 * placeholders (#0,#1,#2) to make it easier to edit afterwards.
 * @param estr The tying expression , e.g. "f1.Sigma = 2*f0.Sigma + 1"
 */
void FitParameterTie::set(const QString& estr)
{
  int ieq = estr.indexOf('=');
  if (ieq < 0)
  {
    throw std::invalid_argument("The tie expression doesn't contain the tied parameter.\n"
      "Syntax: <tied_name> = <tying_expression>");
  }

  if (ieq == estr.size())
  {
    throw std::invalid_argument("The tying expression is missing.\n"
      "Syntax: <tied_name> = <tying_expression>");
  }

  if (estr.mid(ieq+1).trimmed().isEmpty())
  {
    throw std::invalid_argument("The tying expression is missing.\n"
      "Syntax: <tied_name> = <tying_expression>");
  }

  QString parName = estr.left(ieq).trimmed();

  // rx matches function identifiers in the parameter names and captures the function index:
  // for f12.Sigma rx.cap(1).toInt() returns 12
  QRegExp rx("\\bf(\\d+)\\.");

  if (rx.indexIn(parName) < 0)
  {
    throw std::invalid_argument("Parameter names must contain function identifiers:\n"
      "e.g. f0.Sigma, f5.HWHM");
  }

  m_expr = estr;
  for(int i=rx.indexIn(m_expr);i>=0;)
  {
    int iFun = rx.cap(1).toInt();
    int j = m_iFunctions.indexOf(iFun);
    if (j < 0)
    {
      j = m_iFunctions.size();
      m_iFunctions.append(iFun);
    }
    QString s = "#"+QString::number(j)+".";
    m_expr.replace(rx.pos(),rx.cap().size(),s);
    i=rx.indexIn(m_expr,i+rx.cap().size());
  }

}

/// The tying expression
QString FitParameterTie::expr(bool removePrefix)const
{
  QString str = m_expr;
  for(int j=0;j<m_iFunctions.size();j++)
  {
    QString ph = "#"+QString::number(j);
    QString fi;
    if (removePrefix)
    {
      ph += ".";
      fi = "";
    }
    else
    {
      fi = "f"+QString::number(m_iFunctions[j]);
    }
    str.replace(ph,fi);
  }
  return str;
}

/// The parameter name
QString FitParameterTie::parName()const
{
  QString str = m_expr.left(m_expr.indexOf('=')).trimmed();
  for(int j=0;j<m_iFunctions.size();j++)
  {
    QString ph = "#"+QString::number(j);
    QString fi = "f"+QString::number(m_iFunctions[j]);
    str.replace(ph,fi);
  }
  return str;
}

/// Returns the right-hand side of the expression
QString FitParameterTie::exprRHS()const
{
  QString ex = expr();
  int ieq = ex.indexOf('=');
  if (ieq<0)
  {
    return ex;
  }
  if (ieq==ex.size()-1)
  {
    return "";
  }
  return ex.mid(ieq+1);
}

/**
 * When a new function is added the function indeces in the tying expression must
 * be changed.
 * @param i The index at wich the function is inserted. All old indeces starting
 *   from i (inclusive) must be incremented.
 */
void FitParameterTie::functionInserted(int i)
{
  for(int j=0;j<m_iFunctions.size();j++)
  {
    if (m_iFunctions[j] >= i)
    {
      m_iFunctions[j]++;
    }
  }

}

/**
 * When a function is deleted the function indeces in the tying expression must
 * be changed or the tie may become invalid if the deleted function is used in the tie.
 * @param i The index of the deleted function. All old indeces starting
 *   from i+1 must be decremented.
 * @return true if the tie remains valid and false otherwise.
 */
bool FitParameterTie::functionDeleted(int i)
{
  for(int j=0;j<m_iFunctions.size();j++)
  {
    if (m_iFunctions[j] == i)
    {
      return false;
    }
    if (m_iFunctions[j] > i)
    {
      m_iFunctions[j]--;
    }
  }
  return true;
}
