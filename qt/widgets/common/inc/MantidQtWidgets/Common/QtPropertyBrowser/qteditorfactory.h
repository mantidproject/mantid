/****************************************************************************
**
** This file is part of a Qt Solutions component.
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Solutions Commercial License Agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** Please note Third Party Software included with Qt Solutions may impose
** additional restrictions and it is the user's responsibility to ensure
** that they have met the licensing requirements of the GPL, LGPL, or Qt
** Solutions Commercial license and the relevant license of the Third
** Party Software they are using.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
****************************************************************************/

/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include "qtpropertybrowserutils_p.h"
#include "qtpropertymanager.h"

#include <QApplication>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QLineEdit>
#include <QScrollBar>
#include <QSpinBox>
#include <QTimerEvent>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QColor;
class QLabel;
class QToolButton;

template <class SpinBox> class QtSpinBoxFactoryPrivateBase;
class QtSpinBoxFactoryPrivate;
class QtSpinBoxFactoryNoTimerPrivate;

// Base QtSpinBoxFactory class
template <class SpinBox> class QtSpinBoxFactoryBase : public QtAbstractEditorFactory<QtIntPropertyManager> {
public:
  QtSpinBoxFactoryBase(QObject *parent = nullptr) : QtAbstractEditorFactory<QtIntPropertyManager>(parent) {};

protected:
  void connectPropertyManager(QtIntPropertyManager *manager) override;
  QWidget *createEditorForManager(QtIntPropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtIntPropertyManager *manager) override;
  QtSpinBoxFactoryPrivateBase<SpinBox> *d_ptr;
  friend class QtSpinBoxFactoryPrivateBase<SpinBox>;
  void initializeQPtr() { d_ptr->q_ptr = this; }
};
/**
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
template <class SpinBox> void QtSpinBoxFactoryBase<SpinBox>::connectPropertyManager(QtIntPropertyManager *manager) {
  connect(manager, SIGNAL(valueChanged(QtProperty *, int)), this, SLOT(slotPropertyChanged(QtProperty *, int)));
  connect(manager, SIGNAL(rangeChanged(QtProperty *, int, int)), this, SLOT(slotRangeChanged(QtProperty *, int, int)));
  connect(manager, SIGNAL(singleStepChanged(QtProperty *, int)), this, SLOT(slotSingleStepChanged(QtProperty *, int)));
}

/**
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
template <class SpinBox>
QWidget *QtSpinBoxFactoryBase<SpinBox>::createEditorForManager(QtIntPropertyManager *manager, QtProperty *property,
                                                               QWidget *parent) {
  auto *editor = d_ptr->createEditor(property, parent);
  editor->setSingleStep(manager->singleStep(property));
  editor->setRange(manager->minimum(property), manager->maximum(property));
  editor->setValue(manager->value(property));
  editor->setKeyboardTracking(false);

  connect(editor, SIGNAL(valueChanged(int)), this, SLOT(slotSetValue(int)));
  connect(editor, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));
  return editor;
}

/**
    \internal

    Reimplemented from the QtAbstractEditorFactory class.
*/
template <class SpinBox> void QtSpinBoxFactoryBase<SpinBox>::disconnectPropertyManager(QtIntPropertyManager *manager) {
  disconnect(manager, SIGNAL(valueChanged(QtProperty *, int)), this, SLOT(slotPropertyChanged(QtProperty *, int)));
  disconnect(manager, SIGNAL(rangeChanged(QtProperty *, int, int)), this,
             SLOT(slotRangeChanged(QtProperty *, int, int)));
  disconnect(manager, SIGNAL(singleStepChanged(QtProperty *, int)), this,
             SLOT(slotSingleStepChanged(QtProperty *, int)));
}

class QSpinBoxNoTimer : public QSpinBox {
  Q_OBJECT
public:
  QSpinBoxNoTimer(QWidget *parent = nullptr) : QSpinBox(parent) {};

private:
  void timerEvent(QTimerEvent *event) override {
    // Override the timer event method and check if the user is actually holding
    // the mouse buttons down
    qApp->processEvents();
    if (QApplication::mouseButtons() & Qt::LeftButton)
      QSpinBox::timerEvent(event);
  }
};

class EXPORT_OPT_MANTIDQT_COMMON QtSpinBoxFactoryNoTimer : public QtSpinBoxFactoryBase<QSpinBoxNoTimer> {
  Q_OBJECT
public:
  QtSpinBoxFactoryNoTimer(QObject *parent = nullptr);
  ~QtSpinBoxFactoryNoTimer() override;

private:
  Q_DECLARE_PRIVATE(QtSpinBoxFactoryNoTimer)
  Q_DISABLE_COPY(QtSpinBoxFactoryNoTimer)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(QtProperty *, int, int))
  Q_PRIVATE_SLOT(d_func(), void slotSingleStepChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(int))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class EXPORT_OPT_MANTIDQT_COMMON QtSpinBoxFactory : public QtSpinBoxFactoryBase<QSpinBox> {
  Q_OBJECT
public:
  QtSpinBoxFactory(QObject *parent = nullptr);
  ~QtSpinBoxFactory() override;

private:
  Q_DECLARE_PRIVATE(QtSpinBoxFactory)
  Q_DISABLE_COPY(QtSpinBoxFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(QtProperty *, int, int))
  Q_PRIVATE_SLOT(d_func(), void slotSingleStepChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(int))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtSliderFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtSliderFactory : public QtAbstractEditorFactory<QtIntPropertyManager> {
  Q_OBJECT
public:
  QtSliderFactory(QObject *parent = nullptr);
  ~QtSliderFactory() override;

protected:
  void connectPropertyManager(QtIntPropertyManager *manager) override;
  QWidget *createEditorForManager(QtIntPropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtIntPropertyManager *manager) override;

private:
  QtSliderFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtSliderFactory)
  Q_DISABLE_COPY(QtSliderFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(QtProperty *, int, int))
  Q_PRIVATE_SLOT(d_func(), void slotSingleStepChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(int))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtScrollBarFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtScrollBarFactory : public QtAbstractEditorFactory<QtIntPropertyManager> {
  Q_OBJECT
public:
  QtScrollBarFactory(QObject *parent = nullptr);
  ~QtScrollBarFactory() override;

protected:
  void connectPropertyManager(QtIntPropertyManager *manager) override;
  QWidget *createEditorForManager(QtIntPropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtIntPropertyManager *manager) override;

private:
  QtScrollBarFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtScrollBarFactory)
  Q_DISABLE_COPY(QtScrollBarFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(QtProperty *, int, int))
  Q_PRIVATE_SLOT(d_func(), void slotSingleStepChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(int))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtCheckBoxFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtCheckBoxFactory : public QtAbstractEditorFactory<QtBoolPropertyManager> {
  Q_OBJECT
public:
  QtCheckBoxFactory(QObject *parent = nullptr);
  ~QtCheckBoxFactory() override;

protected:
  void connectPropertyManager(QtBoolPropertyManager *manager) override;
  QWidget *createEditorForManager(QtBoolPropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtBoolPropertyManager *manager) override;

private:
  QtCheckBoxFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtCheckBoxFactory)
  Q_DISABLE_COPY(QtCheckBoxFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, bool))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(bool))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtDoubleSpinBoxFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtDoubleSpinBoxFactory : public QtAbstractEditorFactory<QtDoublePropertyManager> {
  Q_OBJECT
public:
  QtDoubleSpinBoxFactory(QObject *parent = nullptr);
  ~QtDoubleSpinBoxFactory() override;

protected:
  void connectPropertyManager(QtDoublePropertyManager *manager) override;
  QWidget *createEditorForManager(QtDoublePropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtDoublePropertyManager *manager) override;

private:
  QtDoubleSpinBoxFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtDoubleSpinBoxFactory)
  Q_DISABLE_COPY(QtDoubleSpinBoxFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, double))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(QtProperty *, double, double))
  Q_PRIVATE_SLOT(d_func(), void slotSingleStepChanged(QtProperty *, double))
  Q_PRIVATE_SLOT(d_func(), void slotDecimalsChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(double))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtLineEditFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtLineEditFactory : public QtAbstractEditorFactory<QtStringPropertyManager> {
  Q_OBJECT
public:
  QtLineEditFactory(QObject *parent = nullptr);
  ~QtLineEditFactory() override;

protected:
  void connectPropertyManager(QtStringPropertyManager *manager) override;
  QWidget *createEditorForManager(QtStringPropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtStringPropertyManager *manager) override;

private:
  QtLineEditFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtLineEditFactory)
  Q_DISABLE_COPY(QtLineEditFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QString &))
  Q_PRIVATE_SLOT(d_func(), void slotRegExpChanged(QtProperty *, const QRegExp &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QString &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtDateEditFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtDateEditFactory : public QtAbstractEditorFactory<QtDatePropertyManager> {
  Q_OBJECT
public:
  QtDateEditFactory(QObject *parent = nullptr);
  ~QtDateEditFactory() override;

protected:
  void connectPropertyManager(QtDatePropertyManager *manager) override;
  QWidget *createEditorForManager(QtDatePropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtDatePropertyManager *manager) override;

private:
  QtDateEditFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtDateEditFactory)
  Q_DISABLE_COPY(QtDateEditFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QDate &))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(QtProperty *, const QDate &, const QDate &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QDate &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtTimeEditFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtTimeEditFactory : public QtAbstractEditorFactory<QtTimePropertyManager> {
  Q_OBJECT
public:
  QtTimeEditFactory(QObject *parent = nullptr);
  ~QtTimeEditFactory() override;

protected:
  void connectPropertyManager(QtTimePropertyManager *manager) override;
  QWidget *createEditorForManager(QtTimePropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtTimePropertyManager *manager) override;

private:
  QtTimeEditFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtTimeEditFactory)
  Q_DISABLE_COPY(QtTimeEditFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QTime &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QTime &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtDateTimeEditFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtDateTimeEditFactory : public QtAbstractEditorFactory<QtDateTimePropertyManager> {
  Q_OBJECT
public:
  QtDateTimeEditFactory(QObject *parent = nullptr);
  ~QtDateTimeEditFactory() override;

protected:
  void connectPropertyManager(QtDateTimePropertyManager *manager) override;
  QWidget *createEditorForManager(QtDateTimePropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtDateTimePropertyManager *manager) override;

private:
  QtDateTimeEditFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtDateTimeEditFactory)
  Q_DISABLE_COPY(QtDateTimeEditFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QDateTime &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QDateTime &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtKeySequenceEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtKeySequenceEditorFactory
    : public QtAbstractEditorFactory<QtKeySequencePropertyManager> {
  Q_OBJECT
public:
  QtKeySequenceEditorFactory(QObject *parent = nullptr);
  ~QtKeySequenceEditorFactory() override;

protected:
  void connectPropertyManager(QtKeySequencePropertyManager *manager) override;
  QWidget *createEditorForManager(QtKeySequencePropertyManager *manager, QtProperty *property,
                                  QWidget *parent) override;
  void disconnectPropertyManager(QtKeySequencePropertyManager *manager) override;

private:
  QtKeySequenceEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtKeySequenceEditorFactory)
  Q_DISABLE_COPY(QtKeySequenceEditorFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QKeySequence &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QKeySequence &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtCharEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtCharEditorFactory : public QtAbstractEditorFactory<QtCharPropertyManager> {
  Q_OBJECT
public:
  QtCharEditorFactory(QObject *parent = nullptr);
  ~QtCharEditorFactory() override;

protected:
  void connectPropertyManager(QtCharPropertyManager *manager) override;
  QWidget *createEditorForManager(QtCharPropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtCharPropertyManager *manager) override;

private:
  QtCharEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtCharEditorFactory)
  Q_DISABLE_COPY(QtCharEditorFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QChar &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QChar &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtEnumEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtEnumEditorFactory : public QtAbstractEditorFactory<QtEnumPropertyManager> {
  Q_OBJECT
public:
  QtEnumEditorFactory(QObject *parent = nullptr);
  ~QtEnumEditorFactory() override;

protected:
  void connectPropertyManager(QtEnumPropertyManager *manager) override;
  QWidget *createEditorForManager(QtEnumPropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtEnumPropertyManager *manager) override;

private:
  QtEnumEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtEnumEditorFactory)
  Q_DISABLE_COPY(QtEnumEditorFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotEnumNamesChanged(QtProperty *, const QStringList &))
  Q_PRIVATE_SLOT(d_func(), void slotEnumIconsChanged(QtProperty *, const QMap<int, QIcon> &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(int))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtCursorEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtCursorEditorFactory : public QtAbstractEditorFactory<QtCursorPropertyManager> {
  Q_OBJECT
public:
  QtCursorEditorFactory(QObject *parent = nullptr);
  ~QtCursorEditorFactory() override;

protected:
  void connectPropertyManager(QtCursorPropertyManager *manager) override;
  QWidget *createEditorForManager(QtCursorPropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtCursorPropertyManager *manager) override;

private:
  QtCursorEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtCursorEditorFactory)
  Q_DISABLE_COPY(QtCursorEditorFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QCursor &))
  Q_PRIVATE_SLOT(d_func(), void slotEnumChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtColorEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtColorEditorFactory : public QtAbstractEditorFactory<QtColorPropertyManager> {
  Q_OBJECT
public:
  QtColorEditorFactory(QObject *parent = nullptr);
  ~QtColorEditorFactory() override;

protected:
  void connectPropertyManager(QtColorPropertyManager *manager) override;
  QWidget *createEditorForManager(QtColorPropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtColorPropertyManager *manager) override;

private:
  QtColorEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtColorEditorFactory)
  Q_DISABLE_COPY(QtColorEditorFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QColor &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QColor &))
};

class QtFontEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtFontEditorFactory : public QtAbstractEditorFactory<QtFontPropertyManager> {
  Q_OBJECT
public:
  QtFontEditorFactory(QObject *parent = nullptr);
  ~QtFontEditorFactory() override;

protected:
  void connectPropertyManager(QtFontPropertyManager *manager) override;
  QWidget *createEditorForManager(QtFontPropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtFontPropertyManager *manager) override;

private:
  QtFontEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtFontEditorFactory)
  Q_DISABLE_COPY(QtFontEditorFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, const QFont &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QFont &))
};

// Private classes //

// ---------- EditorFactoryPrivate :
// Base class for editor factory private classes. Manages mapping of properties
// to editors and vice versa.

template <class Editor> class EditorFactoryPrivate {
public:
  using EditorList = QList<Editor *>;
  using PropertyToEditorListMap = QMap<QtProperty *, EditorList>;
  using EditorToPropertyMap = QMap<Editor *, QtProperty *>;

  Editor *createEditor(QtProperty *property, QWidget *parent);
  void initializeEditor(QtProperty *property, Editor *e);
  void slotEditorDestroyed(QObject *object);

  PropertyToEditorListMap m_createdEditors;
  EditorToPropertyMap m_editorToProperty;
};

template <class Editor> Editor *EditorFactoryPrivate<Editor>::createEditor(QtProperty *property, QWidget *parent) {
  auto *editor = new Editor(parent);
  initializeEditor(property, editor);
  return editor;
}

template <class Editor> void EditorFactoryPrivate<Editor>::initializeEditor(QtProperty *property, Editor *editor) {
  typename PropertyToEditorListMap::iterator it = m_createdEditors.find(property);
  if (it == m_createdEditors.end())
    it = m_createdEditors.insert(property, EditorList());
  it.value().append(editor);
  m_editorToProperty.insert(editor, property);
}

template <class Editor> void EditorFactoryPrivate<Editor>::slotEditorDestroyed(QObject *object) {
  const typename EditorToPropertyMap::iterator ecend = m_editorToProperty.end();
  for (typename EditorToPropertyMap::iterator itEditor = m_editorToProperty.begin(); itEditor != ecend; ++itEditor) {
    if (itEditor.key() == object) {
      Editor *editor = itEditor.key();
      QtProperty *property = itEditor.value();
      const typename PropertyToEditorListMap::iterator pit = m_createdEditors.find(property);
      if (pit != m_createdEditors.end()) {
        pit.value().removeAll(editor);
        if (pit.value().empty())
          m_createdEditors.erase(pit);
      }
      m_editorToProperty.erase(itEditor);
      return;
    }
  }
}

class QtCharEdit : public QWidget {
  Q_OBJECT
public:
  QtCharEdit(QWidget *parent = nullptr);

  QChar value() const;
  bool eventFilter(QObject *o, QEvent *e) override;
public Q_SLOTS:
  void setValue(const QChar &value);
Q_SIGNALS:
  void valueChanged(const QChar &value);

protected:
  void focusInEvent(QFocusEvent *e) override;
  void focusOutEvent(QFocusEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;
  void keyReleaseEvent(QKeyEvent *e) override;
  bool event(QEvent *e) override;
private slots:
  void slotClearChar();

private:
  void handleKeyEvent(QKeyEvent *e);

  QChar m_value;
  QLineEdit *m_lineEdit;
};

class QtColorEditWidget : public QWidget {
  Q_OBJECT

public:
  QtColorEditWidget(QWidget *parent);

  bool eventFilter(QObject *obj, QEvent *ev) override;

public Q_SLOTS:
  void setValue(const QColor &value);

private Q_SLOTS:
  void buttonClicked();

Q_SIGNALS:
  void valueChanged(const QColor &value);

private:
  QColor m_color;
  QLabel *m_pixmapLabel;
  QLabel *m_label;
  QToolButton *m_button;
};

class QtCharEditorFactoryPrivate : public EditorFactoryPrivate<QtCharEdit> {
  QtCharEditorFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtCharEditorFactory)
public:
  void slotPropertyChanged(QtProperty *property, const QChar &value);
  void slotSetValue(const QChar &value);
};

class QtCheckBoxFactoryPrivate : public EditorFactoryPrivate<QtBoolEdit> {
  QtCheckBoxFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtCheckBoxFactory)
public:
  void slotPropertyChanged(QtProperty *property, bool value);
  void slotSetValue(bool value);
};

class QtColorEditorFactoryPrivate : public EditorFactoryPrivate<QtColorEditWidget> {
  QtColorEditorFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtColorEditorFactory)
public:
  void slotPropertyChanged(QtProperty *property, const QColor &value);
  void slotSetValue(const QColor &value);
};

class QtCursorEditorFactoryPrivate {
  QtCursorEditorFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtCursorEditorFactory)
public:
  QtCursorEditorFactoryPrivate();

  void slotPropertyChanged(QtProperty *const property, const QCursor &cursor);
  void slotEnumChanged(QtProperty *property, int value);
  void slotEditorDestroyed(QObject *object);

  QtEnumEditorFactory *m_enumEditorFactory;
  QtEnumPropertyManager *m_enumPropertyManager;

  QMap<QtProperty *, QtProperty *> m_propertyToEnum;
  QMap<QtProperty *, QtProperty *> m_enumToProperty;
  QMap<QtProperty *, QList<QWidget *>> m_enumToEditors;
  QMap<QWidget *, QtProperty *> m_editorToEnum;
  bool m_updatingEnum;
};

class QtDateEditFactoryPrivate : public EditorFactoryPrivate<QDateEdit> {
  QtDateEditFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtDateEditFactory)
public:
  void slotPropertyChanged(QtProperty *property, const QDate &value);
  void slotRangeChanged(QtProperty *property, const QDate &min, const QDate &max);
  void slotSetValue(const QDate &value);
};

class QtDateTimeEditFactoryPrivate : public EditorFactoryPrivate<QDateTimeEdit> {
  QtDateTimeEditFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtDateTimeEditFactory)
public:
  void slotPropertyChanged(QtProperty *property, const QDateTime &value);
  void slotSetValue(const QDateTime &value);
};

class QtDoubleSpinBoxFactoryPrivate : public EditorFactoryPrivate<QDoubleSpinBox> {
  QtDoubleSpinBoxFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtDoubleSpinBoxFactory)
public:
  void slotPropertyChanged(QtProperty *property, double value);
  void slotRangeChanged(QtProperty *property, double min, double max);
  void slotSingleStepChanged(QtProperty *property, double step);
  void slotDecimalsChanged(QtProperty *property, int prec);
  void slotSetValue(double value);
};

class QtEnumEditorFactoryPrivate : public EditorFactoryPrivate<QComboBox> {
  QtEnumEditorFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtEnumEditorFactory)
public:
  void slotPropertyChanged(QtProperty *property, int value);
  void slotEnumNamesChanged(QtProperty *property, const QStringList & /*enumNames*/);
  void slotEnumIconsChanged(QtProperty *property, const QMap<int, QIcon> & /*enumIcons*/);
  void slotSetValue(int value);
};

class QtFontEditWidget : public QWidget {
  Q_OBJECT

public:
  QtFontEditWidget(QWidget *parent);

  bool eventFilter(QObject *obj, QEvent *ev) override;

public Q_SLOTS:
  void setValue(const QFont &value);

private Q_SLOTS:
  void buttonClicked();

Q_SIGNALS:
  void valueChanged(const QFont &value);

private:
  QFont m_font;
  QLabel *m_pixmapLabel;
  QLabel *m_label;
  QToolButton *m_button;
};

class QtFontEditorFactoryPrivate : public EditorFactoryPrivate<QtFontEditWidget> {
  QtFontEditorFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtFontEditorFactory)
public:
  void slotPropertyChanged(QtProperty *property, const QFont &value);
  void slotSetValue(const QFont &value);
};

class QtKeySequenceEditorFactoryPrivate : public EditorFactoryPrivate<QtKeySequenceEdit> {
  QtKeySequenceEditorFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtKeySequenceEditorFactory)
public:
  void slotPropertyChanged(QtProperty *property, const QKeySequence &value);
  void slotSetValue(const QKeySequence &value);
};

class QtLineEditFactoryPrivate : public EditorFactoryPrivate<QLineEdit> {
  QtLineEditFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtLineEditFactory)
public:
  void slotPropertyChanged(QtProperty *property, const QString &value);
  void slotRegExpChanged(QtProperty *property, const QRegExp &regExp);
  void slotSetValue(const QString &value);
};

class QtScrollBarFactoryPrivate : public EditorFactoryPrivate<QScrollBar> {
  QtScrollBarFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtScrollBarFactory)
public:
  void slotPropertyChanged(QtProperty *property, int value);
  void slotRangeChanged(QtProperty *property, int min, int max);
  void slotSingleStepChanged(QtProperty *property, int step);
  void slotSetValue(int value);
};

class QtSliderFactoryPrivate : public EditorFactoryPrivate<QSlider> {
  QtSliderFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtSliderFactory)
public:
  void slotPropertyChanged(QtProperty *property, int value);
  void slotRangeChanged(QtProperty *property, int min, int max);
  void slotSingleStepChanged(QtProperty *property, int step);
  void slotSetValue(int value);
};

template <class SpinBox> class QtSpinBoxFactoryPrivateBase : public EditorFactoryPrivate<SpinBox> {
public:
  // This q_ptr is public due to the base class QtSpinBoxFactoryBase needing
  // access to it.
  QtSpinBoxFactoryBase<SpinBox> *q_ptr;
  using EditorFactoryPrivate<SpinBox>::m_editorToProperty;
  void slotPropertyChanged(QtProperty *property, int value);
  void slotRangeChanged(QtProperty *property, int min, int max);
  void slotSingleStepChanged(QtProperty *property, int step);
  void slotSetValue(int value);
};

class QtSpinBoxFactoryPrivate : public QtSpinBoxFactoryPrivateBase<QSpinBox> {
  Q_DECLARE_PUBLIC(QtSpinBoxFactory)
};

class QtSpinBoxFactoryNoTimerPrivate : public QtSpinBoxFactoryPrivateBase<QSpinBoxNoTimer> {
  Q_DECLARE_PUBLIC(QtSpinBoxFactoryNoTimer)
};

// ------------ QtSpinBoxFactory
template <class SpinBox>
void QtSpinBoxFactoryPrivateBase<SpinBox>::slotPropertyChanged(QtProperty *property, int value) {
  if (!this->m_createdEditors.contains(property))
    return;
  QListIterator<SpinBox *> itEditor(this->m_createdEditors[property]);
  while (itEditor.hasNext()) {
    auto *editor = itEditor.next();
    if (editor->value() != value) {
      editor->blockSignals(true);
      editor->setValue(value);
      editor->blockSignals(false);
    }
  }
}

template <class SpinBox>
void QtSpinBoxFactoryPrivateBase<SpinBox>::slotRangeChanged(QtProperty *property, int min, int max) {
  if (!this->m_createdEditors.contains(property))
    return;

  QtIntPropertyManager *manager = q_ptr->propertyManager(property);
  if (!manager)
    return;

  QListIterator<SpinBox *> itEditor(this->m_createdEditors[property]);
  while (itEditor.hasNext()) {
    auto *editor = itEditor.next();
    editor->blockSignals(true);
    editor->setRange(min, max);
    editor->setValue(manager->value(property));
    editor->blockSignals(false);
  }
}

template <class SpinBox>
void QtSpinBoxFactoryPrivateBase<SpinBox>::slotSingleStepChanged(QtProperty *property, int step) {
  if (!this->m_createdEditors.contains(property))
    return;
  QListIterator<SpinBox *> itEditor(this->m_createdEditors[property]);
  while (itEditor.hasNext()) {
    auto *editor = itEditor.next();
    editor->blockSignals(true);
    editor->setSingleStep(step);
    editor->blockSignals(false);
  }
}

template <class SpinBox> void QtSpinBoxFactoryPrivateBase<SpinBox>::slotSetValue(int value) {
  QObject *object = q_ptr->sender();
  typename QMap<SpinBox *, QtProperty *>::ConstIterator ecend = this->m_editorToProperty.constEnd();
  for (typename QMap<SpinBox *, QtProperty *>::ConstIterator itEditor = this->m_editorToProperty.constBegin();
       itEditor != ecend; ++itEditor) {
    if (itEditor.key() == object) {
      QtProperty *property = itEditor.value();
      QtIntPropertyManager *manager = q_ptr->propertyManager(property);
      if (!manager)
        return;
      manager->setValue(property, value);
      return;
    }
  }
}

class QtTimeEditFactoryPrivate : public EditorFactoryPrivate<QTimeEdit> {
  QtTimeEditFactory *q_ptr;
  Q_DECLARE_PUBLIC(QtTimeEditFactory)
public:
  void slotPropertyChanged(QtProperty *property, const QTime &value);
  void slotSetValue(const QTime &value);
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif
