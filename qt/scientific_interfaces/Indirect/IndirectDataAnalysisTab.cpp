// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisTab.h"
#include "IndirectSettingsHelper.h"

#include "MantidAPI/MatrixWorkspace.h"

#include <QSettings>

using namespace Mantid::API;

namespace MantidQt::CustomInterfaces::IDA {

IndirectDataAnalysisTab::IndirectDataAnalysisTab(QWidget *parent) : IndirectTab(parent), m_selectedSpectrum(0) {}

/**
 * Prevents the loading of data with incorrect naming if passed true
 *
 * @param filter :: true if you want to allow filtering
 */
void IndirectDataAnalysisTab::filterInputData(bool filter) { setFileExtensionsByName(filter); }

/**
 * Retrieves the selected spectrum.
 *
 * @return  The selected spectrum.
 */
int IndirectDataAnalysisTab::getSelectedSpectrum() const { return m_selectedSpectrum; }

} // namespace MantidQt::CustomInterfaces::IDA
