
#include "MantidQtMantidWidgets/ICatUtils.h"
#include <QMdiSubWindow>
#include <QStringList>
#include <QFont>
#include<QHeaderView>

using namespace MantidQt::MantidWidgets;

using namespace Mantid::API;

/**This method updates the search result to search tree
 *@param ws_sptr workspace shared pointer
 *@param tablewidget pointer to table widget
*/ 
void ICatUtils::updatesearchResults(Mantid::API::ITableWorkspace_sptr& ws_sptr,QTableWidget* tablewidget )
{
	if(!ws_sptr || ws_sptr->rowCount()==0)
	{
		return ;
	}
	// reset the background color to white
	// if it's not reset to white alternating colour is not working.
	tablewidget->setStyleSheet("background-color: rgb(255, 255, 255)");
	//now set alternating color flag
	tablewidget->setAlternatingRowColors(true);
	//stylesheet for alternating background color
	tablewidget->setStyleSheet("alternate-background-color: rgb(216, 225, 255)");
	//disable  sorting as per QT documentation.otherwise  setitem will give undesired results
	tablewidget->setSortingEnabled(false);

	tablewidget->verticalHeader()->setVisible(false);

	//below for loop is for clearing the table widget on search button click.Bcoz Each click on search button to load data,rows were getting appended.
	// table widget clear() method is clearing only the tablewidgetitem text,not removing the rows,columns
	// so i'm using removeRow().When I removed the row from top of the table it was not working.so the for loop starts from bottom to top
	for (int i=tablewidget->rowCount()-1;i>=0;--i)
	{
		tablewidget->removeRow(i);
	}
	
	for (int i=0;i<ws_sptr->rowCount();++i)
	{
		tablewidget->insertRow(i);
		//setting the row height of tableWidget 
		tablewidget->setRowHeight(i,20);
	}
	
	QStringList qlabelList;
	//QBrush brush;
	//QColor color(255,0,0);
	//brush.setColor(color);
	for(int i=0;i<ws_sptr->columnCount();i++)
	{
		Column_sptr col_sptr = ws_sptr->getColumn(i);
		//get the column name to display as the header of table widget
		QString colTitle = QString::fromStdString(col_sptr->name());
		qlabelList.push_back(colTitle);
	
		for(int j=0;j<ws_sptr->rowCount();++j)
		{
		    std::ostringstream ostr;
		     col_sptr->print(ostr,j);
						 
			 QTableWidgetItem *newItem  = new QTableWidgetItem(QString::fromStdString(ostr.str()));
			 newItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
			 tablewidget->setItem(j,i, newItem);
			 newItem->setToolTip(QString::fromStdString(ostr.str()));
		}
	}
	QFont font;
	font.setBold(true);
	//setting table widget header labels from table workspace
	tablewidget->setHorizontalHeaderLabels(qlabelList);
	for (int i=0;i<tablewidget->columnCount();++i)
	{
		tablewidget->horizontalHeaderItem(i)->setFont(font);
	}
	//sorting by title
	tablewidget->sortByColumn(2,Qt::AscendingOrder);
	// resizing the coulmn based on data size
	tablewidget->resizeColumnsToContents ();
	//enable sorting
	tablewidget->setSortingEnabled(true);

}
/**This method is called when an investigation is selected  from investigations list
 *@param item  table widget item
 */
void ICatUtils::investigationSelected(QTableWidget* tablewidget,QTableWidgetItem* item,
									   QWidget* parent,Mantid::API::ITableWorkspace_sptr ws_sptr )
{
	if(!item) return ;
	int row=item->row();

	// column zero is investigation id
	QTableWidgetItem* invstItem = tablewidget->item(row,0);
	QString qinvstId = invstItem->text();
	long long invstId = qinvstId.toLongLong();
    
	//column one is RbNumber
	QTableWidgetItem* rbNumberItem = tablewidget->item(row,1);
	if(!rbNumberItem) return;
    QString qRbNumber = rbNumberItem->text();
	///column two is Title
	QTableWidgetItem* titleItem = tablewidget->item(row,2);
	if(!titleItem)return ;
	QString qTitle = titleItem->text();
    //column 4 is Instrument
	QTableWidgetItem* instrumentItem = tablewidget->item(row,3);
	if(!instrumentItem)return;
	QString qInstrument = instrumentItem->text();
		
	//parent of user_win is application window;
	QMdiSubWindow* usr_win = new QMdiSubWindow(parent);
	if(!usr_win) return;
	usr_win->setAttribute(Qt::WA_DeleteOnClose, false);

	m_invstWidget= new ICatInvestigation(invstId,qRbNumber,qTitle,qInstrument,ws_sptr,usr_win);
	if( m_invstWidget )
	{ 
		QRect frame = QRect(usr_win->frameGeometry().topLeft() - usr_win->geometry().topLeft(), 
			usr_win->geometry().bottomRight() - usr_win->geometry().bottomRight());
		usr_win->setWidget(m_invstWidget);
		QRect iface_geom = QRect(frame.topLeft() + m_invstWidget->geometry().topLeft(), 
			frame.bottomRight() + m_invstWidget->geometry().bottomRight()+QPoint(15,35));
		usr_win->setGeometry(iface_geom);
		usr_win->move(QPoint(600, 400));
		usr_win->show();
	}
}

SearchCalendar::SearchCalendar(QWidget* par):QCalendarWidget(par)
{
}
void SearchCalendar::leaveEvent(QEvent*)
{
	this->close();
}