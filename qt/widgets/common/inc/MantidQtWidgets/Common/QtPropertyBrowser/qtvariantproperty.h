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
#include <QIcon>
#include <QRegularExpression>
#include <QVariant>

QT_BEGIN_NAMESPACE

using QtIconMap = QMap<int, QIcon>;

class QtVariantPropertyManager;
class QtVariantPropertyPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtVariantProperty : public QtProperty {
public:
  ~QtVariantProperty() override;
  QtVariantProperty(const QtVariantProperty &) = delete;
  QtVariantProperty &operator=(const QtVariantProperty &) = delete;
  QVariant value() const;
  QVariant attributeValue(const QString &attribute) const;
  int valueType() const;
  int propertyType() const;

  void setValue(const QVariant &value);
  void setAttribute(const QString &attribute, const QVariant &value);

protected:
  QtVariantProperty(QtVariantPropertyManager *manager);

private:
  friend class QtVariantPropertyManager;
  QtVariantPropertyPrivate *d_ptr;
};

class QtVariantPropertyManagerPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtVariantPropertyManager : public QtAbstractPropertyManager {
  Q_OBJECT
public:
  QtVariantPropertyManager(QObject *parent = nullptr);
  ~QtVariantPropertyManager() override;

  virtual QtVariantProperty *addProperty(int propertyType, const QString &name = QString());

  int propertyType(const QtProperty *property) const;
  int valueType(const QtProperty *property) const;
  QtVariantProperty *variantProperty(const QtProperty *property) const;

  virtual bool isPropertyTypeSupported(int propertyType) const;
  virtual int valueType(int propertyType) const;
  virtual QStringList attributes(int propertyType) const;
  virtual int attributeType(int propertyType, const QString &attribute) const;

  virtual QVariant value(const QtProperty *property) const;
  virtual QVariant attributeValue(const QtProperty *property, const QString &attribute) const;

  static int enumTypeId();
  static int flagTypeId();
  static int groupTypeId();
  static int iconMapTypeId();
public Q_SLOTS:
  virtual void setValue(QtProperty *property, const QVariant &val);
  virtual void setAttribute(QtProperty *property, const QString &attribute, const QVariant &value);
Q_SIGNALS:
  void valueChanged(QtProperty *property, const QVariant &val);
  void attributeChanged(QtProperty *property, const QString &attribute, const QVariant &val);

protected:
  bool hasValue(const QtProperty *property) const override;
  QString valueText(const QtProperty *property) const override;
  QIcon valueIcon(const QtProperty *property) const override;
  void initializeProperty(QtProperty *property) override;
  void uninitializeProperty(QtProperty *property) override;
  QtProperty *createProperty() override;

private:
  QtVariantPropertyManagerPrivate *d_ptr;
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(const QtProperty *, int, int))
  Q_PRIVATE_SLOT(d_func(), void slotSingleStepChanged(const QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, double))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(const QtProperty *, double, double))
  Q_PRIVATE_SLOT(d_func(), void slotSingleStepChanged(const QtProperty *, double))
  Q_PRIVATE_SLOT(d_func(), void slotDecimalsChanged(const QtProperty *, int))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, bool))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QString &))
  Q_PRIVATE_SLOT(d_func(), void slotRegExpChanged(const QtProperty *, const QRegularExpression &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QDate &))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(const QtProperty *, const QDate &, const QDate &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QTime &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QDateTime &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QKeySequence &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QChar &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QLocale &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QPoint &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QPointF &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QSize &))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(const QtProperty *, const QSize &, const QSize &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QSizeF &))
  Q_PRIVATE_SLOT(d_func(), void slotRangeChanged(const QtProperty *, const QSizeF &, const QSizeF &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QRect &))
  Q_PRIVATE_SLOT(d_func(), void slotConstraintChanged(const QtProperty *, const QRect &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QRectF &))
  Q_PRIVATE_SLOT(d_func(), void slotConstraintChanged(const QtProperty *, const QRectF &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QColor &))
  Q_PRIVATE_SLOT(d_func(), void slotEnumNamesChanged(const QtProperty *, const QStringList &))
  Q_PRIVATE_SLOT(d_func(), void slotEnumIconsChanged(const QtProperty *, const QMap<int, QIcon> &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QSizePolicy &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QFont &))
  Q_PRIVATE_SLOT(d_func(), void slotValueChanged(const QtProperty *, const QCursor &))
  Q_PRIVATE_SLOT(d_func(), void slotFlagNamesChanged(const QtProperty *, const QStringList &))

  Q_PRIVATE_SLOT(d_func(), void slotPropertyInserted(QtProperty *, const QtProperty *, const QtProperty *))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyRemoved(const QtProperty *, QtProperty *))
  Q_DECLARE_PRIVATE(QtVariantPropertyManager)
  Q_DISABLE_COPY(QtVariantPropertyManager)
};

class QtVariantEditorFactoryPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtVariantEditorFactory : public QtAbstractEditorFactory<QtVariantPropertyManager> {
  Q_OBJECT
public:
  QtVariantEditorFactory(QObject *parent = nullptr);
  ~QtVariantEditorFactory() override;

protected:
  void connectPropertyManager(QtVariantPropertyManager *manager) override;
  QWidget *createEditorForManager(QtVariantPropertyManager *manager, QtProperty *property, QWidget *parent) override;
  void disconnectPropertyManager(QtVariantPropertyManager *manager) override;

private:
  QtVariantEditorFactoryPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtVariantEditorFactory)
  Q_DISABLE_COPY(QtVariantEditorFactory)
};

class QtVariantPropertyManagerPrivate {
  QtVariantPropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtVariantPropertyManager)
public:
  QtVariantPropertyManagerPrivate();

  bool m_creatingProperty;
  bool m_creatingSubProperties;
  bool m_destroyingSubProperties;
  int m_propertyType;

  void slotValueChanged(const QtProperty *property, int val);
  void slotRangeChanged(const QtProperty *property, int min, int max);
  void slotSingleStepChanged(const QtProperty *property, int step);
  void slotValueChanged(const QtProperty *property, double val);
  void slotRangeChanged(const QtProperty *property, double min, double max);
  void slotSingleStepChanged(const QtProperty *property, double step);
  void slotDecimalsChanged(const QtProperty *property, int prec);
  void slotValueChanged(const QtProperty *property, bool val);
  void slotValueChanged(const QtProperty *property, const QString &val);
  void slotRegExpChanged(const QtProperty *property, const QRegularExpression &regExp);
  void slotValueChanged(const QtProperty *property, const QDate &val);
  void slotRangeChanged(const QtProperty *property, const QDate &min, const QDate &max);
  void slotValueChanged(const QtProperty *property, const QTime &val);
  void slotValueChanged(const QtProperty *property, const QDateTime &val);
  void slotValueChanged(const QtProperty *property, const QKeySequence &val);
  void slotValueChanged(const QtProperty *property, const QChar &val);
  void slotValueChanged(const QtProperty *property, const QLocale &val);
  void slotValueChanged(const QtProperty *property, const QPointF &val);
  void slotValueChanged(const QtProperty *property, const QPoint &val);
  void slotValueChanged(const QtProperty *property, const QSize &val);
  void slotRangeChanged(const QtProperty *property, const QSize &min, const QSize &max);
  void slotValueChanged(const QtProperty *property, const QSizeF &val);
  void slotRangeChanged(const QtProperty *property, const QSizeF &min, const QSizeF &max);
  void slotValueChanged(const QtProperty *property, const QRect &val);
  void slotConstraintChanged(const QtProperty *property, const QRect &val);
  void slotValueChanged(const QtProperty *property, const QRectF &val);
  void slotConstraintChanged(const QtProperty *property, const QRectF &val);
  void slotValueChanged(const QtProperty *property, const QColor &val);
  void slotEnumChanged(QtProperty *property, int val);
  void slotEnumNamesChanged(const QtProperty *property, const QStringList &enumNames);
  void slotEnumIconsChanged(const QtProperty *property, const QMap<int, QIcon> &enumIcons);
  void slotValueChanged(const QtProperty *property, const QSizePolicy &val);
  void slotValueChanged(const QtProperty *property, const QFont &val);
  void slotValueChanged(const QtProperty *property, const QCursor &val);
  void slotFlagChanged(QtProperty *property, int val);
  void slotFlagNamesChanged(const QtProperty *property, const QStringList &flagNames);
  void slotPropertyInserted(QtProperty *property, const QtProperty *parent, const QtProperty *after);
  void slotPropertyRemoved(const QtProperty *property, QtProperty *parent);

  void valueChanged(const QtProperty *property, const QVariant &val);

  int internalPropertyToType(const QtProperty *property) const;
  QtVariantProperty *createSubProperty(QtVariantProperty *parent, QtVariantProperty *after, QtProperty *internal);
  void removeSubProperty(QtVariantProperty *property);

  QMap<int, QtAbstractPropertyManager *> m_typeToPropertyManager;
  QMap<int, QMap<QString, int>> m_typeToAttributeToAttributeType;

  QMap<const QtProperty *, QPair<QtVariantProperty *, int>> m_propertyToType;

  QMap<int, int> m_typeToValueType;

  QMap<const QtProperty *, QtVariantProperty *> m_internalToProperty;

  const QString m_constraintAttribute;
  const QString m_singleStepAttribute;
  const QString m_decimalsAttribute;
  const QString m_enumIconsAttribute;
  const QString m_enumNamesAttribute;
  const QString m_flagNamesAttribute;
  const QString m_maximumAttribute;
  const QString m_minimumAttribute;
  const QString m_regExpAttribute;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QIcon)
Q_DECLARE_METATYPE(QtIconMap)
