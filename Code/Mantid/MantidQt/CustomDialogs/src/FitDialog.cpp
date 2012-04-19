//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtCustomDialogs/FitDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
// Qt
#include <QCheckBox>
#include <QComboBox>
#include <QUrl>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileInfo>

// Mantid
#include "MantidKernel/Property.h"
#include "MantidAPI/AlgorithmManager.h"

namespace MantidQt
{
  namespace CustomDialogs
  {
    // Declare the dialog. Name must match the class name
    DECLARE_DIALOG(FitDialog);

    //--------------------------------------------------------------------------
    // Public methods
    //---------------------------------------------------------------------------

    /// Default constructor
    FitDialog:: FitDialog(QWidget *parent) 
      : API::AlgorithmDialog(parent), m_form()
    {
    }

    //--------------------------------------------------------------------------
    // Private methods (slot)
    //---------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    // Private methods (non-slot)
    //---------------------------------------------------------------------------

    /// Initialize the layout
    void FitDialog::initLayout()
    {
      m_form.setupUi(this);
      m_form.dialogLayout->addLayout(this->createDefaultButtonLayout());
      //m_form.fileWidget->readSettings("Mantid/Algorithms/Load");

      tieStaticWidgets(true);
    }

    /**
    * Save the input after OK is clicked
    */
    void FitDialog::saveInput()
    {
      //m_form.fileWidget->saveSettings("Mantid/Algorithms/Load");
      AlgorithmDialog::saveInput();
      //Ensure the filename is store as the full file
      //API::AlgorithmInputHistory::Instance().storeNewValue("Load", 
						//	   QPair<QString, QString>("Filename", m_currentFile));
    }

    void FitDialog::parseInput()
    {
      for(auto it = m_dynamicEditors.begin(); it != m_dynamicEditors.end(); ++it)
      {
        setPropertyValue(it.key(),false);
      }
    }

    /**
    * Tie static widgets to their properties
    * @param readHistory :: If true then the history will be re read.
    */
    void FitDialog::tieStaticWidgets(const bool readHistory)
    {
      //m_staticProperties << "Function" << "InputWorkspace" << "CreateOutput" << "Output"
      //  << "MaxIterations" << "Minimizer" << "CostFunction";
      //tie(m_form.leFunction, "Function", m_form.topLayout, readHistory);
      //connect(m_form.leFunction,SIGNAL(editingFinished()),this,SLOT(functionChanged()));
      //// Check input workspace property. If there are available workspaces then
      //// these have been set as allowed values
      //std::set<std::string> workspaces = getAlgorithmProperty("InputWorkspace")->allowedValues();
      //for( std::set<std::string>::const_iterator itr = workspaces.begin(); itr != workspaces.end(); ++itr )
      //{
      //  m_form.cbInputWorkspace->addItem(QString::fromStdString(*itr));
      //}
      //connect(m_form.cbInputWorkspace,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(workspaceChanged(const QString&)));
      //tie(m_form.cbInputWorkspace, "InputWorkspace", m_form.topLayout, readHistory);

      //tie(m_form.chbCreateOutput, "CreateOutput", m_form.bottomLayout, readHistory);
      //tie(m_form.leOutput, "Output", m_form.bottomLayout, readHistory);
      //tie(m_form.leMaxIterations, "MaxIterations", m_form.bottomLayout, readHistory);

      //auto values = getAlgorithmProperty("Minimizer")->allowedValues();
      //for( auto itr = values.begin(); itr != values.end(); ++itr )
      //{
      //  m_form.cbMinimizer->addItem(QString::fromStdString(*itr));
      //}
      //tie(m_form.cbMinimizer, "Minimizer", m_form.bottomLayout, readHistory);

      //values = getAlgorithmProperty("CostFunction")->allowedValues();
      //for( auto itr = values.begin(); itr != values.end(); ++itr )
      //{
      //  m_form.cbCostFunction->addItem(QString::fromStdString(*itr));
      //}
      //tie(m_form.cbCostFunction, "CostFunction", m_form.bottomLayout, readHistory);
      //
      //if (!m_form.leFunction->text().isEmpty())
      //{
      //  getAlgorithm()->setPropertyValue("Function",m_form.leFunction->text().toStdString());
      //  getAlgorithm()->setPropertyValue("InputWorkspace",m_form.cbInputWorkspace->currentText().toStdString());
      //  createDynamicLayout();
      //}
    }

    /**
    * Clear the old widgets for a new Loader type
    * @param layout :: The layout containing the child layouts/widgets
    */
    void FitDialog::removeOldInputWidgets()
    {
      //auto layout = m_form.topLayout;
      //layout->setEnabled(false);
      //// Remove the old widgets if necessary
      //if( layout->count() > 4 )
      //{
      //  int count = layout->count();
      //  while( count > 4 )
      //  {
      //    QLayoutItem *child = layout->takeAt(count - 1);
      //    if( QWidget *w = child->widget() )
      //    {
      //      w->deleteLater();
      //    }
      //    else if( QLayout *l = child->layout() )
      //    {
      //      QLayoutItem *subChild(NULL);
      //      while( (subChild = l->takeAt(0)) != NULL )
      //      {
      //        subChild->widget()->deleteLater();
      //      }
      //    }
      //    count = layout->count();
      //  }
      //}
      //layout->setEnabled(true);
      //m_dynamicLabels.clear();
      //m_dynamicEditors.clear();
    }

    /**
    * Create the dynamic widgets for the concrete loader
    */
    void FitDialog::createDynamicLayout()
    {
      //int index = m_form.topLayout->rowCount();
      //auto properties = getAlgorithm()->getProperties();
      //for(auto prop = properties.begin(); prop != properties.end(); ++prop)
      //{
      //  QString propName = QString::fromStdString((**prop).name());
      //  if ( !m_staticProperties.contains(propName) && 
      //    !m_dynamicLabels.contains(propName) && 
      //    (**prop).direction() == Mantid::Kernel::Direction::Input)
      //  {
      //    untie(propName);
      //    QLabel *label = new QLabel(propName,this);
      //    QLineEdit *edit = new QLineEdit(this);
      //    m_form.topLayout->addWidget(label,index,0);
      //    m_form.topLayout->addWidget(edit,index,1);
      //    tie(edit, propName, m_form.topLayout, true);
      //    m_dynamicLabels.insert(propName,label);
      //    m_dynamicEditors.insert(propName,edit);
      //    ++index;
      //  }
      //}
      //m_form.mainLayout->invalidate();
    }

    void FitDialog::workspaceChanged(const QString&)
    {
      this->setPropertyValues();
      removeOldInputWidgets();
      createDynamicLayout();
    }

    void FitDialog::functionChanged()
    {
      //this->setPropertyValues();
      //removeOldInputWidgets();
      //createDynamicLayout();
    }


  } // CustomDialogs
} // MantidQt
