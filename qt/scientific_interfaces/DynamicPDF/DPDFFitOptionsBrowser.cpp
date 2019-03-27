// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "DPDFFitOptionsBrowser.h"
// Mantid headers from other projects
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"
// 3rd party library headers
#include "MantidKernel/Logger.h"

namespace {
Mantid::Kernel::Logger g_log("DynamicPDF");
}

using Fittype = MantidQt::MantidWidgets::FitOptionsBrowser::FittingType;

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

/*               **********************
 *               **  Public Members  **
 *               **********************/

/**
 * @brief Constructor, override fitting type with Sequential
 */
DPDFFitOptionsBrowser::DPDFFitOptionsBrowser(QWidget *parent)
    : FitOptionsBrowser(parent, Fittype::Sequential) {
  this->createAdditionalProperties();
  this->customizeBrowser();
}

/*                ***********************
 *                **  Private Members  **
 *                ***********************/

/**
 * @brief Include properties not defined in the parent class
 */
void DPDFFitOptionsBrowser::createAdditionalProperties() {
  // fitting range
  m_startX = this->addDoubleProperty("StartX");
  m_endX = this->addDoubleProperty("EndX");
}

/**
 * @brief Show additional properties in the browser
 */
void DPDFFitOptionsBrowser::customizeBrowser() {
  // show the fitting range
  this->displayProperty("StartX");
  this->displayProperty("EndX");
  this->setProperty("CreateOutput", "true");
}

} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt
