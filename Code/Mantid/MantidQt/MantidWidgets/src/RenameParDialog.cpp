#include "MantidQtMantidWidgets/RenameParDialog.h"

#include <algorithm>

using namespace MantidQt::MantidWidgets;

/**
 * Constructor.
 * @params Parameter names to rename
 */
RenameParDialog::RenameParDialog(
        const std::vector<std::string>& old_params,
        const std::vector<std::string>& new_params,
        QWidget *parent)
:QDialog(parent),
m_old_params(old_params),
m_new_params(new_params)
{
  m_uiForm.setupUi(this);
  QAbstractItemModel* model = m_uiForm.tableWidget->model();
  model->insertRows(0,new_params.size());
  for(int row = 0;row < static_cast<int>(new_params.size()); ++row)
  {
    QString par = QString::fromStdString(new_params[row]);
    model->setData(model->index(row,0),par);
    model->setData(model->index(row,1),par);
  }
  connect(m_uiForm.btnRename,SIGNAL(clicked()),this,SLOT(accept()));
  connect(m_uiForm.btnCancel,SIGNAL(clicked()),this,SLOT(reject()));
  connect(m_uiForm.rbAddIndex,SIGNAL(toggled(bool)),this,SLOT(uniqueIndexedNames(bool)));
  connect(m_uiForm.rbDoNot,SIGNAL(toggled(bool)),this,SLOT(doNotRename(bool)));
}

/**
 * Checks whether a name is unique. The name is compared to the names in 
 * m_old_names and in column #1 of the table widget
 */
bool RenameParDialog::isUnique(const QString& name)const
{
  std::vector<std::string>::const_iterator it = 
    std::find(m_old_params.begin(),m_old_params.end(),name.toStdString());
  if (it != m_old_params.end()) return false;
  QAbstractItemModel* model = m_uiForm.tableWidget->model();
  for(int row=0;row< m_uiForm.tableWidget->rowCount(); ++row)
  {
    if (model->data(model->index(row,1)).toString() == name) 
    {
      return false;
    }
  }
  return true;
}

/**
 * Adds a suffix to the unput parameter name in the form: _n where n is a number.
 * The method ensures that the new name is unique
 * @param name The name to rename
 */
QString RenameParDialog::makeUniqueIndexedName(const QString& name)
{
  int index = 1;
  QString base;
  int i_ = name.indexOf('_');
  if (i_ >= 0)
  {
    QString old_index = name.mid(i_+1);
    bool ok;
    int n = old_index.toInt(&ok);
    // e.g. name = a_3
    if (ok)
    {
      index = n + 1;
      base = name.mid(0,i_);
    }
    // e.g. name = a_b
    else
    {
      base = name;
    }
  }
  else
  {
    base = name + "_";
  }
  QString tst(base + QString::number(index));
  while( !isUnique(tst) )
  {
    ++index;
    tst = base + QString::number(index);
  }
  return tst;
}

/**
 * Output the new names to a vector 
 * @param out Reference to a vector for output
 */
void RenameParDialog::setOutput(std::vector<std::string>& out)const
{
  out.clear();
  QAbstractItemModel* model = m_uiForm.tableWidget->model();
  for(int row=0;row< m_uiForm.tableWidget->rowCount(); ++row)
  {
    out.push_back(model->data(model->index(row,1)).toString().toStdString());
  }
}

void RenameParDialog::uniqueIndexedNames(bool ok)
{
  if (!ok) return;
  QAbstractItemModel* model = m_uiForm.tableWidget->model();
  for(int row=0;row< m_uiForm.tableWidget->rowCount(); ++row)
  {
    QString name = model->data(model->index(row,0)).toString();
    model->setData(model->index(row,1),makeUniqueIndexedName(name));
  }
}

/**
 * Do not rename the parameters
 */
void RenameParDialog::doNotRename(bool ok)
{
  if (!ok) return;
  QAbstractItemModel* model = m_uiForm.tableWidget->model();
  for(int row=0;row< m_uiForm.tableWidget->rowCount(); ++row)
  {
    QString name = model->data(model->index(row,0)).toString();
    model->setData(model->index(row,1),name);
  }
}
