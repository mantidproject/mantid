#include "MantidQtMantidWidgets/SlicingAlgorithmDialog.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include <QIntValidator>
#include <QFileDialog>
#include <QDir>
#include <QSettings>

using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace MantidQt
{
  namespace MantidWidgets
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

      loadSettings();

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

      /*
       Do not need to connect up Accept/Reject. This gets done automatically, and the AlgorithmDialog base class
       handles the slots.
       */
      connect(ui.workspace_selector,SIGNAL(activated(int)),this ,SLOT(onWorkspaceChanged()));
      connect(ui.ck_axis_aligned, SIGNAL(clicked(bool)), this, SLOT(onAxisAlignedChanged(bool)));
      connect(ui.ck_max_from_input, SIGNAL(clicked(bool)), this, SLOT(onMaxFromInput(bool)));
      connect(ui.ck_calculate, SIGNAL(clicked(bool)), this, SLOT(onCalculateChanged(bool)));
      connect(ui.btn_browse, SIGNAL(clicked()), this, SLOT(onBrowse()));
      connect(ui.btn_help, SIGNAL(clicked()), this, SLOT(helpClicked()));
      connect(ui.btn_calculate, SIGNAL(clicked()), this, SLOT(onRebuildDimensions()));

      //Configure workspace selector
      ui.workspace_selector->setValidatingAlgorithm(m_algName);
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
      buildDimensionInputs(this->doAutoFillDimensions());
    }

    /// Destructor
    SlicingAlgorithmDialog::~SlicingAlgorithmDialog()
    {
      saveSettings();
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
     As the input it is expected the dimension to format the string. 

     It is expected that inherited classes will be able to implement this method, but
     this class can not obviously define how the basis vectors could be automatically formed, 
     so, it retuns always an empty string.

     @return : empty string.
    */
    QString formatNonAlignedDimensionInput(Mantid::Geometry::IMDDimension_const_sptr)
    {
      //Deliberately return an empty string here, because it's not obvious how the basis vectors could be automatically formed.
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
      SlicingAlgorithmDialog::HistoryChanged result = HasNotChanged;

      const QString& currentWorkspaceName = getCurrentInputWorkspaceName();
      const QString previousInputWorkspaceName = MantidQt::API::AlgorithmInputHistory::Instance().previousInput(m_algName, "InputWorkspace");
      if(currentWorkspaceName.isEmpty())
      {
        result = HasChanged; //Force a rebuild because the dialog cant find any eligable input workspaces. That's why there is an empty entry.
      }
      else if(AnalysisDataService::Instance().doesExist(previousInputWorkspaceName.toStdString()))
      {
        const auto oldWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(previousInputWorkspaceName.toStdString());
        const auto newWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(currentWorkspaceName.toStdString());
        if(oldWS->getNumDims() != newWS->getNumDims())
        {
          result = HasChanged;
        }
      }

      return result;
    }

    /**
     * Determine if history should be used.
     * @param criticalChange : Indicates that the inputs are different in some critical fashion
     * @param bForceForget : Force the use of inputworkspace dimensions when configuring the dialog.
     * @return decision about what to do with history, keep it or ignore it.
     */
    SlicingAlgorithmDialog::History SlicingAlgorithmDialog::useHistory(const HistoryChanged& criticalChange, const bool bForceForget)
    {
      History history;
      if (criticalChange == HasChanged || bForceForget)
      {
        history = Forget;
      }
      else
      {
        history = Remember;
      }
      return history;
    }

    /*
    Decide and command the type of dimension inputs to provide.
    @param bForceForget : Force the use of inputworkspace dimensions when configuring the dialog.
    */
    void SlicingAlgorithmDialog::buildDimensionInputs(const bool bForceForget)
    {
      clearExistingDimensions();
      const bool axisAligned = doAxisAligned();
      ui.non_axis_aligned_layout->setEnabled(!axisAligned);

      HistoryChanged criticalChange = this->hasDimensionHistoryChanged();
      History useHistory = this->useHistory(criticalChange, bForceForget);

      if(axisAligned)
      {
        makeDimensionInputs("AlignedDim", this->ui.axis_aligned_layout->layout(), formattedAlignedDimensionInput, useHistory);
      }
      else
      {
        makeDimensionInputs("BasisVector", this->ui.non_axis_aligned_layout->layout(), formatNonAlignedDimensionInput, useHistory);
      }
    }

    /**
    Make dimensions from the currently selected input workspace. Also fills
    the inputs with default values.
    @param propertyPrefix: The prefix for the property in the algorithm, i.e. AxisAligned.
    @param owningLayout: The layout that will take ownership of the widgets once generated.
    @param format: function pointer to the formatting function
    @param history : Whether to remember of forget property history.
    */
    void SlicingAlgorithmDialog::makeDimensionInputs(const QString& propertyPrefix,
        QLayout* owningLayout, QString (*format)(IMDDimension_const_sptr), History history)
    {
      // Remove excess dimensions from the tied properties and the stored property values
      size_t indexRemoved = 0;
      QString propertyNameRemoved = propertyPrefix.copy().append(QString().number(indexRemoved));
      Mantid::Kernel::Property *propertyRemoved = getAlgorithmProperty(propertyNameRemoved);

      while(propertyRemoved)
      {
        untie(propertyNameRemoved);
        removePropertyValue(propertyNameRemoved);

        indexRemoved++;
        propertyNameRemoved = propertyPrefix.copy().append(QString().number(indexRemoved));
        propertyRemoved = getAlgorithmProperty(propertyNameRemoved);
      }

      const QString& txt = getCurrentInputWorkspaceName();
      if (!txt.isEmpty())
      {
        IMDWorkspace_sptr ws = boost::dynamic_pointer_cast<IMDWorkspace>(
            AnalysisDataService::Instance().retrieve(txt.toStdString()));

        size_t nDimensions = ws->getNumDims();

        for (size_t index = 0; index < nDimensions; ++index)
        {
          Mantid::Geometry::IMDDimension_const_sptr dim = ws->getDimension(index);

          // Configure the label
          const QString propertyName = propertyPrefix.copy().append(QString().number(index));

          QLabel* dimensionLabel = new QLabel(propertyName);

          // Configure the default input.
          const QString dimensionInfo = format(dim);

          QLineEdit* txtDimension = new QLineEdit(dimensionInfo);

          // Create a widget to contain the dimension components.
          QHBoxLayout* layout = new QHBoxLayout;
          QWidget* w = new QWidget;
          w->setLayout(layout);

          tie(txtDimension, propertyName, layout, (history == Remember));

          // Add components to the layout.
          layout->addWidget(dimensionLabel);
          layout->addWidget(txtDimension);

          owningLayout->addWidget(w);
        }
      }
    }

    /** Save settings for next time. */
    void SlicingAlgorithmDialog::saveSettings()
    {
      QSettings settings;
      settings.beginGroup("Mantid/SlicingAlgorithm");
      settings.setValue("AlwaysCalculateExtents", (this->doAutoFillDimensions()? 1 : 0));
      settings.endGroup();
    }

    /**
     * Load Settings
     */
    void SlicingAlgorithmDialog::loadSettings()
    {
      QSettings settings;
      settings.beginGroup("Mantid/SlicingAlgorithm");

      const bool alwaysCalculateExtents = settings.value("AlwaysCalculateExtents", 1).toInt();
      ui.ck_calculate->setChecked(alwaysCalculateExtents);

      const QString alignedDimensions = settings.value("AlignedDimensions", "").toString();
      const QString nonAlignedDimensions = settings.value("NonAlignedDimensions", "").toString();

      settings.endGroup();
    }

    /// Event handler for the workspace changed event.
    void SlicingAlgorithmDialog::onWorkspaceChanged()
    {
      buildDimensionInputs(this->doAutoFillDimensions());
    }

    /** 
    Event handler for the axis changed event.
    This event handler allows us to continually dynamically provide inputs depending upon the dimensionality.
    */
    void SlicingAlgorithmDialog::onAxisAlignedChanged(bool)
    {
      buildDimensionInputs(this->doAutoFillDimensions());
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

    void SlicingAlgorithmDialog::onCalculateChanged(bool)
    {
      if(ui.ck_axis_aligned->isChecked())
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

    /*
    Perform tasks that are almost identical for derived types except for visibility switch.
    @isSliceMD. Is the calling method from SliceMD.
    */
    void SlicingAlgorithmDialog::commonSliceMDSetup(const bool isSliceMD)
    {
      ui.file_backend_layout->setVisible(isSliceMD);
      ui.ck_max_from_input->setVisible(isSliceMD);
      ui.lbl_resursion_depth->setVisible(isSliceMD);
      ui.txt_resursion_depth->setVisible(isSliceMD);
      ui.ck_parallel->setVisible(!isSliceMD);
    }

    /**
     * Do auto fill dimension inputs on changes.
     * @return True if do auto repair.
     */
    bool SlicingAlgorithmDialog::doAutoFillDimensions() const
    {
      return ui.ck_calculate->isChecked();
    }

   /**
     *Customise the layout for usage in the Vsi
     */
    void SlicingAlgorithmDialog::customiseLayoutForVsi(std::string initialWorkspace)
    {
      // File back-end
      ui.file_backend_layout->setVisible(false);

      // Output workspace
      ui.lbl_workspace_output->setVisible(false);
      ui.txt_output->setVisible(false);

      // Input workspace
      ui.workspace_selector->setVisible(false);
      ui.lbl_workspace_input->setVisible(false);

      // Reset the input workspace
      ui.workspace_selector->clear();
      ui.workspace_selector->addItem(initialWorkspace.c_str());

      // Turn off history of the aligned dimension fields;
      buildDimensionInputs(true);
    }

    /**
     * Resets the axis dimensions externally.
     * @param index The property index.
     * @param propertyValue The new value of the axis dimension.
     */
    void SlicingAlgorithmDialog::resestAlignedDimProperty(size_t index, QString propertyValue)
    {
      QString alignedDim = "AlignedDim";

      const QString propertyName = alignedDim.copy().append(QString().number(index));

      if (!m_tied_properties.contains(propertyName))
      {
        return;
      }

      QWidget* widget = m_tied_properties[propertyName];

      if (!widget)
      {
        return;
      }

      QLineEdit* edit = dynamic_cast<QLineEdit*>(widget);

      if (!edit)
      {
        return;
      }

      edit->setText(propertyValue);
    }


    /*---------------------------------------------------------------------------------------------
    SliceMDDialog Methods
    ---------------------------------------------------------------------------------------------*/
    void SliceMDDialog::customiseInitLayout()
    {
      commonSliceMDSetup(true);

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
      commonSliceMDSetup(false);

      tie(ui.ck_parallel, "Parallel");
    }

    /*---------------------------------------------------------------------------------------------
    End BinMDDialog Methods
   ---------------------------------------------------------------------------------------------*/

  }
}
