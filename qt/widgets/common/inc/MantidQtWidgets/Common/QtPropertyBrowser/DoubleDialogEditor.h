#ifndef DOUBLEDIALOGEDITORFACTORY_H
#define DOUBLEDIALOGEDITORFACTORY_H

#include "DoubleEditorFactory.h"
#include "ParameterPropertyManager.h"

class QLineEdit;
class QPushButton;

/**
 * Partially implemented string editor. It has a QLineEdit for manual editing
 * and a button [...] next to it
 * to call a dialog for more complex editing. Clicking the button calls virtual
 * runDialog() method.
 * Concrete classes must implement it.
 */
class DoubleDialogEditor : public QWidget {
  Q_OBJECT
public:
  DoubleDialogEditor(QtProperty *property, QWidget *parent);
signals:
  void buttonClicked(QtProperty *);
  void closeEditor();

protected slots:
  /// Implementations must open a dialog to edit the editor's text. If editing
  /// is successful
  /// setText() and updateProperty() methods must be called.
  virtual void runDialog();
  void updateProperty();
  void setText(const QString &txt);
  QString getText() const;

private:
  bool eventFilter(QObject *, QEvent *) override;
  DoubleEditor *m_editor;
  QPushButton *m_button;
  QtProperty *m_property;
};

/**
 * An abstract editor factory to be used with QtPropertyBrowser. Implementations
 * need to
 * implement QWidget *createEditor(QtStringPropertyManager *manager, QtProperty
 * *property,QWidget *parent)
 * method which creates a specific editor. The underlying type of the edited
 * property must be string.
 */
class DoubleDialogEditorFactory
    : public QtAbstractEditorFactory<ParameterPropertyManager> {
  Q_OBJECT
public:
  DoubleDialogEditorFactory(QObject *parent = nullptr)
      : QtAbstractEditorFactory<ParameterPropertyManager>(parent) {}
  QWidget *createEditorForManager(ParameterPropertyManager *,
                                  QtProperty *property,
                                  QWidget *parent) override {
    auto editor = new DoubleDialogEditor(property, parent);
    connect(editor, SIGNAL(buttonClicked(QtProperty *)), this,
            SIGNAL(buttonClicked(QtProperty *)));
    connect(editor, SIGNAL(closeEditor()), this, SIGNAL(closeEditor()),
            Qt::QueuedConnection);
    return editor;
  }
signals:
  void buttonClicked(QtProperty *);
  void closeEditor();

protected:
  void connectPropertyManager(ParameterPropertyManager *) override {}
  void disconnectPropertyManager(ParameterPropertyManager *) override {}
};

#endif // DOUBLEDIALOGEDITORFACTORY_H
