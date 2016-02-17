/***************************************************************************
    File                 : TitlePicker.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Title picker

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include <QObject>

class QwtPlot;
class QwtTextLabel;

class TitlePicker: public QObject
{
    Q_OBJECT

public:
  explicit TitlePicker(QwtPlot *plot);
    void setSelected(bool select = true);
	bool selected(){return d_selected;};

signals:
	void clicked();
	void doubleClicked();
	void removeTitle();
	void showTitleMenu();

private:
  bool eventFilter(QObject *, QEvent *) override;
  QwtTextLabel *title;
  bool d_selected;
};
