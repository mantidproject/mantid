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

#ifndef QTEDITORFACTORY_H
#define QTEDITORFACTORY_H

#include "qtpropertymanager.h"

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QtSpinBoxFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtSpinBoxFactory
    : public QtAbstractEditorFactory<QtIntPropertyManager> {
  Q_OBJECT
public:
  QtSpinBoxFactory(QObject *parent = 0);
  ~QtSpinBoxFactory() override;

protected:
  void connectPropertyManager(QtIntPropertyManager *manager) override;
  QWidget *createEditorForManager(QtIntPropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void disconnectPropertyManager(QtIntPropertyManager *manager) override;

private:
  QtSpinBoxFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtSpinBoxFactory)
  Q_DISABLE_COPY(QtSpinBoxFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(QtProperty *, int, int))
  Q_PRIVATE_SLOT(d_func(), void slotSingleStepChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(int))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtSliderFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtSliderFactory
    : public QtAbstractEditorFactory<QtIntPropertyManager> {
  Q_OBJECT
public:
  QtSliderFactory(QObject *parent = 0);
  ~QtSliderFactory() override;

protected:
  void connectPropertyManager(QtIntPropertyManager *manager) override;
  QWidget *createEditorForManager(QtIntPropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
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

class EXPORT_OPT_MANTIDQT_COMMON QtScrollBarFactory
    : public QtAbstractEditorFactory<QtIntPropertyManager> {
  Q_OBJECT
public:
  QtScrollBarFactory(QObject *parent = 0);
  ~QtScrollBarFactory() override;

protected:
  void connectPropertyManager(QtIntPropertyManager *manager) override;
  QWidget *createEditorForManager(QtIntPropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
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

class EXPORT_OPT_MANTIDQT_COMMON QtCheckBoxFactory
    : public QtAbstractEditorFactory<QtBoolPropertyManager> {
  Q_OBJECT
public:
  QtCheckBoxFactory(QObject *parent = 0);
  ~QtCheckBoxFactory() override;

protected:
  void connectPropertyManager(QtBoolPropertyManager *manager) override;
  QWidget *createEditorForManager(QtBoolPropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
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

class EXPORT_OPT_MANTIDQT_COMMON QtDoubleSpinBoxFactory
    : public QtAbstractEditorFactory<QtDoublePropertyManager> {
  Q_OBJECT
public:
  QtDoubleSpinBoxFactory(QObject *parent = 0);
  ~QtDoubleSpinBoxFactory() override;

protected:
  void connectPropertyManager(QtDoublePropertyManager *manager) override;
  QWidget *createEditorForManager(QtDoublePropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
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

class EXPORT_OPT_MANTIDQT_COMMON QtLineEditFactory
    : public QtAbstractEditorFactory<QtStringPropertyManager> {
  Q_OBJECT
public:
  QtLineEditFactory(QObject *parent = 0);
  ~QtLineEditFactory() override;

protected:
  void connectPropertyManager(QtStringPropertyManager *manager) override;
  QWidget *createEditorForManager(QtStringPropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void disconnectPropertyManager(QtStringPropertyManager *manager) override;

private:
  QtLineEditFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtLineEditFactory)
  Q_DISABLE_COPY(QtLineEditFactory)
  Q_PRIVATE_SLOT(d_func(),
                 void slotPropertyChanged(QtProperty *, const QString &))
  Q_PRIVATE_SLOT(d_func(),
                 void slotRegExpChanged(QtProperty *, const QRegExp &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QString &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtDateEditFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtDateEditFactory
    : public QtAbstractEditorFactory<QtDatePropertyManager> {
  Q_OBJECT
public:
  QtDateEditFactory(QObject *parent = 0);
  ~QtDateEditFactory() override;

protected:
  void connectPropertyManager(QtDatePropertyManager *manager) override;
  QWidget *createEditorForManager(QtDatePropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void disconnectPropertyManager(QtDatePropertyManager *manager) override;

private:
  QtDateEditFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtDateEditFactory)
  Q_DISABLE_COPY(QtDateEditFactory)
  Q_PRIVATE_SLOT(d_func(),
                 void slotPropertyChanged(QtProperty *, const QDate &))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(QtProperty *, const QDate &,
                                                 const QDate &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QDate &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtTimeEditFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtTimeEditFactory
    : public QtAbstractEditorFactory<QtTimePropertyManager> {
  Q_OBJECT
public:
  QtTimeEditFactory(QObject *parent = 0);
  ~QtTimeEditFactory() override;

protected:
  void connectPropertyManager(QtTimePropertyManager *manager) override;
  QWidget *createEditorForManager(QtTimePropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void disconnectPropertyManager(QtTimePropertyManager *manager) override;

private:
  QtTimeEditFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtTimeEditFactory)
  Q_DISABLE_COPY(QtTimeEditFactory)
  Q_PRIVATE_SLOT(d_func(),
                 void slotPropertyChanged(QtProperty *, const QTime &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QTime &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtDateTimeEditFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtDateTimeEditFactory
    : public QtAbstractEditorFactory<QtDateTimePropertyManager> {
  Q_OBJECT
public:
  QtDateTimeEditFactory(QObject *parent = 0);
  ~QtDateTimeEditFactory() override;

protected:
  void connectPropertyManager(QtDateTimePropertyManager *manager) override;
  QWidget *createEditorForManager(QtDateTimePropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void disconnectPropertyManager(QtDateTimePropertyManager *manager) override;

private:
  QtDateTimeEditFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtDateTimeEditFactory)
  Q_DISABLE_COPY(QtDateTimeEditFactory)
  Q_PRIVATE_SLOT(d_func(),
                 void slotPropertyChanged(QtProperty *, const QDateTime &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QDateTime &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtKeySequenceEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtKeySequenceEditorFactory
    : public QtAbstractEditorFactory<QtKeySequencePropertyManager> {
  Q_OBJECT
public:
  QtKeySequenceEditorFactory(QObject *parent = 0);
  ~QtKeySequenceEditorFactory() override;

protected:
  void connectPropertyManager(QtKeySequencePropertyManager *manager) override;
  QWidget *createEditorForManager(QtKeySequencePropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void
  disconnectPropertyManager(QtKeySequencePropertyManager *manager) override;

private:
  QtKeySequenceEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtKeySequenceEditorFactory)
  Q_DISABLE_COPY(QtKeySequenceEditorFactory)
  Q_PRIVATE_SLOT(d_func(),
                 void slotPropertyChanged(QtProperty *, const QKeySequence &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QKeySequence &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtCharEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtCharEditorFactory
    : public QtAbstractEditorFactory<QtCharPropertyManager> {
  Q_OBJECT
public:
  QtCharEditorFactory(QObject *parent = 0);
  ~QtCharEditorFactory() override;

protected:
  void connectPropertyManager(QtCharPropertyManager *manager) override;
  QWidget *createEditorForManager(QtCharPropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void disconnectPropertyManager(QtCharPropertyManager *manager) override;

private:
  QtCharEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtCharEditorFactory)
  Q_DISABLE_COPY(QtCharEditorFactory)
  Q_PRIVATE_SLOT(d_func(),
                 void slotPropertyChanged(QtProperty *, const QChar &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QChar &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtEnumEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtEnumEditorFactory
    : public QtAbstractEditorFactory<QtEnumPropertyManager> {
  Q_OBJECT
public:
  QtEnumEditorFactory(QObject *parent = 0);
  ~QtEnumEditorFactory() override;

protected:
  void connectPropertyManager(QtEnumPropertyManager *manager) override;
  QWidget *createEditorForManager(QtEnumPropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void disconnectPropertyManager(QtEnumPropertyManager *manager) override;

private:
  QtEnumEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtEnumEditorFactory)
  Q_DISABLE_COPY(QtEnumEditorFactory)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(),
                 void slotEnumNamesChanged(QtProperty *, const QStringList &))
  Q_PRIVATE_SLOT(d_func(), void slotEnumIconsChanged(QtProperty *,
                                                     const QMap<int, QIcon> &))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(int))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtCursorEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtCursorEditorFactory
    : public QtAbstractEditorFactory<QtCursorPropertyManager> {
  Q_OBJECT
public:
  QtCursorEditorFactory(QObject *parent = 0);
  ~QtCursorEditorFactory() override;

protected:
  void connectPropertyManager(QtCursorPropertyManager *manager) override;
  QWidget *createEditorForManager(QtCursorPropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void disconnectPropertyManager(QtCursorPropertyManager *manager) override;

private:
  QtCursorEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtCursorEditorFactory)
  Q_DISABLE_COPY(QtCursorEditorFactory)
  Q_PRIVATE_SLOT(d_func(),
                 void slotPropertyChanged(QtProperty *, const QCursor &))
  Q_PRIVATE_SLOT(d_func(), void slotEnumChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
};

class QtColorEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtColorEditorFactory
    : public QtAbstractEditorFactory<QtColorPropertyManager> {
  Q_OBJECT
public:
  QtColorEditorFactory(QObject *parent = 0);
  ~QtColorEditorFactory() override;

protected:
  void connectPropertyManager(QtColorPropertyManager *manager) override;
  QWidget *createEditorForManager(QtColorPropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void disconnectPropertyManager(QtColorPropertyManager *manager) override;

private:
  QtColorEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtColorEditorFactory)
  Q_DISABLE_COPY(QtColorEditorFactory)
  Q_PRIVATE_SLOT(d_func(),
                 void slotPropertyChanged(QtProperty *, const QColor &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QColor &))
};

class QtFontEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtFontEditorFactory
    : public QtAbstractEditorFactory<QtFontPropertyManager> {
  Q_OBJECT
public:
  QtFontEditorFactory(QObject *parent = 0);
  ~QtFontEditorFactory() override;

protected:
  void connectPropertyManager(QtFontPropertyManager *manager) override;
  QWidget *createEditorForManager(QtFontPropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void disconnectPropertyManager(QtFontPropertyManager *manager) override;

private:
  QtFontEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtFontEditorFactory)
  Q_DISABLE_COPY(QtFontEditorFactory)
  Q_PRIVATE_SLOT(d_func(),
                 void slotPropertyChanged(QtProperty *, const QFont &))
  Q_PRIVATE_SLOT(d_func(), void slotEditorDestroyed(QObject *))
  Q_PRIVATE_SLOT(d_func(), void slotSetValue(const QFont &))
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

#endif
