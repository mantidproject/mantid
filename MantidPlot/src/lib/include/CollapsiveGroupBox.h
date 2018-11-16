// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
        File                 : CollapsiveGroupBox.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

#ifndef COLLAPSIVE_GROUP_BOX__H
#define COLLAPSIVE_GROUP_BOX__H

#include <QGroupBox>

//! A collapsive QGroupBox.

class CollapsiveGroupBox : public QGroupBox {
  Q_OBJECT

public:
  //! Constructor.
  /**
   * \param parent parent widget (only affects placement of the widget)
   */
  CollapsiveGroupBox(const QString &title = QString(),
                     QWidget *parent = nullptr);

public slots:
  void setCollapsed(bool collapsed = true);
  void setExpanded(bool expanded = true);
};

#endif
