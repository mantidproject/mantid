#ifndef PROPERTY_HANDLER_H
#define PROPERTY_HANDLER_H

#include "MantidAPI/IFitFunction.h"

/* Forward declarations */
namespace Mantid
{
  namespace API
  {
    class IPeakFunction;
    class CompositeFunction;
  }
}
class FitPropertyBrowser;
class FunctionCurve;
class PlotCurve;
class Graph;
class QtBrowserItem;
class QtProperty;

/* Qt includes */
#include <QString>
#include <QList>
#include <QMap>
#include <QObject>

/**
 * Helps display and edit functions in FitPropertyBrowser
 */
class PropertyHandler:public QObject, public Mantid::API::FitFunctionHandler
{
  Q_OBJECT
public:
  // Constructor
  PropertyHandler(Mantid::API::IFitFunction* fun,
                  Mantid::API::CompositeFunction* parent,
                  FitPropertyBrowser* browser,
                  QtBrowserItem* item = NULL);

  /// Destructor
  ~PropertyHandler();

  /// overrides virtual init() which is called from IFitFunction::setHandler(...)
  void init();

  void initAttributes();
  void initParameters();

  PropertyHandler* addFunction(const std::string& fnName);
  // Removes handled function from its parent function and 
  // properties from the browser
  void removeFunction();

  void renameChildren()const;

  /// Creates name for this function to be displayed
  /// in the browser
  QString functionName()const;

  QString functionPrefix()const;

  // Return composite function
  Mantid::API::CompositeFunction* cfun()const{return m_cf;}
  // Return peak function
  Mantid::API::IPeakFunction* pfun()const{return m_pf;}
  // Return the browser item
  QtBrowserItem* item()const{return m_item;}
  // Return the parent handler
  PropertyHandler* parentHandler()const;
  // Return the child's handler
  PropertyHandler* getHandler(int i)const;
  /** Returns 'this' if item == m_item and this is a composite function or
   * calls findCompositeFunction recursively with all its children or
   * zero
   */
  const Mantid::API::CompositeFunction* findCompositeFunction(QtBrowserItem* item)const;
  /** Returns 'this' if item == m_item or
   * calls findFunction recursively with all its children or
   * zero
   */
  const Mantid::API::IFitFunction* findFunction(QtBrowserItem* item)const;

  PropertyHandler* findHandler(QtProperty* prop);

  PropertyHandler* findHandler(const Mantid::API::IFitFunction* fun);

  /**
   * Set function parameter value read from a QtProperty
   * @param prop :: The (double) property with the new parameter value
   * @return true if successfull
   */
  bool setParameter(QtProperty* prop);

  // Check if it is a parameter property
  bool isParameter(QtProperty* prop);

  /**
   * Set function attribute value read from a QtProperty
   * @param prop :: The (string) property with the new attribute value
   * @return true if successfull
   */
  bool setAttribute(QtProperty* prop);

  /**
   * Set function's double attribute
   * @param attName :: The name of the attribute
   * @param attValue :: The new attribute value
   */
  void setAttribute(const QString& attName, const double& attValue);

  /**
   * Set function's attribute of any type.
   * @param attName :: The name of the attribute
   * @param attValue :: The new attribute value as a string. If the attValue's
   *  format doesn't match the attribute's type it is ignored.
   */
  void setAttribute(const QString& attName, const QString& attValue);

  /**
   * Update the parameter properties
   */
  void updateParameters();

  // Get property for function parameter parName
  QtProperty* getParameterProperty(const QString& parName)const;

  // Get parameter property which has the argument as a child (i.e. tie or conatraint)
  QtProperty* getParameterProperty(QtProperty* prop)const;

  /**
   * Change the type of the function (replace the function)
   * @param prop :: The "Type" property with new value
   * @param fnName :: New function name (type) or full initialization expression
   */
  Mantid::API::IFitFunction* changeType(QtProperty* prop);

  void setHeight(const double& h);
  void setCentre(const double& c);
  void setWidth(const double& w);
  void setBase(const double& b){m_base = b;}
  void calcBase();//< caclulate baseline from workspace data
  void calcBaseAll();//< calc baseline for all peaks in the function

  double height()const;
  double centre()const;
  double width()const;
  double base()const{return m_base;}

  void addTie(const QString& tieStr);
  void fix(const QString& parName);
  void removeTie(QtProperty* prop);
  void removeTie(const QString& propName);

  void addConstraint(QtProperty* parProp,bool lo,bool up,double loBound,double upBound);
  void removeConstraint(QtProperty* parProp);

  // Return list of handlers of peak functions which can be used in PeakPickerTool
  // The item->pfun() will return a correct pointer to a peak
  // Non-const because it may return a non-const pointer to this.
  QList<PropertyHandler*> getPeakList();

  // Plot the function on a graph
  void plot(Graph* g)const;
  bool hasPlot()const{return m_curve != NULL;}
  void replot()const;
  void removePlot();
  void removeAllPlots();

  void fit();

protected slots:

  // 
  void plotRemoved(PlotCurve*);

private:
  FitPropertyBrowser* m_browser;
  Mantid::API::CompositeFunction* m_cf;//< if the function is composite holds pointer to it
  Mantid::API::IPeakFunction* m_pf;//< if the function is peak holds pointer to it
  Mantid::API::CompositeFunction* m_parent; //< if the function has parent holds pointer to it
  Mantid::API::IFitFunction* m_if;//< pointer to IFitFunction
  QtProperty* m_type;
  QtBrowserItem* m_item;//< the browser item
  QList<QtProperty*> m_attributes; //< function attribute properties
  QList<QtProperty*> m_parameters; //< function parameter properties
  QMap<QString,QtProperty*> m_ties;//< tie properties
  QMap<QString,std::pair<QtProperty*,QtProperty*> > m_constraints;//< constraints
  double m_base; //< the baseline for a peak
  int m_ci; //< approximate index in the workspace at the peak centre
  mutable FunctionCurve* m_curve;//< the curve to plot the handled function
};

#endif /* PROPERTY_HANDLER_H */
