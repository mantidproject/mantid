#include "MantidQtAPI/ScriptRepositoryView.h"
#include "MantidQtAPI/RepoModel.h"
#include  <QSortFilterProxyModel>
#include <QDebug>
#include "MantidAPI/ScriptRepository.h"
#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidKernel/ConfigService.h"
#include <QtConcurrentRun>
#include <QMessageBox>
#include <QTime>
#include <QCoreApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
namespace MantidQt
{
namespace API
{


  const QString install_mantid_label = "<html><head/><body><p>New in this release, the <span style=\" font-weight:600;\">"
    "Script Repository</span> allows you to:</p>"
    "<p>  * Share your scripts and reduction algorithms;</p>"
    "<p>  * Get <span style=\" font-weight:600;\">Mantid</span> Scripts from the mantid developers and the community. </p>"
    "<p><span style=\" font-style:italic;\">"
    "N.B. The installation usually requires a couple of minutes, depending on your network bandwidth. </span></p>"
    "<p>More Information available at "
    "<a href=\"http://www.mantidproject.org/ScriptRepository\"><span style=\" text-decoration: underline; color:#0000ff;\">"
    "http://www.mantidproject.org/ScriptRepository</span></a></p></br><p><span style=\" font-weight:600;\">"
    "Would you like to install it now?</span></p></body></html>";

  const QString installation_in_progress = "<html><head/><body><p><span style=\" font-weight:600;\">"
    "Installing Script Repository Installation in background!</span></p>"
    "<p>You may continue to use mantid.</p>"
    "<p>The Result Log willl give you information of the installation progress.</p>"
    "<p>When finished, please, reopen the <span style=\" font-weight:600;\">Script Repository</span>. </p></body></html>";

  const QString installation_failed = "<html><head/><body><p>The installation of Script Repository "
    "<span style=\" font-weight:600;\">Failed</span>!</p>"
    "<p>Please, check the Result Log to see why the installation failed. </p></body></html>";

  /** Allow the application to be alive while giving some time to this Widget to ensure that
  the installation process is going on well.*/
  void delay()
{
    QTime dieTime= QTime::currentTime().addSecs(3);
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
     // QMessageBox::information(NULL, "Install Script Repository", "Script Repository Installed!\n"); 
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
       qWarning() << "Update of Script Repository failure: " << ex.what() << endl;        
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
    using Mantid::Kernel::ConfigServiceImpl; 
    using Mantid::Kernel::ConfigService;
    Mantid::API::ScriptRepository_sptr repo_ptr =   ScriptRepositoryFactory::Instance().create("GitScriptRepository");
 
    if (!repo_ptr->isValid()){
      // no repository cloned
      if (QMessageBox::Ok != QMessageBox::question(this,"Install Script Repository?",
                                                   install_mantid_label,
          QMessageBox::Ok|QMessageBox::Cancel)){
            // user does not whant to install
            close();
            deleteLater();
            return;
      }
      ConfigServiceImpl & config = ConfigService::Instance();
      QString loc = QString::fromStdString(config.getString("ScriptLocalRepository")); 
       QString dir = QFileDialog::getExistingDirectory(this, tr("Where do you want to install Script Repository?"),
                                                 loc,
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

      //configuring
      if (dir.isEmpty())
      {
        QMessageBox::warning(this, "Installation Failed",
            "Invalid Folder to install Script Repository!\n"); 
        close();
        deleteLater(); 
        return;      
      }
      QString local_path = dir + "/mantidscripts"; 
      qDebug() << "setting scriptlocalrepository to " << local_path << "\n"; 
      config.setString("ScriptLocalRepository", local_path.toStdString()); 
      config.saveConfig(config.getUserFilename()); 

      // installing in a new thread
      QFuture<int> install = QtConcurrent::run(install_repository);
      delay();
      if (install.isResultReadyAt(0)){
        if (install.resultAt(0) < 0){
          QMessageBox::warning(this, "Failure", installation_failed);
          close(); 
          deleteLater(); 
          return;
        }
      }else{
        // give some time to the thread to start
        QLabel * inf = new QLabel(installation_in_progress,this); 
        QPushButton * close = new QPushButton("Close"); 
        connect(close, SIGNAL(clicked()), this, SLOT(close())); 
        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(inf);
        layout->addWidget(close);
        this->setLayout(layout);
        return;       
      }
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
