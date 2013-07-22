#include "MantidQtCustomDialogs/SmoothNeighboursDialog.h"

using Mantid::Geometry::Instrument;
using Mantid::API::MatrixWorkspace_sptr;

//Register the class with the factory
DECLARE_DIALOG(SmoothNeighboursDialog)

// As defined in algorithm. Make sure you change them in SmoothNeighbours.cpp as well.
const QString SmoothNeighboursDialog::NON_UNIFORM_GROUP = "NonUniform Detectors";
const QString SmoothNeighboursDialog::RECTANGULAR_GROUP = "Rectangular Detectors";
const QString SmoothNeighboursDialog::INPUT_WORKSPACE = "InputWorkspace";

SmoothNeighboursDialog::SmoothNeighboursDialog(QWidget* parent) 
  : AlgorithmDialog(parent)
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

  // Workspace should have been set by PropertyWidget before emitting valueChanged
  MatrixWorkspace_sptr workspace = this->getAlgorithm()->getProperty(INPUT_WORKSPACE.toStdString());

  // If invalid workspace, do nothing
  if(!workspace)
    return;

  Instrument::ContainsState containsRectDetectors = workspace->getInstrument()->containsRectDetectors();

  QString groupToShow;

  if(containsRectDetectors == Instrument::ContainsState::Full)
    groupToShow = RECTANGULAR_GROUP;
  else
    groupToShow = NON_UNIFORM_GROUP;

  m_propertiesWidget->m_groupWidgets[groupToShow]->setVisible(true);
}

void SmoothNeighboursDialog::accept()
{
  AlgorithmDialog::accept();

  // If got there, there were errors
  for(auto it = m_errors.begin(); it != m_errors.end(); it++)
    m_propertiesWidget->m_propWidgets[it.key()]->setError(it.value());
}
