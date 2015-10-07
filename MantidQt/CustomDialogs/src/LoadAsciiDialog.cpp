//----------------------
// Includes
//----------------------
#include "MantidQtCustomDialogs/LoadAsciiDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include <QValidator>
#include <QtGui>
#include <QFileInfo>

namespace MantidQt
{
  namespace CustomDialogs
  {
    DECLARE_DIALOG(LoadAsciiDialog)

    LoadAsciiDialog::LoadAsciiDialog(QWidget *parent)
      : MantidQt::API::AlgorithmDialog(parent),
      m_lineFilename(NULL),
      m_lineOutputWorkspace(NULL),
      m_lineCommentIndicator(NULL),
      m_lineCustomSeparator(NULL),
      m_separatorBox(NULL)
    {
    }

    LoadAsciiDialog::~LoadAsciiDialog()
    {
    }

    void LoadAsciiDialog::initLayout()
    {
      QGridLayout *paramsLayout = new QGridLayout;

      QLabel *label = new QLabel(tr("Filename"));
      m_lineFilename = new QLineEdit;
      label->setBuddy(m_lineFilename);
      paramsLayout->addWidget(label,0,0);
      paramsLayout->addWidget(m_lineFilename,0,1);
      tie(m_lineFilename, "Filename", paramsLayout);
      connect(m_lineFilename,SIGNAL(editingFinished()),this,SLOT(checkFileExtension()));

      QPushButton *browseBtn = new QPushButton("Browse");
      connect(browseBtn, SIGNAL(clicked()), this, SLOT(browseClicked()));
      browseBtn->setEnabled(isWidgetEnabled("Filename"));
      paramsLayout->addWidget(browseBtn,0,3);

      label = new QLabel(tr("Workspace"));
      m_lineOutputWorkspace = new QLineEdit;
      label->setBuddy(m_lineOutputWorkspace);
      paramsLayout->addWidget(label,1,0);
      paramsLayout->addWidget(m_lineOutputWorkspace,1,1);
      tie(m_lineOutputWorkspace, "OutputWorkspace", paramsLayout);

      m_separatorBox = new QComboBox;
      fillAndSetComboBox("Separator",m_separatorBox);
      paramsLayout->addWidget(new QLabel("Separator"),2,0);
      paramsLayout->addWidget(m_separatorBox,2,1);
      tie(m_separatorBox,"Separator",paramsLayout);

      if (getAlgorithm()->version() == 2)
      {
        label = new QLabel(tr("CustomSeparator"));
        m_lineCustomSeparator = new QLineEdit;
        label->setBuddy(m_lineCustomSeparator);
        paramsLayout->addWidget(label,3,0);
        paramsLayout->addWidget(m_lineCustomSeparator,3,1);
        tie(m_lineCustomSeparator, "CustomSeparator", paramsLayout);

        label = new QLabel(tr("CommentIndicator"));
        m_lineCommentIndicator = new QLineEdit;
        label->setBuddy(m_lineCommentIndicator);
        paramsLayout->addWidget(label,4,0);
        paramsLayout->addWidget(m_lineCommentIndicator,4,1);
        tie(m_lineCommentIndicator, "CommentIndicator", paramsLayout);

        QComboBox *unitBox = new QComboBox;
        fillAndSetComboBox("Unit",unitBox );
        paramsLayout->addWidget(new QLabel("Unit"),5,0);
        paramsLayout->addWidget(unitBox ,5,1);
        tie(unitBox ,"Unit",paramsLayout);
      }
      else
      {
        QComboBox *unitBox = new QComboBox;
        fillAndSetComboBox("Unit",unitBox );
        paramsLayout->addWidget(new QLabel("Unit"),3,0);
        paramsLayout->addWidget(unitBox ,3,1);
        tie(unitBox ,"Unit",paramsLayout);
      }

      QVBoxLayout *mainLayout = new QVBoxLayout;

      //Adds the yellow optional message
      if( isMessageAvailable() )
      {
        this->addOptionalMessage(mainLayout);
      }

      mainLayout->addLayout(paramsLayout);
      mainLayout->addLayout(createDefaultButtonLayout("?", "Load", "Cancel"));

      setLayout(mainLayout);
      setFixedHeight(sizeHint().height());

      checkFileExtension();
    }

    /**
    * A slot for the browse button "clicked" signal
    */
    void LoadAsciiDialog::browseClicked()
    {
      if( !m_lineFilename->text().isEmpty() )
      {
        MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(m_lineFilename->text()).absoluteDir().path());
      }

      QString filepath = this->openFileDialog("Filename");
      if( !filepath.isEmpty() )
      {
        m_lineFilename->clear();
        m_lineFilename->setText(filepath.trimmed());
      }

      //Add a suggestion for workspace name
      if( m_lineOutputWorkspace->isEnabled() && !filepath.isEmpty() ) m_lineOutputWorkspace->setText(QFileInfo(filepath).baseName());

      checkFileExtension();
    }

    /**
    * Check for consistency between the file extension and the separator
    */
    void LoadAsciiDialog::checkFileExtension()
    {
      QString fileName = m_lineFilename->text();
      if (fileName.isEmpty()) return;
      QFileInfo file(fileName);
      if (file.suffix().toLower() == "csv")
      {
        m_separatorBox->setCurrentIndex(m_separatorBox->findText("CSV"));
      }
    }
  }
}
