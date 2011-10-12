#include "MantidQtCustomInterfaces/ApproachDialog.h"
#include <qboxlayout.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qpushbutton.h>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    ApproachDialog::ApproachDialog() : m_aborted(false)
    {
      QVBoxLayout *layout = new QVBoxLayout;
      layout->addWidget(new QLabel("Choose the approach you wish to use for your conversion"));

      m_approaches = new QComboBox;
      m_approaches->addItem("ISIS Inelastic (Default)");
      m_approaches->addItem("ISIS Single Crystal Diffraction");
      layout->addWidget(m_approaches);
      connect(m_approaches,SIGNAL(activated(int)),this ,SLOT(approachChanged()));

      QPushButton* okButton = new QPushButton("OK");
      QPushButton* cancelButton = new QPushButton("Cancel");
      QHBoxLayout* commandsLayout = new QHBoxLayout();
      commandsLayout->addWidget(okButton);
      commandsLayout->addWidget(cancelButton);

      connect(okButton,SIGNAL(clicked()),this ,SLOT(ok()));
      connect(cancelButton,SIGNAL(clicked()),this ,SLOT(cancel()));

      layout->addLayout(commandsLayout);

      this->setLayout(layout);
      this->setFixedWidth(400);
      this->setFixedHeight(200);
    }

    ApproachDialog::~ApproachDialog()
    {
    }

    void ApproachDialog::approachChanged()
    {
      //Do nothing (for now)
    }

    void ApproachDialog::ok()
    {
      m_aborted = false;
      this->close();
    }

    void ApproachDialog::cancel()
    {
      m_aborted = true;
      this->close();
    }

    ApproachType ApproachDialog::getApproach() const
    {
      int index = m_approaches->currentIndex();
      ApproachType chosen;
      switch(index)
      {
      case 1:
        chosen = ISISSingleCrystalDiff;
        break;
      default:
        chosen = ISISInelastic;
      }
      return chosen;
    }

    bool ApproachDialog::getWasAborted() const
    {
      return m_aborted;
    }
  }
}