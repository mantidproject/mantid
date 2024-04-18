// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/IFunction.h"

/* Forward declarations */
namespace Mantid {
namespace API {
class IPeakFunction;
class CompositeFunction;
} // namespace API
} // namespace Mantid

// class FunctionCurve;
// class PlotCurve;
// class Graph;
class QtBrowserItem;
class QtProperty;

/* Qt includes */
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

namespace MantidQt {
namespace MantidWidgets {
class FitPropertyBrowser;

/**
 * Helps display and edit functions in FitPropertyBrowser
 */
class EXPORT_OPT_MANTIDQT_COMMON PropertyHandler : public QObject, public Mantid::API::FunctionHandler {
  Q_OBJECT
public:
  // Constructor
  PropertyHandler(const Mantid::API::IFunction_sptr &fun, std::shared_ptr<Mantid::API::CompositeFunction> parent,
                  FitPropertyBrowser *browser, QtBrowserItem *item = nullptr);

  /// Destructor
  ~PropertyHandler() override;

  /// overrides virtual init() which is called from IFunction::setHandler(...)
  void init() override;

  PropertyHandler *addFunction(const std::string &fnName);
  // Removes handled function from its parent function and
  // properties from the browser
  void removeFunction();

  void renameChildren(const Mantid::API::CompositeFunction &cf);

  /// Creates name for this function to be displayed
  /// in the browser
  QString functionName() const;

  QString functionPrefix() const;

  // Return composite function
  std::shared_ptr<Mantid::API::CompositeFunction> cfun() const { return m_cf; }
  // Return peak function
  std::shared_ptr<Mantid::API::IPeakFunction> pfun() const { return m_pf; }
  // Return IFunction
  std::shared_ptr<Mantid::API::IFunction> ifun() const { return m_fun; }
  // Return the browser item
  QtBrowserItem *item() const { return m_item; }
  // Return the parent handler
  PropertyHandler *parentHandler() const;
  // Return the child's handler
  PropertyHandler *getHandler(std::size_t i) const;
  /** Returns 'this' if item == m_item and this is a composite function or
   * calls findCompositeFunction recursively with all its children or
   * zero
   */
  std::shared_ptr<const Mantid::API::CompositeFunction> findCompositeFunction(QtBrowserItem *item) const;
  /** Returns 'this' if item == m_item or
   * calls findFunction recursively with all its children or
   * zero
   */
  std::shared_ptr<const Mantid::API::IFunction> findFunction(QtBrowserItem *item) const;

  PropertyHandler *findHandler(const Mantid::API::IFunction *fun);
  PropertyHandler *findHandler(const Mantid::API::IFunction_const_sptr &fun);
  PropertyHandler *findHandler(QtProperty *prop);

  /**
   * Set function parameter value read from a QtProperty
   * @param prop :: The (double) property with the new parameter value
   * @return true if successfull
   */
  bool setParameter(QtProperty *prop);

  // Check if it is a parameter property
  bool isParameter(QtProperty *prop);

  /// Set function attribute value read from a QtProperty
  bool setAttribute(QtProperty *prop, bool resetProperties = true);

  /// Set function attribute value
  void setAttribute(QString const &attName, Mantid::API::IFunction::Attribute const &attValue);

  /// Set function's attribute if it has type double or int
  template <typename AttributeType> void setAttribute(QString const &attName, AttributeType const &attValue);

  /// Set function's attribute of any type.
  void setAttribute(const QString &attName, const QString &attValue);

  /// Set function vector attribute value
  void setVectorAttribute(QtProperty *prop);

  /// Sync all parameter values with the manager
  void updateParameters();

  /// Sync all parameter values with the manager
  void updateAttributes();

  /// Set all parameter error values in the manager
  void updateErrors();

  /// Clear all parameter error values in the manager
  void clearErrors();

  // Get property for function parameter parName
  QtProperty *getParameterProperty(const QString &parName) const;

  // Get parameter property which has the argument as a child (i.e. tie or
  // conatraint)
  QtProperty *getParameterProperty(QtProperty *prop) const;

  /**
   * Change the type of the function (replace the function)
   * @param prop :: The "Type" property with new value
   */
  std::shared_ptr<Mantid::API::IFunction> changeType(QtProperty *prop);

  void setHeight(const double &h);
  void setCentre(const double &c);
  void setFwhm(const double &w);
  void setBase(const double &b) { m_base = b; }
  void calcBase();    //< caclulate baseline from workspace data
  void calcBaseAll(); //< calc baseline for all peaks in the function
  /// Estimate the FwHM for a peak
  double EstimateFwhm() const;

  double height() const;
  double centre() const;
  double fwhm() const;
  std::string getWidthParameterName() const;
  std::string getCentreParameterName() const;
  bool isParameterExplicitlySet(const std::string &param) const;
  double base() const { return m_base; }

  void addTie(const QString &tieStr);
  void fix(const QString &parName);
  void removeTie(QtProperty *prop, const std::string &globalName);
  void removeTie(QtProperty *prop);
  void removeTie(const QString &propName);
  void addConstraint(QtProperty *parProp, bool lo, bool up, double loBound, double upBound);
  void removeConstraint(QtProperty *parProp);

  // Return list of handlers of peak functions which can be used in
  // PeakPickerTool
  // The item->pfun() will return a correct pointer to a peak
  // Non-const because it may return a non-const pointer to this.
  QList<PropertyHandler *> getPeakList();

  // Plot the function on a graph
  bool hasPlot() const { return m_hasPlot; }
  void setHasPlot(const bool state) { m_hasPlot = state; };
  void removeAllPlots();

  void fit();

  // update workspace property when workspaces added to or removed from ADS
  void updateWorkspaces(const QStringList &oldWorkspaces);
  // set workspace in workspace property to the function
  void setFunctionWorkspace();

  /// Update high-level structure tooltip and return it
  QString updateStructureTooltip();

  QMap<QString, QtProperty *> getTies() { return m_ties; };
  bool hasTies() { return !m_ties.isEmpty(); };

protected slots:

  //
  void plotRemoved();

protected:
  void initAttributes();
  void initParameters();
  void initWorkspace();
  void initTies();

private:
  FitPropertyBrowser *m_browser;
  std::shared_ptr<Mantid::API::CompositeFunction> m_cf;     //< if the function is composite holds pointer to it
  std::shared_ptr<Mantid::API::IPeakFunction> m_pf;         //< if the function is peak holds pointer to it
  std::shared_ptr<Mantid::API::CompositeFunction> m_parent; //< if the function has parent holds pointer to it
  QtProperty *m_type;
  QtBrowserItem *m_item;                                              //< the browser item
  QList<QtProperty *> m_attributes;                                   //< function attribute properties
  QList<QtProperty *> m_parameters;                                   //< function parameter properties
  QMap<QString, QtProperty *> m_ties;                                 //< tie properties
  QMap<QString, std::pair<QtProperty *, QtProperty *>> m_constraints; //< constraints
  QList<QtProperty *> m_vectorMembers;                                //< vector member properties
  QList<QtProperty *> m_vectorSizes;                                  //< vector size properties
  bool m_isMultispectral; ///< true if fitting to multiple spectra using MultiBG
  /// function
  QtProperty *m_workspace;      ///< workspace name for multispectral fitting
  QtProperty *m_workspaceIndex; ///< workspace index for multispectral fitting
  double m_base;                //< the baseline for a peak
  int m_ci;                     //< approximate index in the workspace at the peak centre
  // mutable FunctionCurve* m_curve;//< the curve to plot the handled function
  mutable bool m_hasPlot;

  /// Sync function parameter value with the manager
  void updateParameter(QtProperty *prop);

  /// Set function parameter error in the manager
  void updateError(QtProperty *prop);

  /// Clear function parameter error in the manager
  void clearError(QtProperty *prop);

  /// Applies given function to all the parameter properties recursively
  void applyToAllParameters(void (PropertyHandler::*func)(QtProperty *));

  /// Sync function attribute value with the manager
  void updateAttribute(QtProperty *prop);

  /// Applies given function to all the attribute properties recursively
  void applyToAllAttributes(void (PropertyHandler::*func)(QtProperty *));

  friend class CreateAttributeProperty;
};

} // namespace MantidWidgets
} // namespace MantidQt
