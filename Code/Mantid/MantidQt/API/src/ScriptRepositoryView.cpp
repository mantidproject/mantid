#include "MantidQtAPI/ScriptRepositoryView.h"
#include "MantidQtAPI/RepoModel.h"
#include  <QSortFilterProxyModel>
#include <QDebug>
#include "MantidAPI/ScriptRepository.h"
#include "MantidAPI/ScriptRepositoryFactory.h"
#include <QtConcurrentRun>
#include <QMessageBox>
#include <QTime>
#include <QCoreApplication>
#include <QLabel>
#include <QVBoxLayout>
namespace MantidQt
{
namespace API
{
  /** Allow the application to be alive while giving some time to this Widget to ensure that
  the installation process is going on well.*/
  void delay()
{
    QTime dieTime= QTime::currentTime().addSecs(1);
    while( QTime::currentTime() < dieTime )
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);    
}

  /**
    TODO: it is necessary to make a better approach when a installation is required. 
    
    This is a temporary solution in order to allow the installation of the script repository at first run. 

  */
   int install_repository(){
    using Mantid::API::ScriptRepositoryFactory; 

    Mantid::API::ScriptRepository_sptr repo_ptr =  ScriptRepositoryFactory::Instance().create("GitScriptRepository");
    try{
      repo_ptr->update();
      QMessageBox::information(NULL, "Install Script Repository", "Script Repository Installed!\n"); 
    }catch(Mantid::API::ScriptRepoException & ex){
       qWarning() << "Update exception: " << ex.what() << endl;        
       return -1; 
    }
    return 0;
  }

   /* Allow to update the script repositoy in background operation when the user tries to open the 
    ScriptRepository.
   */ 
   int update_repository(){
    using Mantid::API::ScriptRepositoryFactory; 

    Mantid::API::ScriptRepository_sptr repo_ptr =  ScriptRepositoryFactory::Instance().create("GitScriptRepository");
    try{
      repo_ptr->update();      
    }catch(Mantid::API::ScriptRepoException & ex){
       qWarning() << "Update exception: " << ex.what() << endl;        
       return -1; 
    }
    return 0;
  }


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ScriptRepositoryView::ScriptRepositoryView(QWidget * parent):
    QDialog(parent),
    ui(new Ui::ScriptRepositoryView)
  {
    using Mantid::API::ScriptRepositoryFactory; 
    Mantid::API::ScriptRepository_sptr repo_ptr =   ScriptRepositoryFactory::Instance().create("GitScriptRepository");
 
    if (!repo_ptr->isValid()){
      // no repository cloned
      if (QMessageBox::Ok != QMessageBox::question(this,"Install Script Repository?",
          "The Script Repository allow you to share your scripts and to get mantid scripts from the developers and the community.\n"
          "The installation may require a couple of minutes.\nWould you like to install it now?"
          "\n\nMore Information: http://www.mantidproject.org/ScriptRepository",
          QMessageBox::Ok|QMessageBox::Cancel)){
            // user does not whant to install
            close();
            deleteLater();
            return;
      }
      // installing in a new thread
      QFuture<int> install = QtConcurrent::run(install_repository);
      // give some time to the thread to start
      delay();
      if (install.isResultReadyAt(0)){
        // this means that the installation failed (no connection, probably)
        QMessageBox::information(this,"Installa Script Repository Failed!",
            "The installation failed. Please check your internet connection and try again\n"); 
        close(); 
      }

      if (QMessageBox::Yes == QMessageBox::question(this, "Install Script Repository",
          "Installing Script Repository. It may take a couple of minutes...\n"
          "Do you want to do it in background?\n"
          "You will have to open the interface again after some minutes.\n",
          QMessageBox::Yes|QMessageBox::No)){

          QLabel * inf = new QLabel("Running Script Repository Installation in background!\n",this); 
          QVBoxLayout *layout = new QVBoxLayout;
          layout->addWidget(inf);
          this->setLayout(layout);
          return;
        };
      install.resultAt(0);       
    }
    
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
    QFuture<int> update = QtConcurrent::run(update_repository);
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
