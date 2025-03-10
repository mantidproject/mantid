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

#include "qtpropertybrowser.h"

#include <utility>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QDate;
class QTime;
class QDateTime;
class QLocale;

class EXPORT_OPT_MANTIDQT_COMMON QtGroupPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtGroupPropertyManager(QObject *parent = nullptr);
  ~QtGroupPropertyManager() override;

protected:
  bool hasValue(const QtProperty *property) const override;

  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;
};

class QtIntPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtIntPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtIntPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtBoolPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtBoolPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtDoublePropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtDoublePropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtStringPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtStringPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtDatePropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtDatePropertyManager(QObject *parent = nullptr);
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
  void rangeChanged(QtProperty *property, const QDate &minVal, const QDate &maxVal);

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

class EXPORT_OPT_MANTIDQT_COMMON QtTimePropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtTimePropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtDateTimePropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtDateTimePropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtKeySequencePropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtKeySequencePropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtCharPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtCharPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtLocalePropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtLocalePropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtPointPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtPointPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtPointFPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtPointFPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtSizePropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtSizePropertyManager(QObject *parent = nullptr);
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
  void rangeChanged(QtProperty *property, const QSize &minVal, const QSize &maxVal);

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

class EXPORT_OPT_MANTIDQT_COMMON QtSizeFPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtSizeFPropertyManager(QObject *parent = nullptr);
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
  void setRange(QtProperty *property, const QSizeF &minVal, const QSizeF &maxVal);
  void setDecimals(QtProperty *property, int prec);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QSizeF &val);
  void rangeChanged(QtProperty *property, const QSizeF &minVal, const QSizeF &maxVal);
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

class EXPORT_OPT_MANTIDQT_COMMON QtRectPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtRectPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtRectFPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtRectFPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtEnumPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtEnumPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtFlagPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtFlagPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtSizePolicyPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtSizePolicyPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtFontPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtFontPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtColorPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtColorPropertyManager(QObject *parent = nullptr);
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

class EXPORT_OPT_MANTIDQT_COMMON QtCursorPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtCursorPropertyManager(QObject *parent = nullptr);
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

// Private classes //

template <class PrivateData, class Value> static void setSimpleMinimumData(PrivateData *data, const Value &minVal) {
  data->minVal = minVal;
  if (data->maxVal < data->minVal)
    data->maxVal = data->minVal;

  if (data->val < data->minVal)
    data->val = data->minVal;
}

template <class PrivateData, class Value> static void setSimpleMaximumData(PrivateData *data, const Value &maxVal) {
  data->maxVal = maxVal;
  if (data->minVal > data->maxVal)
    data->minVal = data->maxVal;

  if (data->val > data->maxVal)
    data->val = data->maxVal;
}

template <class PrivateData, class Value> static void setSizeMinimumData(PrivateData *data, const Value &newMinVal) {
  data->minVal = newMinVal;
  if (data->maxVal.width() < data->minVal.width())
    data->maxVal.setWidth(data->minVal.width());
  if (data->maxVal.height() < data->minVal.height())
    data->maxVal.setHeight(data->minVal.height());

  if (data->val.width() < data->minVal.width())
    data->val.setWidth(data->minVal.width());
  if (data->val.height() < data->minVal.height())
    data->val.setHeight(data->minVal.height());
}

template <class PrivateData, class Value> static void setSizeMaximumData(PrivateData *data, const Value &newMaxVal) {
  data->maxVal = newMaxVal;
  if (data->minVal.width() > data->maxVal.width())
    data->minVal.setWidth(data->maxVal.width());
  if (data->minVal.height() > data->maxVal.height())
    data->minVal.setHeight(data->maxVal.height());

  if (data->val.width() > data->maxVal.width())
    data->val.setWidth(data->maxVal.width());
  if (data->val.height() > data->maxVal.height())
    data->val.setHeight(data->maxVal.height());
}

template <class SizeValue>
static SizeValue qBoundSize(const SizeValue &minVal, const SizeValue &val, const SizeValue &maxVal) {
  SizeValue croppedVal = val;
  if (minVal.width() > val.width())
    croppedVal.setWidth(minVal.width());
  else if (maxVal.width() < val.width())
    croppedVal.setWidth(maxVal.width());

  if (minVal.height() > val.height())
    croppedVal.setHeight(minVal.height());
  else if (maxVal.height() < val.height())
    croppedVal.setHeight(maxVal.height());

  return croppedVal;
}

// Match the exact signature of qBound for VS 6.
QSize qBound(QSize minVal, QSize val, QSize maxVal);
QSizeF qBound(QSizeF minVal, QSizeF val, QSizeF maxVal);

template <class Value> static void orderBorders(Value &minVal, Value &maxVal) {
  if (minVal > maxVal) {
    using std::swap;
    swap(minVal, maxVal);
  }
}

template <class Value> void orderSizeBorders(Value &minVal, Value &maxVal) {
  Value fromSize = minVal;
  Value toSize = maxVal;
  if (fromSize.width() > toSize.width()) {
    fromSize.setWidth(maxVal.width());
    toSize.setWidth(minVal.width());
  }
  if (fromSize.height() > toSize.height()) {
    fromSize.setHeight(maxVal.height());
    toSize.setHeight(minVal.height());
  }
  minVal = fromSize;
  maxVal = toSize;
}

inline static void orderBorders(QSize &minVal, QSize &maxVal) { orderSizeBorders(minVal, maxVal); }

inline static void orderBorders(QSizeF &minVal, QSizeF &maxVal) { orderSizeBorders(minVal, maxVal); }

////////

template <class Value, class PrivateData>
static Value getData(const QMap<const QtProperty *, PrivateData> &propertyMap, Value PrivateData::*data,
                     const QtProperty *property, const Value &defaultValue = Value()) {
  using PropertyToData = QMap<const QtProperty *, PrivateData>;
  using PropertyToDataConstIterator = typename PropertyToData::const_iterator;
  const PropertyToDataConstIterator it = propertyMap.constFind(property);
  if (it == propertyMap.constEnd())
    return defaultValue;
  return it.value().*data;
}

template <class Value, class PrivateData>
static Value getValue(const QMap<const QtProperty *, PrivateData> &propertyMap, const QtProperty *property,
                      const Value &defaultValue = Value()) {
  return getData<Value>(propertyMap, &PrivateData::val, property, defaultValue);
}

template <class Value, class PrivateData>
static Value getMinimum(const QMap<const QtProperty *, PrivateData> &propertyMap, const QtProperty *property,
                        const Value &defaultValue = Value()) {
  return getData<Value>(propertyMap, &PrivateData::minVal, property, defaultValue);
}

template <class Value, class PrivateData>
static Value getMaximum(const QMap<const QtProperty *, PrivateData> &propertyMap, const QtProperty *property,
                        const Value &defaultValue = Value()) {
  return getData<Value>(propertyMap, &PrivateData::maxVal, property, defaultValue);
}

template <class ValueChangeParameter, class Value, class PropertyManager>
static void setSimpleValue(QMap<const QtProperty *, Value> &propertyMap, PropertyManager *manager,
                           void (PropertyManager::*propertyChangedSignal)(QtProperty *),
                           void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
                           QtProperty *property, const Value &val) {
  using PropertyToData = QMap<const QtProperty *, Value>;
  using PropertyToDataIterator = typename PropertyToData::iterator;
  const PropertyToDataIterator it = propertyMap.find(property);
  if (it == propertyMap.end())
    return;

  if (it.value() == val)
    return;

  it.value() = val;

  emit(manager->*propertyChangedSignal)(property);
  emit(manager->*valueChangedSignal)(property, val);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value>
static void setValueInRange(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
                            void (PropertyManager::*propertyChangedSignal)(QtProperty *),
                            void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
                            QtProperty *property, const Value &val,
                            void (PropertyManagerPrivate::*setSubPropertyValue)(QtProperty *, ValueChangeParameter)) {
  using PrivateData = typename PropertyManagerPrivate::Data;
  using PropertyToData = QMap<const QtProperty *, PrivateData>;
  using PropertyToDataIterator = typename PropertyToData::iterator;
  const PropertyToDataIterator it = managerPrivate->m_values.find(property);
  if (it == managerPrivate->m_values.end())
    return;

  PrivateData &data = it.value();

  if (data.val == val)
    return;

  const Value oldVal = data.val;

  data.val = qBound(data.minVal, val, data.maxVal);

  if (data.val == oldVal)
    return;

  if (setSubPropertyValue)
    (managerPrivate->*setSubPropertyValue)(property, data.val);

  emit(manager->*propertyChangedSignal)(property);
  emit(manager->*valueChangedSignal)(property, data.val);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value>
static void
setBorderValues(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
                void (PropertyManager::*propertyChangedSignal)(QtProperty *),
                void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
                void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter, ValueChangeParameter),
                QtProperty *property, const Value &minVal, const Value &maxVal,
                void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *, ValueChangeParameter,
                                                                    ValueChangeParameter, ValueChangeParameter)) {
  using PrivateData = typename PropertyManagerPrivate::Data;
  using PropertyToData = QMap<const QtProperty *, PrivateData>;
  using PropertyToDataIterator = typename PropertyToData::iterator;
  const PropertyToDataIterator it = managerPrivate->m_values.find(property);
  if (it == managerPrivate->m_values.end())
    return;

  Value fromVal = minVal;
  Value toVal = maxVal;
  orderBorders(fromVal, toVal);

  PrivateData &data = it.value();

  if (data.minVal == fromVal && data.maxVal == toVal)
    return;

  const Value oldVal = data.val;

  data.setMinimumValue(fromVal);
  data.setMaximumValue(toVal);

  emit(manager->*rangeChangedSignal)(property, data.minVal, data.maxVal);

  if (setSubPropertyRange)
    (managerPrivate->*setSubPropertyRange)(property, data.minVal, data.maxVal, data.val);

  if (data.val == oldVal)
    return;

  emit(manager->*propertyChangedSignal)(property);
  emit(manager->*valueChangedSignal)(property, data.val);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value,
          class PrivateData>
static void
setBorderValue(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
               void (PropertyManager::*propertyChangedSignal)(QtProperty *),
               void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
               void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter, ValueChangeParameter),
               QtProperty *property, Value (PrivateData::*getRangeVal)() const,
               void (PrivateData::*setRangeVal)(ValueChangeParameter), const Value &borderVal,
               void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *, ValueChangeParameter,
                                                                   ValueChangeParameter, ValueChangeParameter)) {
  using PropertyToData = QMap<const QtProperty *, PrivateData>;
  using PropertyToDataIterator = typename PropertyToData::iterator;
  const PropertyToDataIterator it = managerPrivate->m_values.find(property);
  if (it == managerPrivate->m_values.end())
    return;

  PrivateData &data = it.value();

  if ((data.*getRangeVal)() == borderVal)
    return;

  const Value oldVal = data.val;

  (data.*setRangeVal)(borderVal);

  emit(manager->*rangeChangedSignal)(property, data.minVal, data.maxVal);

  if (setSubPropertyRange)
    (managerPrivate->*setSubPropertyRange)(property, data.minVal, data.maxVal, data.val);

  if (data.val == oldVal)
    return;

  emit(manager->*propertyChangedSignal)(property);
  emit(manager->*valueChangedSignal)(property, data.val);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value,
          class PrivateData>
static void setMinimumValue(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
                            void (PropertyManager::*propertyChangedSignal)(QtProperty *),
                            void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
                            void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter,
                                                                        ValueChangeParameter),
                            QtProperty *property, const Value &minVal) {
  void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *, ValueChangeParameter, ValueChangeParameter,
                                                      ValueChangeParameter) = nullptr;
  setBorderValue<ValueChangeParameter, PropertyManagerPrivate, PropertyManager, Value, PrivateData>(
      manager, managerPrivate, propertyChangedSignal, valueChangedSignal, rangeChangedSignal, property,
      &PropertyManagerPrivate::Data::minimumValue, &PropertyManagerPrivate::Data::setMinimumValue, minVal,
      setSubPropertyRange);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value,
          class PrivateData>
static void setMaximumValue(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
                            void (PropertyManager::*propertyChangedSignal)(QtProperty *),
                            void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
                            void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter,
                                                                        ValueChangeParameter),
                            QtProperty *property, const Value &maxVal) {
  void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *, ValueChangeParameter, ValueChangeParameter,
                                                      ValueChangeParameter) = nullptr;
  setBorderValue<ValueChangeParameter, PropertyManagerPrivate, PropertyManager, Value, PrivateData>(
      manager, managerPrivate, propertyChangedSignal, valueChangedSignal, rangeChangedSignal, property,
      &PropertyManagerPrivate::Data::maximumValue, &PropertyManagerPrivate::Data::setMaximumValue, maxVal,
      setSubPropertyRange);
}

class QtFontPropertyManagerPrivate {
  QtFontPropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtFontPropertyManager)
public:
  QtFontPropertyManagerPrivate();

  void slotIntChanged(const QtProperty *property, int value);
  void slotEnumChanged(const QtProperty *property, int value);
  void slotBoolChanged(const QtProperty *property, bool value);
  void slotPropertyDestroyed(const QtProperty *property);
  void slotFontDatabaseChanged();
  void slotFontDatabaseDelayedChange();

  QStringList m_familyNames;

  using PropertyValueMap = QMap<const QtProperty *, QFont>;
  PropertyValueMap m_values;

  QtIntPropertyManager *m_intPropertyManager;
  QtEnumPropertyManager *m_enumPropertyManager;
  QtBoolPropertyManager *m_boolPropertyManager;

  QMap<const QtProperty *, QtProperty *> m_propertyToFamily;
  QMap<const QtProperty *, QtProperty *> m_propertyToPointSize;
  QMap<const QtProperty *, QtProperty *> m_propertyToBold;
  QMap<const QtProperty *, QtProperty *> m_propertyToItalic;
  QMap<const QtProperty *, QtProperty *> m_propertyToUnderline;
  QMap<const QtProperty *, QtProperty *> m_propertyToStrikeOut;
  QMap<const QtProperty *, QtProperty *> m_propertyToKerning;

  QMap<const QtProperty *, QtProperty *> m_familyToProperty;
  QMap<const QtProperty *, QtProperty *> m_pointSizeToProperty;
  QMap<const QtProperty *, QtProperty *> m_boldToProperty;
  QMap<const QtProperty *, QtProperty *> m_italicToProperty;
  QMap<const QtProperty *, QtProperty *> m_underlineToProperty;
  QMap<const QtProperty *, QtProperty *> m_strikeOutToProperty;
  QMap<const QtProperty *, QtProperty *> m_kerningToProperty;

  bool m_settingValue;
  QTimer *m_fontDatabaseChangeTimer;
};

class QtFlagPropertyManagerPrivate {
  QtFlagPropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtFlagPropertyManager)
public:
  void slotBoolChanged(const QtProperty *property, bool value);
  void slotPropertyDestroyed(QtProperty *property);

  struct Data {
    Data() : val(-1) {}
    int val;
    QStringList flagNames;
  };

  using PropertyValueMap = QMap<const QtProperty *, Data>;
  PropertyValueMap m_values;

  QtBoolPropertyManager *m_boolPropertyManager;

  QMap<const QtProperty *, QList<QtProperty *>> m_propertyToFlags;

  QMap<const QtProperty *, QtProperty *> m_flagToProperty;
};

class QtColorPropertyManagerPrivate {
  QtColorPropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtColorPropertyManager)
public:
  void slotIntChanged(const QtProperty *property, int value);
  void slotPropertyDestroyed(const QtProperty *property);

  using PropertyValueMap = QMap<const QtProperty *, QColor>;
  PropertyValueMap m_values;

  QtIntPropertyManager *m_intPropertyManager;

  QMap<const QtProperty *, QtProperty *> m_propertyToR;
  QMap<const QtProperty *, QtProperty *> m_propertyToG;
  QMap<const QtProperty *, QtProperty *> m_propertyToB;
  QMap<const QtProperty *, QtProperty *> m_propertyToA;

  QMap<const QtProperty *, QtProperty *> m_rToProperty;
  QMap<const QtProperty *, QtProperty *> m_gToProperty;
  QMap<const QtProperty *, QtProperty *> m_bToProperty;
  QMap<const QtProperty *, QtProperty *> m_aToProperty;
};

class QtLocalePropertyManagerPrivate {
  QtLocalePropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtLocalePropertyManager)
public:
  QtLocalePropertyManagerPrivate();

  void slotEnumChanged(const QtProperty *property, int value);
  void slotPropertyDestroyed(const QtProperty *property);

  using PropertyValueMap = QMap<const QtProperty *, QLocale>;
  PropertyValueMap m_values;

  QtEnumPropertyManager *m_enumPropertyManager;

  QMap<const QtProperty *, QtProperty *> m_propertyToLanguage;
  QMap<const QtProperty *, QtProperty *> m_propertyToCountry;

  QMap<const QtProperty *, QtProperty *> m_languageToProperty;
  QMap<const QtProperty *, QtProperty *> m_countryToProperty;
};

class QtPointPropertyManagerPrivate {
  QtPointPropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtPointPropertyManager)
public:
  void slotIntChanged(const QtProperty *property, int value);
  void slotPropertyDestroyed(const QtProperty *property);

  using PropertyValueMap = QMap<const QtProperty *, QPoint>;
  PropertyValueMap m_values;

  QtIntPropertyManager *m_intPropertyManager;

  QMap<const QtProperty *, QtProperty *> m_propertyToX;
  QMap<const QtProperty *, QtProperty *> m_propertyToY;

  QMap<const QtProperty *, QtProperty *> m_xToProperty;
  QMap<const QtProperty *, QtProperty *> m_yToProperty;
};

class QtPointFPropertyManagerPrivate {
  QtPointFPropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtPointFPropertyManager)
public:
  struct Data {
    Data() : decimals(2) {}
    QPointF val;
    int decimals;
  };

  void slotDoubleChanged(const QtProperty *property, double value);
  void slotPropertyDestroyed(const QtProperty *property);

  using PropertyValueMap = QMap<const QtProperty *, Data>;
  PropertyValueMap m_values;

  QtDoublePropertyManager *m_doublePropertyManager;

  QMap<const QtProperty *, QtProperty *> m_propertyToX;
  QMap<const QtProperty *, QtProperty *> m_propertyToY;

  QMap<const QtProperty *, QtProperty *> m_xToProperty;
  QMap<const QtProperty *, QtProperty *> m_yToProperty;
};

class QtRectFPropertyManagerPrivate {
  QtRectFPropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtRectFPropertyManager)
public:
  void slotDoubleChanged(const QtProperty *property, double value);
  void slotPropertyDestroyed(const QtProperty *property);
  void setConstraint(QtProperty *property, const QRectF &constraint, const QRectF &val);

  struct Data {
    Data() : val(0, 0, 0, 0), decimals(2) {}
    QRectF val;
    QRectF constraint;
    int decimals;
  };

  using PropertyValueMap = QMap<const QtProperty *, Data>;
  PropertyValueMap m_values;

  QtDoublePropertyManager *m_doublePropertyManager;

  QMap<const QtProperty *, QtProperty *> m_propertyToX;
  QMap<const QtProperty *, QtProperty *> m_propertyToY;
  QMap<const QtProperty *, QtProperty *> m_propertyToW;
  QMap<const QtProperty *, QtProperty *> m_propertyToH;

  QMap<const QtProperty *, QtProperty *> m_xToProperty;
  QMap<const QtProperty *, QtProperty *> m_yToProperty;
  QMap<const QtProperty *, QtProperty *> m_wToProperty;
  QMap<const QtProperty *, QtProperty *> m_hToProperty;
};

class QtRectPropertyManagerPrivate {
  QtRectPropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtRectPropertyManager)
public:
  void slotIntChanged(const QtProperty *property, int value);
  void slotPropertyDestroyed(const QtProperty *property);
  void setConstraint(QtProperty *property, const QRect &constraint, const QRect &val);

  struct Data {
    Data() : val(0, 0, 0, 0) {}
    QRect val;
    QRect constraint;
  };

  using PropertyValueMap = QMap<const QtProperty *, Data>;
  PropertyValueMap m_values;

  QtIntPropertyManager *m_intPropertyManager;

  QMap<const QtProperty *, QtProperty *> m_propertyToX;
  QMap<const QtProperty *, QtProperty *> m_propertyToY;
  QMap<const QtProperty *, QtProperty *> m_propertyToW;
  QMap<const QtProperty *, QtProperty *> m_propertyToH;

  QMap<const QtProperty *, QtProperty *> m_xToProperty;
  QMap<const QtProperty *, QtProperty *> m_yToProperty;
  QMap<const QtProperty *, QtProperty *> m_wToProperty;
  QMap<const QtProperty *, QtProperty *> m_hToProperty;
};

class QtSizeFPropertyManagerPrivate {
  QtSizeFPropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtSizeFPropertyManager)
public:
  void slotDoubleChanged(const QtProperty *property, double value);
  void slotPropertyDestroyed(const QtProperty *property);
  void setValue(QtProperty *property, const QSizeF &val);
  void setRange(QtProperty *property, const QSizeF &minVal, const QSizeF &maxVal, const QSizeF &val);

  struct Data {
    Data() : val(QSizeF(0, 0)), minVal(QSizeF(0, 0)), maxVal(QSizeF(INT_MAX, INT_MAX)), decimals(2) {}
    QSizeF val;
    QSizeF minVal;
    QSizeF maxVal;
    int decimals;
    QSizeF minimumValue() const { return minVal; }
    QSizeF maximumValue() const { return maxVal; }
    void setMinimumValue(const QSizeF &newMinVal) { setSizeMinimumData(this, newMinVal); }
    void setMaximumValue(const QSizeF &newMaxVal) { setSizeMaximumData(this, newMaxVal); }
  };

  using PropertyValueMap = QMap<const QtProperty *, Data>;
  PropertyValueMap m_values;

  QtDoublePropertyManager *m_doublePropertyManager;

  QMap<const QtProperty *, QtProperty *> m_propertyToW;
  QMap<const QtProperty *, QtProperty *> m_propertyToH;

  QMap<const QtProperty *, QtProperty *> m_wToProperty;
  QMap<const QtProperty *, QtProperty *> m_hToProperty;
};

class QtSizePolicyPropertyManagerPrivate {
  QtSizePolicyPropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtSizePolicyPropertyManager)
public:
  QtSizePolicyPropertyManagerPrivate();

  void slotIntChanged(const QtProperty *property, int value);
  void slotEnumChanged(const QtProperty *property, int value);
  void slotPropertyDestroyed(const QtProperty *property);

  using PropertyValueMap = QMap<const QtProperty *, QSizePolicy>;
  PropertyValueMap m_values;

  QtIntPropertyManager *m_intPropertyManager;
  QtEnumPropertyManager *m_enumPropertyManager;

  QMap<const QtProperty *, QtProperty *> m_propertyToHPolicy;
  QMap<const QtProperty *, QtProperty *> m_propertyToVPolicy;
  QMap<const QtProperty *, QtProperty *> m_propertyToHStretch;
  QMap<const QtProperty *, QtProperty *> m_propertyToVStretch;

  QMap<const QtProperty *, QtProperty *> m_hPolicyToProperty;
  QMap<const QtProperty *, QtProperty *> m_vPolicyToProperty;
  QMap<const QtProperty *, QtProperty *> m_hStretchToProperty;
  QMap<const QtProperty *, QtProperty *> m_vStretchToProperty;
};

class QtSizePropertyManagerPrivate {
  QtSizePropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtSizePropertyManager)
public:
  void slotIntChanged(const QtProperty *property, int value);
  void slotPropertyDestroyed(const QtProperty *property);
  void setValue(QtProperty *property, const QSize &val);
  void setRange(QtProperty *property, const QSize &minVal, const QSize &maxVal, const QSize &val);

  struct Data {
    Data() : val(QSize(0, 0)), minVal(QSize(0, 0)), maxVal(QSize(INT_MAX, INT_MAX)) {}
    QSize val;
    QSize minVal;
    QSize maxVal;
    QSize minimumValue() const { return minVal; }
    QSize maximumValue() const { return maxVal; }
    void setMinimumValue(const QSize &newMinVal) { setSizeMinimumData(this, newMinVal); }
    void setMaximumValue(const QSize &newMaxVal) { setSizeMaximumData(this, newMaxVal); }
  };

  using PropertyValueMap = QMap<const QtProperty *, Data>;
  PropertyValueMap m_values;

  QtIntPropertyManager *m_intPropertyManager;

  QMap<const QtProperty *, QtProperty *> m_propertyToW;
  QMap<const QtProperty *, QtProperty *> m_propertyToH;

  QMap<const QtProperty *, QtProperty *> m_wToProperty;
  QMap<const QtProperty *, QtProperty *> m_hToProperty;
};

class QtMetaEnumWrapper : public QObject {
  Q_OBJECT
  Q_PROPERTY(QSizePolicy::Policy policy READ policy)
public:
  QSizePolicy::Policy policy() const { return QSizePolicy::Ignored; }

private:
  QtMetaEnumWrapper(QObject *parent) : QObject(parent) {}
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif
