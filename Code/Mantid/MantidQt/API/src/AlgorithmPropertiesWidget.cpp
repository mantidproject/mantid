#include "MantidKernel/Property.h"
#include "MantidKernel/System.h"
#include "MantidQtAPI/AlgorithmPropertiesWidget.h"
#include <QtGui>
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtAPI/PropertyWidgetFactory.h"
#include "MantidQtAPI/PropertyWidget.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmProxy.h"

using namespace Mantid::Kernel;
using Mantid::API::IWorkspaceProperty;
using Mantid::API::AlgorithmManager;
using Mantid::API::FrameworkManager;
using Mantid::API::Algorithm_sptr;
using Mantid::API::AlgorithmProxy;

namespace MantidQt
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  AlgorithmPropertiesWidget::AlgorithmPropertiesWidget(QWidget * parent)
  : QWidget(parent),
    m_algoName(""),
    m_algo(NULL), m_deleteAlgo(false)
  {
    // Create the grid layout that will have all the widgets
    m_inputGrid = new QGridLayout;

    // Create the viewport that holds only the grid layout
    m_viewport = new QWidget(this);

    // Put everything in a vertical box and put it inside the m_scroll area
    QVBoxLayout *mainLay = new QVBoxLayout();
    m_viewport->setLayout(mainLay);
    //The property boxes
    mainLay->addLayout(m_inputGrid);
    // Add a stretchy item to allow the properties grid to be top-aligned
    mainLay->addStretch(1);


    // Create a m_scroll area for the (rare) occasion when an algorithm has
    // so many properties it won't fit on the screen
    m_scroll = new QScrollArea(this);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->setWidget(m_viewport);
    m_scroll->setWidgetResizable(true);
    m_scroll->setAlignment(Qt::AlignLeft & Qt::AlignTop);


    // Add a layout for the whole widget, containing just the m_scroll area
    QVBoxLayout *dialog_layout = new QVBoxLayout();
    dialog_layout->addWidget(m_scroll);
    setLayout(dialog_layout);

    this->initLayout();
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  AlgorithmPropertiesWidget::~AlgorithmPropertiesWidget()
  {
    if (m_deleteAlgo) delete m_algo;
  }
  

  //----------------------------------------------------------------------------------------------
  ///@return the algorithm being viewed
  Mantid::API::IAlgorithm * AlgorithmPropertiesWidget::getAlgorithm()
  {
    return m_algo;
  }

  //----------------------------------------------------------------------------------------------
  /** Directly set the algorithm to view. Sets the name to match
   *
   * @param algo :: IAlgorithm bare ptr */
  void AlgorithmPropertiesWidget::setAlgorithm(Mantid::API::IAlgorithm * algo)
  {
    if (!algo) return;
    if (m_deleteAlgo) delete m_algo;
    m_algo = algo;
    m_algoName = QString::fromStdString(m_algo->name());
    this->initLayout();
    // Caller should replace this value as needed
    m_deleteAlgo = false;
  }

  //----------------------------------------------------------------------------------------------
  ///@return the name of the algorithm being displayed
  QString AlgorithmPropertiesWidget::getAlgorithmName() const
  {
    return m_algoName;
  }

  /** Set the algorithm to view using its name
   *
   * @param algo :: IAlgorithm bare ptr */
  void AlgorithmPropertiesWidget::setAlgorithmName(QString name)
  {
    FrameworkManager::Instance();
    m_algoName = name;
    try
    {
      Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged(m_algoName.toStdString());
      AlgorithmProxy * algProxy = new AlgorithmProxy(alg);
      algProxy->initialize();

      // Set the algorithm ptr. This will redo the layout
      this->setAlgorithm(algProxy);

      // Take ownership of the pointer
      m_deleteAlgo = true;
    }
    catch (std::runtime_error & )
    {
    }
  }



  //---------------------------------------------------------------------------------------------------------------
  bool haveInputWS(const std::vector<Property*> & prop_list)
  {
    // For the few algorithms (mainly loading) that do not have input workspaces, we do not
    // want to render the 'replace input workspace button'. Do a quick scan to check.
    // Also the ones that don't have a set of allowed values as input workspace
    std::vector<Property*>::const_iterator pEnd = prop_list.end();
    for(std::vector<Property*>::const_iterator pIter = prop_list.begin();
      pIter != pEnd; ++pIter)
    {
      Property *prop = *pIter;
      if( prop->direction() == Direction::Input && dynamic_cast<IWorkspaceProperty*>(prop) )
      {
        return true;
      }
    }
    return false;
  }



  //---------------------------------------------------------------------------------------------------------------
  /**
  * Create the layout for this dialog.
  */
  void AlgorithmPropertiesWidget::initLayout()
  {
    if (!getAlgorithm())
      return;

    // Delete all widgets in the layout
    QLayoutItem *child;
    while ((child = m_inputGrid->takeAt(0)) != 0) {
      delete child->widget();
      delete child;
    }

    // This also deletes the PropertyWidget, which does not actually
    // contain the sub-widgets because they are shared in the grid layout
    for (auto it = m_propWidgets.begin(); it != m_propWidgets.end(); it++)
        (*it)->deleteLater();
    QCoreApplication::processEvents();
    m_propWidgets.clear();

    // Create a grid of properties if there are any available
    const std::vector<Property*> & prop_list = getAlgorithm()->getProperties();
    bool hasInputWS = haveInputWS(prop_list);

    if ( !prop_list.empty() )
    {
      //Put the property boxes in a grid
      m_currentGrid = m_inputGrid;

      std::string group = "";

      //Each property is on its own row
      int row = 0;

      for(std::vector<Property*>::const_iterator pIter = prop_list.begin();
        pIter != prop_list.end(); ++pIter)
      {
        Property* prop = *pIter;
        QString propName = QString::fromStdString(prop->name());

        // Are we entering a new group?
        if (prop->getGroup() != group)
        {
          group = prop->getGroup();

          if (group == "")
          {
            // Return to the original grid
            m_currentGrid = m_inputGrid;
          }
          else
          {
            // Make a groupbox with a border and a light background
            QGroupBox * grpBox = new QGroupBox(QString::fromStdString(group) );
            grpBox->setAutoFillBackground(true);
            grpBox->setStyleSheet(
                "QGroupBox { border: 1px solid gray;  border-radius: 4px; font-weight: bold; margin-top: 4px; margin-bottom: 4px; padding-top: 16px; }"
                "QGroupBox::title { background-color: transparent;  subcontrol-position: top center;  padding-top:4px; padding-bottom:4px; } ");
            QPalette pal = grpBox->palette();
            pal.setColor(grpBox->backgroundRole(), pal.alternateBase().color());
            grpBox->setPalette(pal);

            // Put the frame in the main grid
            m_inputGrid->addWidget(grpBox, row, 0, 1, 4);

            // Make a layout in the grp box
            m_currentGrid = new QGridLayout;
            grpBox->setLayout(m_currentGrid);
            row++;
          }
        }

        // Only accept input for output properties or workspace properties
        bool isWorkspaceProp(dynamic_cast<IWorkspaceProperty*>(prop));
        if( prop->direction() == Direction::Output && !isWorkspaceProp )
          continue;

        // Create the appropriate widget at this row in the grid.
        PropertyWidget * widget = PropertyWidgetFactory::createWidget(prop, this, m_currentGrid, row);

        m_propWidgets[propName] = widget;

        // Whenever the value changes in the widget, this fires propertyChanged()
        connect(widget, SIGNAL( valueChanged(const QString &)), this, SLOT(propertyChanged(const QString &)));

        // For clicking the "Replace Workspace" button (if any)
        connect(widget, SIGNAL( replaceWorkspaceName(const QString &)), this, SLOT(replaceWSClicked(const QString &)));

        // Only show the "Replace Workspace" button if the algorithm has an input workspace.
        if (hasInputWS)
          widget->addReplaceWSButton();

        ++row;
      } //(end for each property)

    } // (there are properties)
  }



  //--------------------------------------------------------------------------------------
  /** SLOT to be called whenever a property's value has just been changed
   * and the widget has lost focus/value has been changed.
   * @param pName :: name of the property that was changed
   */
  void AlgorithmPropertiesWidget::propertyChanged(const QString & pName)
  {
    //PropertyWidget * widget = m_propWidgets[pName];
    this->hideOrDisableProperties();
  }


  //-------------------------------------------------------------------------------------------------
  /** A slot to handle the replace workspace button click
   *
   * @param propName :: the property for which we clicked "Replace Workspace"
   */
  void AlgorithmPropertiesWidget::replaceWSClicked(const QString & propName)
  {
    if (m_propWidgets.contains(propName))
    {
      PropertyWidget * propWidget = m_propWidgets[propName];
      if (propWidget)
      {
        // Find the name to put in the spot
        QString wsName("");
        for (auto it = m_propWidgets.begin(); it != m_propWidgets.end(); it++)
        {
          // Only look at workspace properties
          PropertyWidget* otherWidget = it.value();
          Property * prop = it.value()->getProperty();
          IWorkspaceProperty * wsProp = dynamic_cast<IWorkspaceProperty*>(prop);
          if (otherWidget && wsProp)
          {
            if (prop->direction() == Direction::Input)
            {
              // Input workspace property. Get the text typed in.
              wsName = otherWidget->getValue();
              break;
            }
          }
        }

        if (!wsName.isEmpty())
          propWidget->setValue(wsName);
      }
    }

  }


  //-------------------------------------------------------------------------------------------------
  /** Go through all the properties, and check their validators to determine
   * whether they should be made disabled/invisible.
   * It also shows/hids the validators.
   * All properties' values should be set already, otherwise the validators
   * will be running on old data.
   */
  void AlgorithmPropertiesWidget::hideOrDisableProperties()
  {
    for( auto pitr = m_propWidgets.begin(); pitr != m_propWidgets.end(); ++pitr )
    {
      PropertyWidget * widget = pitr.value();
      Mantid::Kernel::Property *prop = widget->getProperty();
      QString propName = pitr.key();

      // Set the enabled and visible flags based on what the validators say. Default is always true.

      // TODO: Also use a list of enabled/disabled from AlgorithmDialog
      bool enabled = prop->isEnabled();
      bool visible = prop->isVisible();

      // Dynamic PropertySettings objects allow a property to change validators.
      // This removes the old widget and creates a new one instead.
      if (prop->isConditionChanged())
      {
        prop->getSettings()->applyChanges(prop);

        // Delete the old widget
        int row = widget->getGridRow();
        QGridLayout * layout = widget->getGridLayout();
        widget->deleteLater();

        // Create the appropriate widget at this row in the grid.
        widget = PropertyWidgetFactory::createWidget(prop, this, layout, row);

//        // Record in the list of tied widgets (used in the base AlgorithmDialog)
//        tie(widget, propName, layout);
//
//        // Whenever the value changes in the widget, this fires propertyChanged()
//        connect(widget, SIGNAL( valueChanged(const QString &)), this, SLOT(propertyChanged(const QString &)));
      }

      // Show/hide the validator label (that red star)
      QString error = "";
      if (m_errors.contains(propName)) error = m_errors[propName];
      // Always show controls that are in error
      if (error.length() != 0)
        visible = true;

      // Hide/disable the widget
      widget->setEnabled( enabled );
      widget->setVisible( visible );

//      QLabel *validator = getValidatorMarker(propName);
//      // If there's no validator then assume it's handling its own validation notification
//      if( validator )
//      {
//        validator->setToolTip( error );
//        validator->setVisible( error.length() != 0);
//      }
    } // for each property

    this->repaint(true);
  }


} // namespace Mantid
} // namespace API
