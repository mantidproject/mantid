//----------------------
// Includes
//----------------------
#include "MantidQtCustomDialogs/PeakIntegrationDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

// Qt
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QUrl>
#include <QDesktopServices>
#include <QFileInfo>
#include <QValidator>
#include <QtGui>
#include <QPushButton>

namespace MantidQt
{
  namespace CustomDialogs
  {

    DECLARE_DIALOG(PeakIntegrationDialog)
    
    PeakIntegrationDialog::PeakIntegrationDialog(QWidget *parent) 
    : MantidQt::API::AlgorithmDialog(parent)
    {
    }
    
    PeakIntegrationDialog::~PeakIntegrationDialog()
    {
    	
    }
    
    void PeakIntegrationDialog::initLayout()
    {
      m_loaderLayout = new QGridLayout;
    
      wksp1_opt = new QComboBox;
      fillAndSetComboBox("InPeaksWorkspace",wksp1_opt);
      m_loaderLayout->addWidget(new QLabel(tr("InPeaksWorkspace")),0,0);
      m_loaderLayout->addWidget(wksp1_opt,0,1);
      tie(wksp1_opt, "InPeaksWorkspace", m_loaderLayout);
      flagInputWS(wksp1_opt);
    
      wksp2_opt = new QComboBox;
      fillAndSetComboBox("InputWorkspace",wksp2_opt);
      m_loaderLayout->addWidget(new QLabel(tr("InputWorkspace")),1,0);
      m_loaderLayout->addWidget(wksp2_opt,1,1);
      tie(wksp2_opt, "InputWorkspace", m_loaderLayout);
    
      label = new QLabel(tr("OutPeaksWorkspace"));
      input = new QLineEdit;
      label->setBuddy(input);
      m_loaderLayout->addWidget(label,2,0);
      m_loaderLayout->addWidget(input,2,1);
      tie(input, "OutPeaksWorkspace", m_loaderLayout);
      QPushButton *inputWSReplace = this->createReplaceWSButton(input);
      inputWSReplace->setEnabled(true);
      m_loaderLayout->addWidget(inputWSReplace, 2, 2, 0);
    
      checkbox = new QCheckBox ("IkedaCarpenterTOF",this);
      m_loaderLayout->addWidget(checkbox);
      tie(checkbox, "IkedaCarpenterTOF", m_loaderLayout);
    
      checkbox = new QCheckBox ("MatchingRunNo",this);
      m_loaderLayout->addWidget(checkbox);
      tie(checkbox, "MatchingRunNo", m_loaderLayout);
    
      checkbox = new QCheckBox ("FitSlices",this);
      m_loaderLayout->addWidget(checkbox);
      tie(checkbox, "FitSlices", m_loaderLayout);
      connect(checkbox, SIGNAL(stateChanged(int)), this, SLOT(createDynamicLayout(int)));
    
      m_extrasLayout = new QGridLayout;
      Qt::CheckState state = checkbox->checkState();
      if(!(state == Qt::Checked))
      {
        label1 = new QLabel(tr("XMin"));
        input1 = new QLineEdit;
        label1->setBuddy(input1);
        m_extrasLayout->addWidget(label1,0,0);
        m_extrasLayout->addWidget(input1,0,1);
        tie(input1, "XMin", NULL);
      
        label2 = new QLabel(tr("XMax"));
        input2 = new QLineEdit;
        label2->setBuddy(input2);
        m_extrasLayout->addWidget(label2,1,0);
        m_extrasLayout->addWidget(input2,1,1);
        tie(input2, "XMax", NULL);
      
        label3 = new QLabel(tr("YMin"));
        input3 = new QLineEdit;
        label3->setBuddy(input3);
        m_extrasLayout->addWidget(label3,2,0);
        m_extrasLayout->addWidget(input3,2,1);
        tie(input3, "YMin", NULL);
      
        label4 = new QLabel(tr("YMax"));
        input4 = new QLineEdit;
        label4->setBuddy(input4);
        m_extrasLayout->addWidget(label4,3,0);
        m_extrasLayout->addWidget(input4,3,1);
        tie(input4, "YMax", NULL);
      
        label5 = new QLabel(tr("TOFBinMin"));
        input5 = new QLineEdit;
        label5->setBuddy(input5);
        m_extrasLayout->addWidget(label5,4,0);
        m_extrasLayout->addWidget(input5,4,1);
        tie(input5, "TOFBinMin", NULL);
      
        label6 = new QLabel(tr("TOFBinMax"));
        input6 = new QLineEdit;
        label6->setBuddy(input6);
        m_extrasLayout->addWidget(label6,5,0);
        m_extrasLayout->addWidget(input6,5,1);
        tie(input6, "TOFBinMax", NULL);
      }
    
      m_dialogLayout = new QVBoxLayout;
      
      //Adds the yellow optional message
      if( isMessageAvailable() )
      {
        this->addOptionalMessage(m_dialogLayout);
      }
    
      m_dialogLayout->addLayout(m_loaderLayout);
      m_dialogLayout->addLayout(m_extrasLayout);
      m_dialogLayout->addLayout(createDefaultButtonLayout("?", "Run", "Cancel"));
    
      setLayout(m_dialogLayout);
    
    }
    
    void PeakIntegrationDialog::createDynamicLayout(int state)
    {
      // Remove the old widgets if necessary
      if( m_extrasLayout )
      {
        m_dialogLayout->takeAt(1);
        QLayoutItem *child;
        while( (child = m_extrasLayout->takeAt(0)) != NULL )
        {
          // Mark the widget for deletion later as deleting it now can cause contention issues while
          // things are changing
          child->widget()->deleteLater();
          delete child;
        }
        m_extrasLayout->deleteLater();
        m_extrasLayout = NULL;
      }
      m_extrasLayout = new QGridLayout;
      untie("XMin");
      untie("XMax");
      untie("YMin");
      untie("YMax");
      untie("TOFBinMin");
      untie("TOFBinMax");
    
      if(state == Qt::Checked)
      {
        setFixedHeight(sizeHint().height());
      }
      else
      {
        setFixedHeight(sizeHint().height()+200);
        label1 = new QLabel(tr("XMin"));
        input1 = new QLineEdit;
        label1->setBuddy(input1);
        m_extrasLayout->addWidget(label1,0,0);
        m_extrasLayout->addWidget(input1,0,1);
        tie(input1, "XMin", NULL);
      
        label2 = new QLabel(tr("XMax"));
        input2 = new QLineEdit;
        label2->setBuddy(input2);
        m_extrasLayout->addWidget(label2,1,0);
        m_extrasLayout->addWidget(input2,1,1);
        tie(input2, "XMax", NULL);
      
        label3 = new QLabel(tr("YMin"));
        input3 = new QLineEdit;
        label3->setBuddy(input3);
        m_extrasLayout->addWidget(label3,2,0);
        m_extrasLayout->addWidget(input3,2,1);
        tie(input3, "YMin", NULL);
      
        label4 = new QLabel(tr("YMax"));
        input4 = new QLineEdit;
        label4->setBuddy(input4);
        m_extrasLayout->addWidget(label4,3,0);
        m_extrasLayout->addWidget(input4,3,1);
        tie(input4, "YMax", NULL);
      
        label5 = new QLabel(tr("TOFBinMin"));
        input5 = new QLineEdit;
        label5->setBuddy(input5);
        m_extrasLayout->addWidget(label5,4,0);
        m_extrasLayout->addWidget(input5,4,1);
        tie(input5, "TOFBinMin", NULL);
      
        label6 = new QLabel(tr("TOFBinMax"));
        input6 = new QLineEdit;
        label6->setBuddy(input6);
        m_extrasLayout->addWidget(label6,5,0);
        m_extrasLayout->addWidget(input6,5,1);
        tie(input6, "TOFBinMax", NULL);
      }
      m_dialogLayout->insertLayout(1, m_extrasLayout);
    }

  }

}
