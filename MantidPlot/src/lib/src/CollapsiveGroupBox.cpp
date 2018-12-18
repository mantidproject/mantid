// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
        File                 : CollapsiveGroupBox.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "CollapsiveGroupBox.h"

CollapsiveGroupBox::CollapsiveGroupBox(const QString &title, QWidget *parent)
    : QGroupBox(title, parent) {
  setCheckable(true);
  connect(this, SIGNAL(toggled(bool)), this, SLOT(setExpanded(bool)));
}

void CollapsiveGroupBox::setCollapsed(bool collapsed) {
  foreach (QObject *o, children()) {
    if (o->isWidgetType())
      ((QWidget *)o)->setVisible(collapsed);
  }

  setFlat(collapsed);
}

void CollapsiveGroupBox::setExpanded(bool expanded) {
  foreach (QObject *o, children()) {
    if (o->isWidgetType())
      ((QWidget *)o)->setVisible(expanded);
  }

  setFlat(!expanded);
}
