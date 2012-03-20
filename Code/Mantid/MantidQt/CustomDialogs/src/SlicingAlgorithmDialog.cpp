#include "MantidQtCustomDialogs/SlicingAlgorithmDialog.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::API;

namespace MantidQt
{
  namespace CustomDialogs
  {
    DECLARE_DIALOG(SliceMDDialog);
    DECLARE_DIALOG(BinMDDialog);

    SlicingAlgorithmDialog::SlicingAlgorithmDialog(QWidget* parent) : AlgorithmDialog(parent)
    {
    }

    /// Set up the dialog layout
    void SlicingAlgorithmDialog::initLayout()
    {
      ui.setupUi(this);
      this->setWindowTitle(m_algName);

      connect(ui.workspace_selector,SIGNAL(activated(int)),this ,SLOT(onWorkspaceChanged()));
      connect(ui.controls, SIGNAL(accepted()), this, SLOT(accept()));
      connect(ui.controls, SIGNAL(rejected()), this, SLOT(reject()));

      ui.workspace_selector->setValidatingAlgorithm(m_algName);
      connect(ui.workspace_selector, SIGNAL(clicked()), this, SLOT(createMDWorkspaceClicked()));
      ui.workspace_selector->clear();
      typedef std::set<std::string> WorkspaceNames;
      WorkspaceNames names = AnalysisDataService::Instance().getObjectNames();
      WorkspaceNames::iterator it = names.begin();
      while(it != names.end())
      {
        IMDEventWorkspace_sptr ws = boost::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve(*it));
        if(ws)
        {
          ui.workspace_selector->addItem((*it).c_str());
        }
        ++it;
      }

      buildFromCurrentInput();

    }

    SlicingAlgorithmDialog::~SlicingAlgorithmDialog()
    {
    }

    QString formattedDimensionInput(Mantid::Geometry::IMDDimension_const_sptr dim)
    {
      QString min, max, nbins, result;
      QString name(dim->getName().c_str());
      min.setNum(dim->getMinimum());
      max.setNum(dim->getMaximum());
      nbins.setNum(dim->getNBins());
      result.append(name).append(",").append(min).append(",").append(max).append(",").append(nbins);
      return result;
    }

    void SlicingAlgorithmDialog::clearExistingDimensions()
    {
      QLayout* layout = this->ui.axis_aligned_layout->layout();
      for(int i = 0; i < layout->chil

      QLayoutItem* pLayoutItem = layout()->itemAt(size - 1);
    QWidget* pWidget = pLayoutItem->widget();
    if (NULL == pWidget)
    {
      throw std::domain_error(
        "Error ::popWidget(). Attempting to pop a non-widget object off the layout!");
    }
    else
    {
      pWidget->setHidden(true);
      this->layout()->removeItem(pLayoutItem);
    }

    void SlicingAlgorithmDialog::buildFromCurrentInput()
    {
      const QString& txt = ui.workspace_selector->currentText();
      if(!txt.isEmpty())
      {
        IMDWorkspace_sptr ws = boost::dynamic_pointer_cast<IMDWorkspace>(AnalysisDataService::Instance().retrieve(txt.toStdString()));
        size_t nDimensions = ws->getNumDims();
        for(size_t index = 0; index < nDimensions; ++index)
        {
          Mantid::Geometry::IMDDimension_const_sptr dim = ws->getDimension(index);
          QString dimensioninfo = formattedDimensionInput(dim);

          QHBoxLayout* layout = new QHBoxLayout;
          QWidget* w = new QWidget;
          w->setLayout(layout);

          QLabel* dimensionLabel = new QLabel("Dimension x..");

          QLineEdit* txtDimension = new QLineEdit(dimensioninfo);
          layout->addWidget(dimensionLabel);
          layout->addWidget(txtDimension);

          this->ui.axis_aligned_layout->layout()->addWidget(w);
        }
      }
    }

    void SlicingAlgorithmDialog::onWorkspaceChanged()
    {
      buildFromCurrentInput();
    }

    


  }
}