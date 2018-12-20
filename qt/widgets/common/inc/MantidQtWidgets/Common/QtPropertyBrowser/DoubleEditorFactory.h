#ifndef DOUBLEEDITORFACTORY_H
#define DOUBLEEDITORFACTORY_H

#include "ParameterPropertyManager.h"
#include "qtpropertymanager.h"

#include <QLineEdit>

/**
 * Base class for double editor factories
 * @param DoubleManagerType :: Double manager class to use
 * @param DoubleEditorType :: Double editor class to create
 */
template <class DoubleManagerType, class DoubleEditorType>
class DoubleEditorFactoryBase
    : public QtAbstractEditorFactory<DoubleManagerType> {
public:
  DoubleEditorFactoryBase(QObject *parent = nullptr)
      : QtAbstractEditorFactory<DoubleManagerType>(parent) {}

protected:
  void connectPropertyManager(DoubleManagerType *manager) override {
    (void)manager; // Unused
    // Do nothing
  }

  void disconnectPropertyManager(DoubleManagerType *manager) override {
    (void)manager; // Unused
    // Do nothing
  }

  QWidget *createEditorForManager(DoubleManagerType *manager,
                                  QtProperty *property,
                                  QWidget *parent) override {
    (void)manager; // Unused

    return new DoubleEditorType(property, parent);
  }
};

/**
 *  Editor for double values
 */
class EXPORT_OPT_MANTIDQT_COMMON DoubleEditor : public QLineEdit {
  Q_OBJECT
public:
  DoubleEditor(QtProperty *property, QWidget *parent);
  ~DoubleEditor() override;
  void setValue(const double &d);
protected slots:
  virtual void updateProperty();

protected:
  /// Returns string representation of the value, using the format of the
  /// property
  QString formatValue(const double &d) const;

  QtProperty *m_property;
  int m_decimals;
};

/**
 *  Specialized version of double editor for parameters
 */
class EXPORT_OPT_MANTIDQT_COMMON ParameterEditor : public DoubleEditor {
  Q_OBJECT
public:
  ParameterEditor(QtProperty *property, QWidget *parent)
      : DoubleEditor(property, parent) {}
protected slots:
  void updateProperty() override;
};

/**
 * Concrete double editor factory for double properties
 */
class EXPORT_OPT_MANTIDQT_COMMON DoubleEditorFactory
    : public DoubleEditorFactoryBase<QtDoublePropertyManager, DoubleEditor> {
  Q_OBJECT
public:
  DoubleEditorFactory(QObject *parent = nullptr)
      : DoubleEditorFactoryBase<QtDoublePropertyManager, DoubleEditor>(parent) {
  }
};

/**
 * Concrete double editor factory for parameter properties
 */
class EXPORT_OPT_MANTIDQT_COMMON ParameterEditorFactory
    : public DoubleEditorFactoryBase<ParameterPropertyManager,
                                     ParameterEditor> {
  Q_OBJECT
public:
  ParameterEditorFactory(QObject *parent = nullptr)
      : DoubleEditorFactoryBase<ParameterPropertyManager, ParameterEditor>(
            parent) {}
};

#endif // DOUBLEEDITORFACTORY_H
