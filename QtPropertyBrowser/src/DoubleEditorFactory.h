#ifndef DOUBLEEDITORFACTORY_H
#define DOUBLEEDITORFACTORY_H

#include "qtpropertymanager.h"
#include "ParameterPropertyManager.h"

#include <QLineEdit>

/**
 * Base class for double editor factories
 * @param DoubleManagerType :: Double manager class to use
 * @param DoubleEditorType :: Double editor class to create
 */
template <class DoubleManagerType, class DoubleEditorType>
class DoubleEditorFactoryBase : public QtAbstractEditorFactory<DoubleManagerType>
{
public:
  DoubleEditorFactoryBase(QObject *parent = 0)
    : QtAbstractEditorFactory<DoubleManagerType>(parent)
  {}

protected:
  void connectPropertyManager(DoubleManagerType *manager)
  {
    (void) manager; // Unused
    // Do nothing
  }

  void disconnectPropertyManager(DoubleManagerType *manager)
  {
    (void) manager; // Unused
    // Do nothing
  }

  QWidget *createEditorForManager(DoubleManagerType *manager, QtProperty *property, QWidget *parent)
  {
    (void) manager; // Unused

    return new DoubleEditorType(property, parent);
  }
};

/**
 *  Editor for double values
 */
class QT_QTPROPERTYBROWSER_EXPORT DoubleEditor: public QLineEdit
{
  Q_OBJECT
public:
  DoubleEditor(QtProperty *property, QWidget *parent);
  ~DoubleEditor();
  void setValue(const double& d);
protected slots:
  virtual void updateProperty();
protected:
  /// Returns string representation of the value, using the format of the property
  QString formatValue(const double& d) const;

  QtProperty* m_property;
  int m_decimals;
};

/**
 *  Specialized version of double editor for parameters
 */
class QT_QTPROPERTYBROWSER_EXPORT ParameterEditor : public DoubleEditor
{
  Q_OBJECT
public:
  ParameterEditor(QtProperty *property, QWidget *parent) : DoubleEditor(property, parent) {}
protected slots:
  virtual void updateProperty();
};

/**
 * Concrete double editor factory for double properties
 */
class QT_QTPROPERTYBROWSER_EXPORT DoubleEditorFactory : public DoubleEditorFactoryBase<QtDoublePropertyManager,DoubleEditor>
{
  Q_OBJECT
public:
  DoubleEditorFactory(QObject* parent = 0)
    : DoubleEditorFactoryBase<QtDoublePropertyManager,DoubleEditor>(parent) {}
};

/**
 * Concrete double editor factory for parameter properties
 */
class QT_QTPROPERTYBROWSER_EXPORT ParameterEditorFactory : public DoubleEditorFactoryBase<ParameterPropertyManager, ParameterEditor>
{
  Q_OBJECT
public:
  ParameterEditorFactory(QObject* parent = 0)
    : DoubleEditorFactoryBase<ParameterPropertyManager, ParameterEditor>(parent) {}
};

#endif // DOUBLEEDITORFACTORY_H
