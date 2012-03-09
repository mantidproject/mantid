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
//    // Create a m_scroll area for the (rare) occasion when an algorithm has
//    // so many properties it won't fit on the screen
//    Qm_scrollArea *m_scroll = new Qm_scrollArea(this);
//
//    QWidget *viewport = new QWidget(this);
//    // Put everything in a vertical box and put it inside the m_scroll area
//    QVBoxLayout *mainLay = new QVBoxLayout();
//    viewport->setLayout(mainLay);
//
//    // Add a layout for QDialog
//    QVBoxLayout *dialog_layout = new QVBoxLayout();
//    setLayout(dialog_layout);
//    m_inputGrid = new QGridLayout;
//
//    // -------------- Layout the grid -------------------
//    this->initLayout();
//
//    //The property boxes
//    mainLay->addLayout(m_inputGrid);
//
//    // Add a stretchy item to allow the properties grid to be top-aligned
//    mainLay->addStretch(1);
//
//    dialog_layout->addWidget(m_scroll); // add m_scroll to the QDialog's layout
//
//    m_scroll->setHorizontalm_scrollBarPolicy(Qt::m_scrollBarAlwaysOff);
//    m_scroll->setVerticalm_scrollBarPolicy(Qt::m_scrollBarAsNeeded);
//    m_scroll->setWidget(viewport);
//    m_scroll->setWidgetResizable(true);
//    m_scroll->setAlignment(Qt::AlignLeft & Qt::AlignTop);
//
//    // At this point, all the widgets have been added and are visible.
//    // This makes sure the viewport does not get scaled smaller, even if some controls are hidden.
//    viewport->setMinimumHeight( viewport->height() + 10 );
//
//    const int screenHeight = QApplication::desktop()->height();
//    const int dialogHeight = viewport->height();
//    // If the thing won't end up too big compared to the screen height,
//    // resize the m_scroll area so we don't get a m_scroll bar
//    if ( (dialogHeight+100) < 0.8*screenHeight )
//    {
//      m_scroll->setFixedHeight(viewport->height()+10);
//    }
//
//    dialog_layout->setSizeConstraint(QLayout::SetMinimumSize);

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



  //----------------------------------
  // Protected member functions
  //----------------------------------
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

        m_propWidgets.push_back(widget);

        //TODO: Connect and TIE the widget (this is in GenericDialog.cpp)

        // Only show the "Replace Workspace" button if the algorithm has an input workspace.
        if (hasInputWS)
          widget->addReplaceWSButton();

        ++row;
      } //(end for each property)

    } // (there are properties)
  }


} // namespace Mantid
} // namespace API
