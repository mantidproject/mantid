//---------------------------------------
// Includes
//---------------------------------------

#include "UserFitFunctionDialog.h"
#include <muParser.h>
#include <qcheckbox.h>
#include <qmessagebox.h>
#include <qheaderview.h>
#include <iostream>

//---------------------------------------
// Public member functions
//---------------------------------------

/// Constructor
UserFitFunctionDialog::UserFitFunctionDialog(QWidget* parent) :
  QDialog(parent)
{
  ui.setupUi(this);

  ui.btnAdd->setEnabled(false);

  connect(ui.btnAdd,SIGNAL(clicked()),this,SLOT(addFunction()));
  connect(ui.btnMultiply,SIGNAL(clicked()),this,SLOT(multiplyFunction()));
  connect(ui.btnInsert,SIGNAL(clicked()),this,SLOT(insertFunction()));
  connect(ui.treeFunctions,SIGNAL(itemSelectionChanged()),this,SLOT(functionSelectionChanged()));

}

void UserFitFunctionDialog::addFunction()
{
  addFunction("+",false);
}

void UserFitFunctionDialog::multiplyFunction()
{
  addFunction("*",true);
}

void UserFitFunctionDialog::insertFunction()
{
  addFunction("",false);
}

void UserFitFunctionDialog::addFunction(const QString& op,bool brackets)
{
  ui.teExpression->setFocus();

  QList<QTreeWidgetItem*> selection = ui.treeFunctions->selectedItems();
  if (selection.size() == 0) return;

  QTreeWidgetItem* item = selection.first();
  QTreeWidgetItem* parentItem = item->parent();

  if (parentItem == NULL) return; // this sould never happen, just in case

  if (parentItem->parent() != NULL) item = parentItem;

  //QTextCursor cursor = ui.teExpression->textCursor();
  //cursor.insertText( item->text(1) );

  QString oper = ui.teExpression->toPlainText().isEmpty() ? "" : op;
  QString expr = item->text(1);
  if (brackets) expr = QString("(") + expr + ")";
  expr = oper + expr;
  ui.teExpression->insertPlainText( expr );

  if (item->childCount() == 0 || !ui.lePeakParams->text().isEmpty()) return;

  ui.lePeakParams->setText( item->child(0)->text(1) );
  ui.leWidthFormula->setText( item->child(1)->text(1) );

}

void UserFitFunctionDialog::functionSelectionChanged()
{
  QItemSelectionModel* selection = ui.treeFunctions->selectionModel();
  if (!selection->hasSelection())
  {
    ui.btnAdd->setEnabled(false);
    return;
  }
  QModelIndexList indexList = selection->selectedIndexes();
  QModelIndex index = indexList.first();
  if (index.parent()== QModelIndex())
  {
    ui.btnAdd->setEnabled(false);
    return;
  }
  ui.btnAdd->setEnabled(true);
}
