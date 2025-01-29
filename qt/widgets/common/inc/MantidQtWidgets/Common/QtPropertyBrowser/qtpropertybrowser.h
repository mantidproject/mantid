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

#include "MantidQtWidgets/Common/DllOption.h"
#include <QMap>
#include <QSet>
#include <QWidget>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QtAbstractPropertyManager;
class QtPropertyPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtProperty {
public:
  virtual ~QtProperty();
  QtProperty(const QtProperty &) = delete;
  QtProperty &operator=(const QtProperty &) = delete;

  const QList<QtProperty *> &subProperties() const;

  QtAbstractPropertyManager *propertyManager() const;

  QString toolTip() const;
  QString statusTip() const;

  const QString &whatsThis() const;
  const QString &propertyName() const;
  bool isEnabled() const;
  bool isModified() const;

  bool hasValue() const;
  QIcon valueIcon() const;
  QString valueText() const;

  void setToolTip(const QString &text);
  void setStatusTip(const QString &text);
  void setWhatsThis(const QString &text);
  void setPropertyName(const QString &text);
  void setEnabled(bool enable);
  void setModified(bool modified);

  void addSubProperty(QtProperty *property);
  void insertSubProperty(QtProperty *property, QtProperty *afterProperty);
  void removeSubProperty(QtProperty *property);

  bool hasOption(const QString &opt) const;
  bool checkOption(const QString &opt) const;
  void setOption(const QString &opt, bool on);

protected:
  explicit QtProperty(QtAbstractPropertyManager *manager);
  void propertyChanged();

private:
  friend class QtAbstractPropertyManager;
  QtPropertyPrivate *d_ptr;
};

// class QtAbstractPropertyManagerPrivate;
class QtAbstractPropertyManagerPrivate {
  QtAbstractPropertyManager *q_ptr;
  Q_DECLARE_PUBLIC(QtAbstractPropertyManager)
public:
  void propertyDestroyed(QtProperty *property);
  void propertyChanged(QtProperty *property) const;
  void propertyRemoved(QtProperty *property, QtProperty *parentProperty) const;
  void propertyInserted(QtProperty *property, QtProperty *parentProperty, QtProperty *afterProperty) const;
  QSet<QtProperty *> m_properties;
};

class EXPORT_OPT_MANTIDQT_COMMON QtAbstractPropertyManager : public QObject {
  Q_OBJECT
public:
  explicit QtAbstractPropertyManager(QObject *parent = nullptr);
  ~QtAbstractPropertyManager() override;

  const QSet<QtProperty *> &properties() const;

  void clear() const;
  bool hasProperty(QtProperty *const prop) const;

  QtProperty *addProperty(const QString &name = QString());
Q_SIGNALS:

  void propertyInserted(QtProperty *property, QtProperty *parent, QtProperty *after);
  void propertyChanged(QtProperty *property);
  void propertyRemoved(QtProperty *property, QtProperty *parent);
  void propertyDestroyed(QtProperty *property);

protected:
  virtual bool hasValue(const QtProperty *property) const;
  virtual QIcon valueIcon(const QtProperty *property) const;
  virtual QString valueText(const QtProperty *property) const;
  virtual void initializeProperty(QtProperty *property) = 0;
  virtual void uninitializeProperty(QtProperty *property);
  virtual QtProperty *createProperty();

private:
  friend class QtProperty;
  QtAbstractPropertyManagerPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtAbstractPropertyManager)
  Q_DISABLE_COPY(QtAbstractPropertyManager)
};

class EXPORT_OPT_MANTIDQT_COMMON QtAbstractEditorFactoryBase : public QObject {
  Q_OBJECT
public:
  virtual QWidget *createEditor(QtProperty *property, QWidget *parent) = 0;

protected:
  explicit QtAbstractEditorFactoryBase(QObject *parent = nullptr) : QObject(parent) {}

  virtual void breakConnection(QtAbstractPropertyManager *manager) = 0;
protected Q_SLOTS:
  virtual void managerDestroyed(QObject *manager) = 0;

  friend class QtAbstractPropertyBrowser;
};

template <class PropertyManager> class CompositeEditorFactory;

template <class PropertyManager> class QtAbstractEditorFactory : public QtAbstractEditorFactoryBase {
public:
  explicit QtAbstractEditorFactory(QObject *parent) : QtAbstractEditorFactoryBase(parent) {}
  QWidget *createEditor(QtProperty *property, QWidget *parent) override {
    QSetIterator<PropertyManager *> it(m_managers);
    while (it.hasNext()) {
      PropertyManager *manager = it.next();
      if (manager == property->propertyManager()) {
        return createEditorForManager(manager, property, parent);
      }
    }
    return nullptr;
  }
  void addPropertyManager(PropertyManager *manager) {
    if (m_managers.contains(manager))
      return;
    m_managers.insert(manager);
    connectPropertyManager(manager);
    connect(manager, SIGNAL(destroyed(QObject *)), this, SLOT(managerDestroyed(QObject *)));
  }
  void removePropertyManager(PropertyManager *manager) {
    if (!m_managers.contains(manager))
      return;
    disconnect(manager, SIGNAL(destroyed(QObject *)), this, SLOT(managerDestroyed(QObject *)));
    disconnectPropertyManager(manager);
    m_managers.remove(manager);
  }

  const QSet<PropertyManager *> &propertyManagers() const { return m_managers; }

  PropertyManager *propertyManager(QtProperty *property) const {
    const QtAbstractPropertyManager *manager = property->propertyManager();
    QSetIterator<PropertyManager *> itManager(m_managers);
    while (itManager.hasNext()) {
      PropertyManager *m = itManager.next();
      if (m == manager) {
        return m;
      }
    }
    return nullptr;
  }

protected:
  friend class CompositeEditorFactory<PropertyManager>;
  virtual void connectPropertyManager(PropertyManager *manager) = 0;
  virtual QWidget *createEditorForManager(PropertyManager *manager, QtProperty *property, QWidget *parent) = 0;
  virtual void disconnectPropertyManager(PropertyManager *manager) = 0;
  void managerDestroyed(QObject *manager) override {
    QSetIterator<PropertyManager *> it(m_managers);
    while (it.hasNext()) {
      PropertyManager *m = it.next();
      if (m == manager) {
        m_managers.remove(m);
        return;
      }
    }
  }

private:
  void breakConnection(QtAbstractPropertyManager *manager) override {
    QSetIterator<PropertyManager *> it(m_managers);
    while (it.hasNext()) {
      PropertyManager *m = it.next();
      if (m == manager) {
        removePropertyManager(m);
        return;
      }
    }
  }

private:
  QSet<PropertyManager *> m_managers;
  friend class QtAbstractPropertyEditor;
};

class QtAbstractPropertyBrowser;
class QtBrowserItemPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtBrowserItem {
public:
  QtBrowserItem(const QtBrowserItem &) = delete;
  QtBrowserItem &operator=(const QtBrowserItem &) = delete;
  QtProperty *property() const;
  QtBrowserItem *parent() const;

  const QList<QtBrowserItem *> &children() const;
  QtAbstractPropertyBrowser *browser() const;

private:
  explicit QtBrowserItem(QtAbstractPropertyBrowser *browser, QtProperty *property, QtBrowserItem *parent);
  ~QtBrowserItem();
  QtBrowserItemPrivate *d_ptr;
  friend class QtAbstractPropertyBrowserPrivate;
};

class QtAbstractPropertyBrowserPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtAbstractPropertyBrowser : public QWidget {
  Q_OBJECT
public:
  explicit QtAbstractPropertyBrowser(QWidget *parent = nullptr);
  ~QtAbstractPropertyBrowser() override;

  const QList<QtProperty *> &properties() const;
  QList<QtBrowserItem *> items(QtProperty *property) const;
  QtBrowserItem *topLevelItem(QtProperty *property) const;
  const QList<QtBrowserItem *> &topLevelItems() const;

  void clear();

  template <class PropertyManager>
  void setFactoryForManager(PropertyManager *manager, QtAbstractEditorFactory<PropertyManager> *factory) {
    QtAbstractPropertyManager *abstractManager = manager;
    QtAbstractEditorFactoryBase *abstractFactory = factory;

    if (addFactory(abstractManager, abstractFactory))
      factory->addPropertyManager(manager);
  }

  void unsetFactoryForManager(QtAbstractPropertyManager *manager);

  QtBrowserItem *currentItem() const;
  void setCurrentItem(QtBrowserItem * /*item*/);

Q_SIGNALS:
  void currentItemChanged(QtBrowserItem * /*_t1*/);

public Q_SLOTS:

  QtBrowserItem *addProperty(QtProperty *property);
  QtBrowserItem *insertProperty(QtProperty *property, QtProperty *afterProperty);
  void removeProperty(QtProperty *property);

protected:
  virtual void itemInserted(QtBrowserItem *item, QtBrowserItem *afterItem) = 0;
  virtual void itemRemoved(QtBrowserItem *item) = 0;
  // can be tooltip, statustip, whatsthis, name, icon, text.
  virtual void itemChanged(QtBrowserItem *item) = 0;

  virtual QWidget *createEditor(QtProperty *property, QWidget *parent);

private:
  bool addFactory(QtAbstractPropertyManager *abstractManager, QtAbstractEditorFactoryBase *abstractFactory);

  QtAbstractPropertyBrowserPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtAbstractPropertyBrowser)
  Q_DISABLE_COPY(QtAbstractPropertyBrowser)
  Q_PRIVATE_SLOT(d_func(), void slotPropertyInserted(QtProperty *, QtProperty *, QtProperty *))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyRemoved(QtProperty *, QtProperty *))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDestroyed(QtProperty *))
  Q_PRIVATE_SLOT(d_func(), void slotPropertyDataChanged(QtProperty *))
};

class QtPropertyPrivate {
public:
  QtPropertyPrivate(QtAbstractPropertyManager *manager) : m_enabled(true), m_modified(false), m_manager(manager) {}
  QtProperty *q_ptr;

  QSet<QtProperty *> m_parentItems;
  QList<QtProperty *> m_subItems;
  QMap<QString, bool> m_options;

  QString m_toolTip;
  QString m_statusTip;
  QString m_whatsThis;
  QString m_name;
  bool m_enabled;
  bool m_modified;

  QtAbstractPropertyManager *const m_manager;
};

class QtAbstractPropertyBrowserPrivate {
  QtAbstractPropertyBrowser *q_ptr;
  Q_DECLARE_PUBLIC(QtAbstractPropertyBrowser)
public:
  QtAbstractPropertyBrowserPrivate();

  void insertSubTree(QtProperty *property, QtProperty *parentProperty);
  void removeSubTree(QtProperty *property, QtProperty *parentProperty);
  void createBrowserIndexes(QtProperty *property, QtProperty *parentProperty, QtProperty *afterProperty);
  void removeBrowserIndexes(QtProperty *property, QtProperty *parentProperty);
  QtBrowserItem *createBrowserIndex(QtProperty *property, QtBrowserItem *parentIndex, QtBrowserItem *afterIndex);
  void removeBrowserIndex(QtBrowserItem *index);
  void clearIndex(QtBrowserItem *index);

  void slotPropertyInserted(QtProperty *property, QtProperty *parentProperty, QtProperty *afterProperty);
  void slotPropertyRemoved(QtProperty *property, QtProperty *parentProperty);
  void slotPropertyDestroyed(QtProperty *property);
  void slotPropertyDataChanged(QtProperty *property);

  QList<QtProperty *> m_subItems;
  QMap<QtAbstractPropertyManager *, QList<QtProperty *>> m_managerToProperties;
  QMap<QtProperty *, QList<QtProperty *>> m_propertyToParents;

  QMap<QtProperty *, QtBrowserItem *> m_topLevelPropertyToIndex;
  QList<QtBrowserItem *> m_topLevelIndexes;
  QMap<QtProperty *, QList<QtBrowserItem *>> m_propertyToIndexes;

  QtBrowserItem *m_currentItem;
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif
