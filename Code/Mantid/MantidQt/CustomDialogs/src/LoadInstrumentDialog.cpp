#include "MantidQtCustomDialogs/LoadInstrumentDialog.h"

#include <QFileDialog>
#include <QDir>
#include <QBoxLayout>
#include <QMessageBox>

using namespace Mantid::API;

namespace MantidQt
{
  namespace CustomDialogs
  {
    DECLARE_DIALOG(LoadInstrumentDialog)

    /**
    Constructor
    @param parent : parent widget
    */
    LoadInstrumentDialog::LoadInstrumentDialog(QWidget* parent) : AlgorithmDialog(parent)
    {
    }


    /// Set up the dialog layout
    void LoadInstrumentDialog::initLayout()
    {
      ui.setupUi(this);
      this->setWindowTitle(m_algName);

      // Add the description of the algorithm text to the dialog.
      QVBoxLayout* tempLayout = new QVBoxLayout;
      this->addOptionalMessage(tempLayout);
      ui.main_layout->addLayout(tempLayout, 0, 0, 1, 3);

      tie(ui.workspaceSelector, "Workspace", ui.ws_validator_layout);
      tie(ui.txt_idf, "Filename", ui.idf_validator_layout);
      tie(ui.txt_instrument_name, "InstrumentName");
      tie(ui.txt_instrument_xml, "InstrumentXML");
      tie(ui.ck_rewrite_spec_map, "RewriteSpectraMap");
      
      ui.workspaceSelector->setValidatingAlgorithm(m_algName);

      connect(ui.controls, SIGNAL(accepted()), this, SLOT(accept()));

      connect(ui.btn_idf, SIGNAL(clicked()), this, SLOT(onBrowse()));
    }

    /**
    Event handler for the browsing click event. 
    */
    void LoadInstrumentDialog::onBrowse()
    {
      QFileDialog dialog;
      dialog.setDirectory(QDir::homePath());
      dialog.setNameFilter("IDF (*.xml)");
      if (dialog.exec())
      {
        ui.txt_idf->setText(dialog.selectedFile());
      }
    }

    /**
    Event handler for the accepted slot. Called when the user wishes to execute the algorithm.

    Provides additional confirmation that the spectra-detector map will be overwritten if selected.
    */
    void LoadInstrumentDialog::accept()
    {
      if(ui.ck_rewrite_spec_map->isChecked())
      {
        QMessageBox warningBox;
        warningBox.setText("Are you sure you want to re-write the spectra detector map with a 1:1 mapping?.");
        warningBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        int result = warningBox.exec();
        if(result == QMessageBox::Cancel)
        {
          ui.ck_rewrite_spec_map->setChecked(false);
          return;
        }
      }
      AlgorithmDialog::accept();
    }
    
    /**
    Destructor
    */
    LoadInstrumentDialog::~LoadInstrumentDialog()
    {
    }

  }
}