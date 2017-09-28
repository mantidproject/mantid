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

#ifndef QTPROPERTYMANAGER_H
#define QTPROPERTYMANAGER_H

#include "qtpropertybrowser.h"

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QDate;
class QTime;
class QDateTime;
class QLocale;

class EXPORT_OPT_MANTIDQT_COMMON QtGroupPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtGroupPropertyManager(QObject *parent = 0);
  ~QtGroupPropertyManager() override;

protected:
  bool hasValue(const QtProperty *property) const override;

  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;
};

class QtIntPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtIntPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtIntPropertyManager(QObject *parent = 0);
  ~QtIntPropertyManager() override;

  int value(const QtProperty *property) const;
  int minimum(const QtProperty *property) const;
  int maximum(const QtProperty *property) const;
  int singleStep(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, int val);
  void setMinimum(QtProperty *property, int minVal);
  void setMaximum(QtProperty *property, int maxVal);
  void setRange(QtProperty *property, int minVal, int maxVal);
  void setSingleStep(QtProperty *property, int step);
Q_SIGNALS:
  void valueChanged(QtProperty *property, int val);
  void rangeChanged(QtProperty *property, int minVal, int maxVal);
  void singleStepChanged(QtProperty *property, int step);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtIntPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtIntPropertyManager)
  Q_DISABLE_COPY(QtIntPropertyManager)
};

class QtBoolPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtBoolPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtBoolPropertyManager(QObject *parent = 0);
  ~QtBoolPropertyManager() override;

  bool value(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, bool val);
Q_SIGNALS:
  void valueChanged(QtProperty *property, bool val);

protected:
  QString valueText(const QtProperty *property) const override;
  QIcon valueIcon(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtBoolPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtBoolPropertyManager)
  Q_DISABLE_COPY(QtBoolPropertyManager)
};

class QtDoublePropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtDoublePropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtDoublePropertyManager(QObject *parent = 0);
  ~QtDoublePropertyManager() override;

  double value(const QtProperty *property) const;
  double minimum(const QtProperty *property) const;
  double maximum(const QtProperty *property) const;
  double singleStep(const QtProperty *property) const;
  int decimals(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, double val);
  void setMinimum(QtProperty *property, double minVal);
  void setMaximum(QtProperty *property, double maxVal);
  void setRange(QtProperty *property, double minVal, double maxVal);
  void setSingleStep(QtProperty *property, double step);
  void setDecimals(QtProperty *property, int prec);
Q_SIGNALS:
  void valueChanged(QtProperty *property, double val);
  void rangeChanged(QtProperty *property, double minVal, double maxVal);
  void singleStepChanged(QtProperty *property, double step);
  void decimalsChanged(QtProperty *property, int prec);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtDoublePropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtDoublePropertyManager)
  Q_DISABLE_COPY(QtDoublePropertyManager)
};

class QtStringPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtStringPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtStringPropertyManager(QObject *parent = 0);
  ~QtStringPropertyManager() override;

  QString value(const QtProperty *property) const;
  QRegExp regExp(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QString &val);
  void setRegExp(QtProperty *property, const QRegExp &regExp);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QString &val);
  void regExpChanged(QtProperty *property, const QRegExp &regExp);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtStringPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtStringPropertyManager)
  Q_DISABLE_COPY(QtStringPropertyManager)
};

class QtDatePropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtDatePropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtDatePropertyManager(QObject *parent = 0);
  ~QtDatePropertyManager() override;

  QDate value(const QtProperty *property) const;
  QDate minimum(const QtProperty *property) const;
  QDate maximum(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QDate &val);
  void setMinimum(QtProperty *property, const QDate &minVal);
  void setMaximum(QtProperty *property, const QDate &maxVal);
  void setRange(QtProperty *property, const QDate &minVal, const QDate &maxVal);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QDate &val);
  void rangeChanged(QtProperty *property, const QDate &minVal,
                    const QDate &maxVal);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtDatePropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtDatePropertyManager)
  Q_DISABLE_COPY(QtDatePropertyManager)
};

class QtTimePropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtTimePropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtTimePropertyManager(QObject *parent = 0);
  ~QtTimePropertyManager() override;

  QTime value(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QTime &val);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QTime &val);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtTimePropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtTimePropertyManager)
  Q_DISABLE_COPY(QtTimePropertyManager)
};

class QtDateTimePropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtDateTimePropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtDateTimePropertyManager(QObject *parent = 0);
  ~QtDateTimePropertyManager() override;

  QDateTime value(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QDateTime &val);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QDateTime &val);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtDateTimePropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtDateTimePropertyManager)
  Q_DISABLE_COPY(QtDateTimePropertyManager)
};

class QtKeySequencePropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtKeySequencePropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtKeySequencePropertyManager(QObject *parent = 0);
  ~QtKeySequencePropertyManager() override;

  QKeySequence value(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QKeySequence &val);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QKeySequence &val);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtKeySequencePropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtKeySequencePropertyManager)
  Q_DISABLE_COPY(QtKeySequencePropertyManager)
};

class QtCharPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtCharPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtCharPropertyManager(QObject *parent = 0);
  ~QtCharPropertyManager() override;

  QChar value(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QChar &val);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QChar &val);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtCharPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtCharPropertyManager)
  Q_DISABLE_COPY(QtCharPropertyManager)
};

class QtEnumPropertyManager;
class QtLocalePropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtLocalePropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtLocalePropertyManager(QObject *parent = 0);
  ~QtLocalePropertyManager() override;

  QtEnumPropertyManager *subEnumPropertyManager() const;

  QLocale value(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QLocale &val);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QLocale &val);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtLocalePropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtLocalePropertyManager)
  Q_DISABLE_COPY(QtLocalePropertyManager)
  Q_PRIVATE_SLOT(d_func(), void slotEnumChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
};

class QtPointPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtPointPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtPointPropertyManager(QObject *parent = 0);
  ~QtPointPropertyManager() override;

  QtIntPropertyManager *subIntPropertyManager() const;

  QPoint value(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QPoint &val);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QPoint &val);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtPointPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtPointPropertyManager)
  Q_DISABLE_COPY(QtPointPropertyManager)
  Q_PRIVATE_SLOT(d_func(), void slotIntChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
};

class QtPointFPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtPointFPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtPointFPropertyManager(QObject *parent = 0);
  ~QtPointFPropertyManager() override;

  QtDoublePropertyManager *subDoublePropertyManager() const;

  QPointF value(const QtProperty *property) const;
  int decimals(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QPointF &val);
  void setDecimals(QtProperty *property, int prec);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QPointF &val);
  void decimalsChanged(QtProperty *property, int prec);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtPointFPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtPointFPropertyManager)
  Q_DISABLE_COPY(QtPointFPropertyManager)
  Q_PRIVATE_SLOT(d_func(), void slotDoubleChanged(QtProperty *, double))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
};

class QtSizePropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtSizePropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtSizePropertyManager(QObject *parent = 0);
  ~QtSizePropertyManager() override;

  QtIntPropertyManager *subIntPropertyManager() const;

  QSize value(const QtProperty *property) const;
  QSize minimum(const QtProperty *property) const;
  QSize maximum(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QSize &val);
  void setMinimum(QtProperty *property, const QSize &minVal);
  void setMaximum(QtProperty *property, const QSize &maxVal);
  void setRange(QtProperty *property, const QSize &minVal, const QSize &maxVal);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QSize &val);
  void rangeChanged(QtProperty *property, const QSize &minVal,
                    const QSize &maxVal);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtSizePropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtSizePropertyManager)
  Q_DISABLE_COPY(QtSizePropertyManager)
  Q_PRIVATE_SLOT(d_func(), void slotIntChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
};

class QtSizeFPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtSizeFPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtSizeFPropertyManager(QObject *parent = 0);
  ~QtSizeFPropertyManager() override;

  QtDoublePropertyManager *subDoublePropertyManager() const;

  QSizeF value(const QtProperty *property) const;
  QSizeF minimum(const QtProperty *property) const;
  QSizeF maximum(const QtProperty *property) const;
  int decimals(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QSizeF &val);
  void setMinimum(QtProperty *property, const QSizeF &minVal);
  void setMaximum(QtProperty *property, const QSizeF &maxVal);
  void setRange(QtProperty *property, const QSizeF &minVal,
                const QSizeF &maxVal);
  void setDecimals(QtProperty *property, int prec);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QSizeF &val);
  void rangeChanged(QtProperty *property, const QSizeF &minVal,
                    const QSizeF &maxVal);
  void decimalsChanged(QtProperty *property, int prec);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtSizeFPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtSizeFPropertyManager)
  Q_DISABLE_COPY(QtSizeFPropertyManager)
  Q_PRIVATE_SLOT(d_func(), void slotDoubleChanged(QtProperty *, double))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
};

class QtRectPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtRectPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtRectPropertyManager(QObject *parent = 0);
  ~QtRectPropertyManager() override;

  QtIntPropertyManager *subIntPropertyManager() const;

  QRect value(const QtProperty *property) const;
  QRect constraint(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QRect &val);
  void setConstraint(QtProperty *property, const QRect &constraint);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QRect &val);
  void constraintChanged(QtProperty *property, const QRect &constraint);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtRectPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtRectPropertyManager)
  Q_DISABLE_COPY(QtRectPropertyManager)
  Q_PRIVATE_SLOT(d_func(), void slotIntChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
};

class QtRectFPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtRectFPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtRectFPropertyManager(QObject *parent = 0);
  ~QtRectFPropertyManager() override;

  QtDoublePropertyManager *subDoublePropertyManager() const;

  QRectF value(const QtProperty *property) const;
  QRectF constraint(const QtProperty *property) const;
  int decimals(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QRectF &val);
  void setConstraint(QtProperty *property, const QRectF &constraint);
  void setDecimals(QtProperty *property, int prec);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QRectF &val);
  void constraintChanged(QtProperty *property, const QRectF &constraint);
  void decimalsChanged(QtProperty *property, int prec);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtRectFPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtRectFPropertyManager)
  Q_DISABLE_COPY(QtRectFPropertyManager)
  Q_PRIVATE_SLOT(d_func(), void slotDoubleChanged(QtProperty *, double))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
};

class QtEnumPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtEnumPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtEnumPropertyManager(QObject *parent = 0);
  ~QtEnumPropertyManager() override;

  int value(const QtProperty *property) const;
  QStringList enumNames(const QtProperty *property) const;
  QMap<int, QIcon> enumIcons(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, int val);
  void setEnumNames(QtProperty *property, const QStringList &names);
  void setEnumIcons(QtProperty *property, const QMap<int, QIcon> &icons);
Q_SIGNALS:
  void valueChanged(QtProperty *property, int val);
  void enumNamesChanged(QtProperty *property, const QStringList &names);
  void enumIconsChanged(QtProperty *property, const QMap<int, QIcon> &icons);

protected:
  QString valueText(const QtProperty *property) const override;
  QIcon valueIcon(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtEnumPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtEnumPropertyManager)
  Q_DISABLE_COPY(QtEnumPropertyManager)
};

class QtFlagPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtFlagPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtFlagPropertyManager(QObject *parent = 0);
  ~QtFlagPropertyManager() override;

  QtBoolPropertyManager *subBoolPropertyManager() const;

  int value(const QtProperty *property) const;
  QStringList flagNames(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, int val);
  void setFlagNames(QtProperty *property, const QStringList &names);
Q_SIGNALS:
  void valueChanged(QtProperty *property, int val);
  void flagNamesChanged(QtProperty *property, const QStringList &names);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtFlagPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtFlagPropertyManager)
  Q_DISABLE_COPY(QtFlagPropertyManager)
  Q_PRIVATE_SLOT(d_func(), void slotBoolChanged(QtProperty *, bool))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
};

class QtSizePolicyPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtSizePolicyPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtSizePolicyPropertyManager(QObject *parent = 0);
  ~QtSizePolicyPropertyManager() override;

  QtIntPropertyManager *subIntPropertyManager() const;
  QtEnumPropertyManager *subEnumPropertyManager() const;

  QSizePolicy value(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QSizePolicy &val);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QSizePolicy &val);

protected:
  QString valueText(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtSizePolicyPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtSizePolicyPropertyManager)
  Q_DISABLE_COPY(QtSizePolicyPropertyManager)
  Q_PRIVATE_SLOT(d_func(), void slotIntChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotEnumChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
};

class QtFontPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtFontPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtFontPropertyManager(QObject *parent = 0);
  ~QtFontPropertyManager() override;

  QtIntPropertyManager *subIntPropertyManager() const;
  QtEnumPropertyManager *subEnumPropertyManager() const;
  QtBoolPropertyManager *subBoolPropertyManager() const;

  QFont value(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QFont &val);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QFont &val);

protected:
  QString valueText(const QtProperty *property) const override;
  QIcon valueIcon(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtFontPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtFontPropertyManager)
  Q_DISABLE_COPY(QtFontPropertyManager)
  Q_PRIVATE_SLOT(d_func(), void slotIntChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotEnumChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotBoolChanged(QtProperty *, bool))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
  Q_PRIVATE_SLOT(d_func(), void slotFontDatabaseChanged())
  Q_PRIVATE_SLOT(d_func(), void slotFontDatabaseDelayedChange())
};

class QtColorPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtColorPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtColorPropertyManager(QObject *parent = 0);
  ~QtColorPropertyManager() override;

  QtIntPropertyManager *subIntPropertyManager() const;

  QColor value(const QtProperty *property) const;

public Q_SLOTS:
  void setValue(QtProperty *property, const QColor &val);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QColor &val);

protected:
  QString valueText(const QtProperty *property) const override;
  QIcon valueIcon(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtColorPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtColorPropertyManager)
  Q_DISABLE_COPY(QtColorPropertyManager)
  Q_PRIVATE_SLOT(d_func(), void slotIntChanged(QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
};

class QtCursorPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtCursorPropertyManager
    : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtCursorPropertyManager(QObject *parent = 0);
  ~QtCursorPropertyManager() override;

#ifndef QT_NO_CURSOR
  QCursor value(const QtProperty *property) const;
#endif

public Q_SLOTS:
  void setValue(QtProperty *property, const QCursor &val);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QCursor &val);

protected:
  QString valueText(const QtProperty *property) const override;
  QIcon valueIcon(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;

private:
  QtCursorPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtCursorPropertyManager)
  Q_DISABLE_COPY(QtCursorPropertyManager)
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

#endif
