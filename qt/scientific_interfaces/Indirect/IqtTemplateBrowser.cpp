// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IqtTemplateBrowser.h"

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
  IqtTemplateBrowser::IqtTemplateBrowser(QWidget *parent)
    : FunctionTemplateBrowser(parent) {
}

void IqtTemplateBrowser::createProperties()
{
  // Exponentials
  m_numberOfExponentials = m_intManager->addProperty("Exponential");
  m_hasStretchExponential = m_boolManager->addProperty("Stretch Exponential");
  m_exponentialsGroup = m_groupManager->addProperty("Exponentials");
  m_exponentialsGroup->addSubProperty(m_numberOfExponentials);
  m_exponentialsGroup->addSubProperty(m_hasStretchExponential);
  m_browser->addProperty(m_exponentialsGroup);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
