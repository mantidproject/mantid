// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "FunctionTemplateBrowser.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/PropertyWithValue.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"
#include <iostream>
// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/ButtonEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/CompositeEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleDialogEditor.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>
#include <limits>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
 * Constructor
 * @param parent :: The parent widget.
 */
FunctionTemplateBrowser::FunctionTemplateBrowser(QWidget *parent)
    : QWidget(parent), m_decimals(6) {}

void FunctionTemplateBrowser::createBrowser() {
  m_stringManager = new QtStringPropertyManager(this);
  m_doubleManager = new QtDoublePropertyManager(this);
  m_intManager = new QtIntPropertyManager(this);
  m_boolManager = new QtBoolPropertyManager(this);
  m_enumManager = new QtEnumPropertyManager(this);
  m_groupManager = new QtGroupPropertyManager(this);
  m_parameterManager = new ParameterPropertyManager(this, true);

  // create editor factories
  QtSpinBoxFactory *spinBoxFactory = new QtSpinBoxFactory(this);
  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory(this);
  QtLineEditFactory *lineEditFactory = new QtLineEditFactory(this);
  QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(this);
  QtEnumEditorFactory *comboBoxFactory = new QtEnumEditorFactory(this);
  auto *doubleDialogFactory = new DoubleDialogEditorFactory(this, true);

  m_browser = new QtTreePropertyBrowser(nullptr, QStringList(), false);
  // assign factories to property managers
  m_browser->setFactoryForManager(m_stringManager, lineEditFactory);
  m_browser->setFactoryForManager(m_doubleManager, doubleEditorFactory);
  m_browser->setFactoryForManager(m_intManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_boolManager, checkBoxFactory);
  m_browser->setFactoryForManager(m_enumManager, comboBoxFactory);
  m_browser->setFactoryForManager(m_parameterManager, doubleDialogFactory);

  connect(m_intManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(intChanged(QtProperty *)));
  connect(m_boolManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(boolChanged(QtProperty *)));
  connect(m_enumManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(enumChanged(QtProperty *)));
  connect(m_parameterManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(parameterChanged(QtProperty *)));

  connect(doubleDialogFactory, SIGNAL(buttonClicked(QtProperty *)), this,
          SLOT(parameterButtonClicked(QtProperty *)));
  connect(doubleDialogFactory, SIGNAL(closeEditor()), m_browser,
          SLOT(closeEditor()));

  m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_browser, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(popupMenu(const QPoint &)));

  connect(m_browser, SIGNAL(optionChanged(QtProperty *, const QString &, bool)),
          this, SLOT(globalChanged(QtProperty *, const QString &, bool)));
}

void FunctionTemplateBrowser::init() {
  createBrowser();
  createProperties();
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addWidget(m_browser);
  layout->setContentsMargins(0, 0, 0, 0);
}

void FunctionTemplateBrowser::clear() { m_browser->clear(); }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
