#include "MantidQtCustomDialogs/SlicingAlgorithmDialog.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include <QIntValidator>
#include <QFileDialog>
#include <QDir>

using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace MantidQt
{
  namespace CustomDialogs
  {
    DECLARE_DIALOG(SliceMDDialog);
    DECLARE_DIALOG(BinMDDialog);

    /**
    Constructor
    @param parent : parent widget
    */
    SlicingAlgorithmDialog::SlicingAlgorithmDialog(QWidget* parent) : AlgorithmDialog(parent)
    {
    }


    /// Set up the dialog layout
    void SlicingAlgorithmDialog::initLayout()
    {
      ui.setupUi(this);
      this->setWindowTitle(m_algName);

      //Tie core widgets to core properties.
      tie(ui.workspace_selector, "InputWorkspace", ui.input_layout);
      tie(ui.ck_axis_aligned, "AxisAligned");
      tie(ui.txt_output, "OutputWorkspace", ui.output_layout);
      tie(ui.txt_output_extents, "OutputExtents", ui.output_extents_layout);
      tie(ui.txt_output_bins, "OutputBins", ui.output_bins_layout);
      tie(ui.ck_normalisebasisvectors, "NormalizeBasisVectors");
      tie(ui.ck_force_orthogonal, "ForceOrthogonal");
      tie(ui.txt_translation, "Translation");

      ui.txt_memory->setValidator(new QIntValidator(this));
      ui.txt_resursion_depth->setValidator(new QIntValidator(this));

      connect(ui.workspace_selector,SIGNAL(activated(int)),this ,SLOT(onWorkspaceChanged()));
      connect(ui.controls, SIGNAL(accepted()), this, SLOT(accept()));
      connect(ui.controls, SIGNAL(rejected()), this, SLOT(reject()));
      connect(ui.ck_axis_aligned, SIGNAL(clicked(bool)), this, SLOT(onAxisAlignedChanged(bool)));
      connect(ui.ck_max_from_input, SIGNAL(clicked(bool)), this, SLOT(onMaxFromInput(bool)));
      connect(ui.btn_browse, SIGNAL(clicked()), this, SLOT(onBrowse()));
      connect(ui.btn_help, SIGNAL(clicked()), this, SLOT(helpClicked()));
      connect(ui.btn_rebuild_dimensions, SIGNAL(clicked()), this, SLOT(onRebuildDimensions()));

      //Configure workspace selector
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
      QString lastUsedInputWorkspaceName = getHistoricalInputWorkspaceName();
      int index = ui.workspace_selector->findText(lastUsedInputWorkspaceName);
      if( index >= 0 )
      {
        ui.workspace_selector->setCurrentIndex(index);
      }

      //Derived algorithms may use this to apply any additional ties.
      customiseInitLayout();

      //Dynamically create the input dimensions.
      buildDimensionInputs();
    }

    /// Destructor
    SlicingAlgorithmDialog::~SlicingAlgorithmDialog()
    {
    }

    /**
     Create a formatted string for the dimension input based on an existing dimension.
     @param dim : dimension to format to string.
    */
    QString formattedAlignedDimensionInput(Mantid::Geometry::IMDDimension_const_sptr dim)
    {
      QString min, max, nbins, result;
      QString name(dim->getName().c_str());
      min.setNum(dim->getMinimum());
      max.setNum(dim->getMaximum());
      nbins.setNum(dim->getNBins());
      result.append(name).append(",").append(min).append(",").append(max).append(",").append(nbins);
      return result;
    }

    /**
     Create a formatted string for the dimension input based on an existing dimension.
     @param dim : dimension to format to string.
    */
    QString formatNonAlignedDimensionInput(Mantid::Geometry::IMDDimension_const_sptr)
    {
      //Deliberately return an empty string here, because it's not obveious how the basis vectors could be automatically formed.
      return QString("");
    }
    
    /**
    Clears the layout of any qwidget
    @param layout : layout to clean
    */
    void SlicingAlgorithmDialog::cleanLayoutOfDimensions(QLayout* layout)
    {
      int itemCount = layout->count();
      for(int i = 0; i < itemCount; ++i)
      {
        QLayoutItem* pLayoutItem = layout->itemAt(i);
        QWidget* pWidget = pLayoutItem->widget();
        if (pWidget != NULL)
        {
          //The label text contains the property name.
          QLabel* propertyLabel = dynamic_cast<QLabel*>(pWidget->layout()->itemAt(0)->widget());
          untie(propertyLabel->text());
          pWidget->setHidden(true);
          this->layout()->removeItem(pLayoutItem);
        }
      }
    }

    /**
    Find existing dimension widgets and get rid of them from the layout.
    */
    void SlicingAlgorithmDialog::clearExistingDimensions()
    {
      QLayout* alignedLayout = this->ui.axis_aligned_layout->layout();
      QLayout* nonAlignedLayout = this->ui.non_axis_aligned_layout->layout();

      cleanLayoutOfDimensions(alignedLayout);
      cleanLayoutOfDimensions(nonAlignedLayout);
    }

    /**
    Determine if the inputs should be in an axis aligned form.
    @return : True if axis aligned
    */
    bool SlicingAlgorithmDialog::doAxisAligned() const
    {
      return ui.ck_axis_aligned->isChecked();
    }

    /**
    Gets the provided input workspace name
    @return name of the input workspace
    */
    QString SlicingAlgorithmDialog::getCurrentInputWorkspaceName() const
    {
      return ui.workspace_selector->currentText();
    }

    /**
    Gets the provided output workspace name
    @return name of the output workspace
    */
    QString SlicingAlgorithmDialog::getCurrentOutputWorkspaceName() const
    {
      return ui.txt_output->text();
    }

    /**
    Getter for the historical input workspace name.
    @return old input workspace name.
    */
    QString SlicingAlgorithmDialog::getHistoricalInputWorkspaceName() const
    {
      return MantidQt::API::AlgorithmInputHistory::Instance().previousInput(m_algName, "InputWorkspace");
    }

    /**
    Determine if properties relating to the dimension history have changed. 
    @return True if it has changed.
    */
    SlicingAlgorithmDialog::HistoryChanged SlicingAlgorithmDialog::hasDimensionHistoryChanged() const
    {
      const QString& currentWorkspaceName = getCurrentInputWorkspaceName();
      if(currentWorkspaceName.isEmpty())
      {
        return HasChanged; //Force a rebuild because the dialog cant find any eligable input workspaces. That's why there is an empty entry.
      }

      QString previousInputWorkspaceName = MantidQt::API::AlgorithmInputHistory::Instance().previousInput(m_algName, "InputWorkspace");
      if(previousInputWorkspaceName != currentWorkspaceName)
      {
        return HasChanged; //The input workspace has been switched, so rebuilding will be necessary.
      }

      return HasNotChanged;
    }

    /*
    Decide and command the type of dimension inputs to provide.
    @bForceForget : Force the use of inputworkspace dimensions when configuring the dialog.
    */
    void SlicingAlgorithmDialog::buildDimensionInputs(const bool bForceForget)
    {
      clearExistingDimensions();
      const bool axisAligned = doAxisAligned();
      ui.non_axis_aligned_layout->setEnabled(!axisAligned);

      HistoryChanged changedStatus = hasDimensionHistoryChanged();
      History history;
      if(changedStatus == HasChanged ||  bForceForget)
      {
        history = Forget; 
      }
      else
      {
        history = Remember;
      }

      if(axisAligned)
      {
        buildDimensionInputs("AlignedDim", this->ui.axis_aligned_layout->layout(), formattedAlignedDimensionInput, history);
      }
      else
      {
        buildDimensionInputs("BasisVector", this->ui.non_axis_aligned_layout->layout(), formatNonAlignedDimensionInput, history);
      }
    }

    /**
    Build dimensions from the currently selected input workspace. Also fills
    the inputs with default values.
    @param propertyPrefix: The prefix for the property in the algorithm, i.e. AxisAligned.
    @param owningLayout: The layout that will take ownership of the widgets once generated.
    @param format: function pointer to the formatting function
    @param history : Whether to remember of forget property history.
    */
    void SlicingAlgorithmDialog::buildDimensionInputs(const QString& propertyPrefix, QLayout* owningLayout, QString(*format)(IMDDimension_const_sptr), History history)
    {
      const QString& txt = getCurrentInputWorkspaceName();
      if(!txt.isEmpty())
      {
        IMDWorkspace_sptr ws = boost::dynamic_pointer_cast<IMDWorkspace>(AnalysisDataService::Instance().retrieve(txt.toStdString()));
        size_t nDimensions = ws->getNumDims();
        for(size_t index = 0; index < nDimensions; ++index)
        {
          Mantid::Geometry::IMDDimension_const_sptr dim = ws->getDimension(index);
          
          // Create a widget to contain the dimension components.
          QHBoxLayout* layout = new QHBoxLayout;
          QWidget* w = new QWidget;
          w->setLayout(layout);

          // Configure the label 
          QString propertyName =  propertyPrefix.copy().append(QString().number(index));

          QLabel* dimensionLabel = new QLabel(propertyName);
          
          // Configure the default input.
          QString dimensioninfo = format(dim);

          QLineEdit* txtDimension = new QLineEdit(dimensioninfo);
          tie(txtDimension, propertyName, layout, (history == Remember));

          // Add components to the layout.
          layout->addWidget(dimensionLabel);
          layout->addWidget(txtDimension);

          owningLayout->addWidget(w);
        }
      }
    }

    /// Event handler for the workspace changed event.
    void SlicingAlgorithmDialog::onWorkspaceChanged()
    {
      buildDimensionInputs();
    }

    /** 
    Event handler for the axis changed event.
    This event handler allows us to continually dynamically provide inputs depending upon the dimensionality.
    */
    void SlicingAlgorithmDialog::onAxisAlignedChanged(bool)
    {
      buildDimensionInputs();
    }

    /**
    Event handler for changes so that recursion depth for the ouput workspace is either taken 
    from the input workspace or from an external field.
    */
    void SlicingAlgorithmDialog::onMaxFromInput(bool)
    {
      const bool takeFromInputWorkspace = ui.ck_max_from_input->isChecked();
      ui.txt_resursion_depth->setEnabled(!takeFromInputWorkspace);
      ui.lbl_resursion_depth->setEnabled(!takeFromInputWorkspace);
    }

    /**
    Event handler for the on-forced dimension rebuild event.
    */
    void SlicingAlgorithmDialog::onRebuildDimensions()
    {
      buildDimensionInputs(true);
    }

    /**
    Handler for the onbrowse event.
    */
    void SlicingAlgorithmDialog::onBrowse()
    {
      QFileDialog dialog;
      dialog.setDirectory(QDir::homePath());
      dialog.setNameFilter("Nexus files (*.nxs)");
      if (dialog.exec())
      {
        ui.txt_filename->setText(dialog.selectedFile());
      }
    }

    /*---------------------------------------------------------------------------------------------
    SliceMDDialog Methods
    ---------------------------------------------------------------------------------------------*/
    void SliceMDDialog::customiseInitLayout()
    {
      //Tie the widgets to properties
      tie(ui.ck_max_from_input, "TakeMaxRecursionDepthFromInput");
      tie(ui.txt_resursion_depth, "MaxRecursionDepth");
      tie(ui.txt_filename, "OutputFilename");
      tie(ui.txt_memory, "Memory");
    }

    /*---------------------------------------------------------------------------------------------
    End SliceMDDialog Methods
    ---------------------------------------------------------------------------------------------*/

    /*---------------------------------------------------------------------------------------------
    BinMDDialog Methods
    ---------------------------------------------------------------------------------------------*/
    void BinMDDialog::customiseInitLayout()
    {
      //Disable the stuff that doesn't relate to BinMD
      ui.file_backend_layout->setVisible(false);
      ui.ck_max_from_input->setVisible(false);
      ui.lbl_resursion_depth->setVisible(false);
      ui.txt_resursion_depth->setVisible(false);
    }

    /*---------------------------------------------------------------------------------------------
    End BinMDDialog Methods
   ---------------------------------------------------------------------------------------------*/

  }
}