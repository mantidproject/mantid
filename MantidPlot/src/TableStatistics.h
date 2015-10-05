/***************************************************************************
	File                 : TableStatistics.h
	Project              : QtiPlot
--------------------------------------------------------------------
	Copyright            : (C) 2006 by Knut Franke
	Email (use @ for *)  : knut.franke*gmx.de
	Description          : Table subclass that displays statistics on
	                       columns or rows of another table

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
#ifndef TABLE_STATISTICS_H
#define TABLE_STATISTICS_H

#include "Table.h"

/**\brief Table that computes and displays statistics on another Table.
 *
 * \section future Future Plans
 * Make it possible to add new columns/rows to be monitored.
 */
class TableStatistics : public Table
{
	Q_OBJECT

	public:
		//! supported statistics types
		enum Type { row, column };
		TableStatistics(ScriptingEnv *env, ApplicationWindow *parent, Table *base, Type, QList<int> targets);
		//! return the type of statistics
		Type type() const { return d_type; }
		//! return the base table of which statistics are displayed
		Table *base() const { return d_base; }

    void loadFromProject(const std::string& lines, ApplicationWindow* app, const int fileVersion);
    std::string saveToProject(ApplicationWindow* app);

    public slots:
        //! update statistics after a column has changed (to be connected with Table::modifiedData)
        void update(Table*, const QString& colName);
		//! handle renaming of columns (to be connected with Table::changedColHeader)
		void renameCol(const QString&, const QString&);
		//! remove statistics of removed columns (to be connected with Table::removedCol)
		void removeCol(const QString&);

	private slots:
		void closedBase();
	
	private:
		Table *d_base;
		Type d_type;
		QList<int> d_targets;
};

#endif
