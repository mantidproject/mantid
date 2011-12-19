//----------------------
// Includes
//----------------------

#include "MantidQtCustomInterfaces/CreateMDWorkspace.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceInADS.h"
#include "MantidQtCustomInterfaces/WorkspaceOnDisk.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"


// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
#endif

#include <QtCheckBoxFactory>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <QMessageBox>
#include <QFileDialog>
#include <strstream>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>


namespace MantidQt
{
namespace CustomInterfaces
{

//Add this class to the list of specialised dialogs in this namespace
//DECLARE_SUBWINDOW(CreateMDWorkspace); //TODO: Enable this to use it via mantid plot. Not ready for this yet!

  /**
  Helper type to perform comparisons between WorkspaceMementos
  */
  class IdComparitor : public std::unary_function<WorkspaceMemento_sptr,bool>
  {
  private:
    WorkspaceMemento_sptr m_benchmark;
  public:
    IdComparitor(WorkspaceMemento_sptr benchmark) : m_benchmark(benchmark){}
    bool operator()(WorkspaceMemento_sptr a) const
    {
      std::vector<std::string> strs;
      std::string id =  m_benchmark->getId();
      boost::split(strs, id, boost::is_any_of("/"));

      std::stringstream streamPattern;
      streamPattern << "(" << strs.back() << ")$";
      boost::regex pattern(streamPattern.str(), boost::regex_constants::icase); 

      return boost::regex_search(a->getId(), pattern);

    }
  };

/*
Constructor taking a WorkspaceMementoCollection, which acts as the model.
*/
CreateMDWorkspace::CreateMDWorkspace(QWidget *) 
{
  //Generate memento view model.
  m_model = new QtWorkspaceMementoModel(m_data);
}

/*
Initalize the layout.
*/
void CreateMDWorkspace::initLayout()
{
  m_uiForm.setupUi(this);
  connect(m_uiForm.btn_create, SIGNAL(clicked()), this, SLOT(createMDWorkspaceClicked()));
  connect(m_uiForm.btn_add_workspace, SIGNAL(clicked()), this, SLOT(addWorkspaceClicked()));
  connect(m_uiForm.btn_add_file, SIGNAL(clicked()), this, SLOT(addFileClicked()));
  connect(m_uiForm.btn_remove_workspace, SIGNAL(clicked()), this, SLOT(removeSelectedClicked()));
  //Set MVC Model
  m_uiForm.tableView->setModel(m_model);
}

/*
Add a workspace from the ADS
*/
void CreateMDWorkspace::addWorkspaceClicked()
{
  std::string name = m_uiForm.workspaceSelector->currentText();
  if(!name.empty())
  {
    WorkspaceMemento_sptr candidate(new WorkspaceInADS(name));
    addUniqueMemento(candidate);
  }
}

/**
Adds a memento to the existing data, if it's id has not already been used in the data list.
@param candidate : candidate memento to add.
*/
void CreateMDWorkspace::addUniqueMemento(WorkspaceMemento_sptr candiate)
{
  IdComparitor comparitor(candiate);
  WorkspaceMementoCollection::iterator pos = std::find_if(m_data.begin(), m_data.end(), comparitor);
  if(pos == m_data.end())
  {
    m_data.push_back(candiate);
    m_model->update();
  }
  else
  {
    std::string msg = "Already have a workspace by that name loaded. Cannot add another.";
    runConfirmation(msg);
  }
}

/*
Add a raw file from the ADS
*/
void CreateMDWorkspace::addFileClicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
    "/home",
    tr("Raw Files (*.raw)"));

  std::string name = fileName.toStdString();
  if(!name.empty())
  {
    WorkspaceMemento_sptr candidate(new WorkspaceOnDisk(name));
    addUniqueMemento(candidate);
  }
}

/*
Remove any selected workspace mementos
*/
void CreateMDWorkspace::removeSelectedClicked()
{
  QTableView* view = m_uiForm.tableView;
  QModelIndexList indexes = view->selectionModel()->selection().indexes();

  //Copy the original collection
  WorkspaceMementoCollection temp(m_data.size());
  std::copy(m_data.begin(), m_data.end(), temp.begin());

  for (int i = 0; i < indexes.count(); i++)
  {
    QModelIndex index = indexes.at(i);
    int row = index.row();
    //Copy and swap trick.
    WorkspaceMemento_sptr dead = m_data[row];
    WorkspaceMementoCollection::iterator pos = std::find(temp.begin(), temp.end(), dead);
    if(pos != temp.end())
    {
      temp.erase(pos);
    }
  }
  //Assign using the copied/trimmed temp collection.
  m_data = temp;
  //Update the model.
  m_model->update();
}

void CreateMDWorkspace::initLocalPython()
{
}

/*
Run a generic confirmation dialog.
@param message : The message to display.
*/
int CreateMDWorkspace::runConfirmation(const std::string& message)
{
  QMessageBox msgBox;
  msgBox.setText(message.c_str());
  msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
  msgBox.setDefaultButton(QMessageBox::Cancel);
  return msgBox.exec();
}


/// Destructor
CreateMDWorkspace::~CreateMDWorkspace()
{
}

} //namespace CustomInterfaces
} //namespace MantidQt
