#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtCustomDialogs/SmoothNeighboursDialog.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;

//Register the class with the factory
DECLARE_DIALOG(SmoothNeighboursDialog)

// As defined in algorithm. Make sure you change them in SmoothNeighbours.cpp as well.
const QString SmoothNeighboursDialog::NON_UNIFORM_GROUP = "NonUniform Detectors";
const QString SmoothNeighboursDialog::RECTANGULAR_GROUP = "Rectangular Detectors";
const QString SmoothNeighboursDialog::INPUT_WORKSPACE = "InputWorkspace";

SmoothNeighboursDialog::SmoothNeighboursDialog(QWidget* parent) 
  : AlgorithmDialog(parent),
  m_propertiesWidget(NULL), m_dialogLayout(NULL)
{
}
 
void SmoothNeighboursDialog::initLayout()
{
  // Create main layout
  m_dialogLayout = new QVBoxLayout();

  this->setLayout(m_dialogLayout);

  // Set to size which fits all the possible widget changes
  this->resize(475, 545);

  // Create yellow information box
  this->addOptionalMessage(m_dialogLayout);

  m_propertiesWidget = new AlgorithmPropertiesWidget(this);

  m_propertiesWidget->setAlgorithm(this->getAlgorithm());

  // Mark the properties that will be forced enabled or disabled
  m_propertiesWidget->addEnabledAndDisableLists(m_enabled, m_disabled + m_python_arguments);

  // Tie all the widgets to properties
  for (auto it = m_propertiesWidget->m_propWidgets.begin(); it != m_propertiesWidget->m_propWidgets.end(); it++)
    this->tie(it.value(), it.key());

  m_propertiesWidget->hideOrDisableProperties();

  PropertyWidget* inputWorkspaceWidget = m_propertiesWidget->m_propWidgets[INPUT_WORKSPACE];

  connect(inputWorkspaceWidget, SIGNAL(valueChanged(const QString&)),
          this, SLOT(inputWorkspaceChanged(const QString&)));

  m_dialogLayout->addWidget(m_propertiesWidget);

  // Create and add the OK/Cancel/Help. buttons
  m_dialogLayout->addLayout(this->createDefaultButtonLayout());

  // Explicitly call, to hide/show property group from the beginning
  inputWorkspaceWidget->valueChangedSlot();
}

void SmoothNeighboursDialog::inputWorkspaceChanged(const QString& pName)
{
  UNUSED_ARG(pName);

  m_propertiesWidget->m_groupWidgets[RECTANGULAR_GROUP]->setVisible(false);
  m_propertiesWidget->m_groupWidgets[NON_UNIFORM_GROUP]->setVisible(false);

  std::string inWsName = INPUT_WORKSPACE.toStdString();

  // Workspace should have been set by PropertyWidget before emitting valueChanged
  MatrixWorkspace_sptr inWs = this->getAlgorithm()->getProperty(inWsName);

  if(!inWs)
  {
    // Workspace groups are NOT returned by IWP->getWorkspace(), as they are not MatrixWorkspace,
    // so check the ADS for the GroupWorkspace with the same name
    std::string inWsValue = this->getAlgorithm()->getPointerToProperty(inWsName)->value();

    // If it really doesn't exist, don't do anything
    if(!AnalysisDataService::Instance().doesExist(inWsValue))
      return;

    WorkspaceGroup_sptr inGroupWs = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(inWsValue);

    if(inGroupWs)
      // If is a group workspace, use the first workspace to determine the instrument type,
      // as most of the times it will be the same for all the workspaces
      inWs = boost::dynamic_pointer_cast<MatrixWorkspace>(inGroupWs->getItem(0));
    else
      // If is not a GroupWorkspace as well, do nothing
      return;
  }
  Instrument::ContainsState containsRectDetectors = inWs->getInstrument()->containsRectDetectors();

  if(containsRectDetectors == Instrument::ContainsState::Full)
    m_propertiesWidget->m_groupWidgets[RECTANGULAR_GROUP]->setVisible(true);
  else
    m_propertiesWidget->m_groupWidgets[NON_UNIFORM_GROUP]->setVisible(true);
}

void SmoothNeighboursDialog::accept()
{
  AlgorithmDialog::accept();

  // If got there, there were errors
  for(auto it = m_errors.begin(); it != m_errors.end(); it++)
    m_propertiesWidget->m_propWidgets[it.key()]->updateIconVisibility(it.value());
}
