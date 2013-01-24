#include "MantidQtAPI/ScriptRepositoryView.h"
#include "MantidQtAPI/RepoModel.h"
#include  <QSortFilterProxyModel>
#include <QDebug>
namespace MantidQt
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ScriptRepositoryView::ScriptRepositoryView(QWidget * parent):
    QDialog(parent),
    ui(new Ui::ScriptRepositoryView)
  {
    ui->setupUi(this); 
    model = new RepoModel();   
    ui->repo_treeView->setModel(model);

    ui->repo_treeView->setItemDelegateForColumn(1, new RepoDelegate(this));
    ui->repo_treeView->setItemDelegateForColumn(2, new RepoDelegate(this));
    ui->repo_treeView->setItemDelegateForColumn(3, new RepoDelegate(this));
    
    /*
    QSortFilterProxyModel * proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setFilterKeyColumn (4); 

    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive); 

    ui->repo_treeView->setModel(proxyModel); 
    */

    ui->repo_treeView->setColumnWidth(0,290);
    ui->repo_treeView->setColumnHidden(3,true);
    ui->repo_treeView->setColumnHidden(4,true);
    connect(ui->repo_treeView, SIGNAL(activated(const QModelIndex &)),
            this, SLOT(cell_activated(const QModelIndex&)));
    connect(ui->repo_treeView, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(cell_clicked(const QModelIndex&)));
    connect(model, SIGNAL(fileDescription(const QString)),
            ui->desc_textBrowser, SLOT(setText(const QString &)));
    connect(model,SIGNAL(loadScript(const QString)),
    this, SIGNAL(loadScript(const QString)));
    //   connect(ui->filter_lineEdit, SIGNAL(textChanged(QString)),
    //       this, SLOT(filterValues(QString)));
    
 }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ScriptRepositoryView::~ScriptRepositoryView()
  {
    delete ui;
  }

  void ScriptRepositoryView::filterValues(QString input){
    ui->repo_treeView->expandAll();
    QSortFilterProxyModel * model = qobject_cast<QSortFilterProxyModel*>( ui->repo_treeView->model());
    if (model)
      model->setFilterFixedString(input);
  }
  
  void ScriptRepositoryView::cell_activated(const QModelIndex & in){

    QSortFilterProxyModel * proxyModel = qobject_cast<QSortFilterProxyModel*>( ui->repo_treeView->model());
    if (proxyModel){
      model->fileSelected(proxyModel->mapToSource(in));
      return;
    }
    RepoModel * _model = qobject_cast<RepoModel*>(ui->repo_treeView->model());
    if (_model){
      _model->fileSelected(in);
      return;
    }
  }

  void ScriptRepositoryView::cell_clicked(const QModelIndex & in){
    qDebug() << "Cell activated\n"; 
    QSortFilterProxyModel * proxyModel = qobject_cast<QSortFilterProxyModel*>( ui->repo_treeView->model());
    if (proxyModel){
      model->entrySelected(proxyModel->mapToSource(in));
      return;
    }

    RepoModel * _model = qobject_cast<RepoModel*>(ui->repo_treeView->model());
    if (_model){
      _model->entrySelected(in);
      return;
    }
    //   

  }


} // namespace API
} // namespace Mantid
