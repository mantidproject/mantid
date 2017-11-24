#include "IndirectDataAnalysisTab.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "boost/shared_ptr.hpp"

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <QSettings>
#include <QString>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
/**
 * Constructor.
 *
 * @param parent :: the parent widget (an IndirectDataAnalysis object).
 */
IndirectDataAnalysisTab::IndirectDataAnalysisTab(QWidget *parent)
    : IndirectTab(parent), m_dblEdFac(NULL), m_blnEdFac(NULL), m_parent(NULL) {
  m_parent = dynamic_cast<IndirectDataAnalysis *>(parent);

  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory(this);
  m_blnEdFac = new QtCheckBoxFactory(this);
}

/**
 * Loads the tab's settings.
 *
 * Calls overridden version of loadSettings() in child class.
 *
 * @param settings :: the QSettings object from which to load
 */
void IndirectDataAnalysisTab::loadTabSettings(const QSettings &settings) {
  loadSettings(settings);
}

/**
 * Slot that can be called when a user edits an input.
 */
void IndirectDataAnalysisTab::inputChanged() { validate(); }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
