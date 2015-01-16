#include "MantidVatesSimpleGuiQtWidgets/RebinDialog.h"
#include <QDialog>
#include <QLayoutItem>
#include <vector>
#include <QStringList>
#include <QSpinBox>
#include <QLabel>



namespace Mantid
{
  namespace Vates
  {
    namespace SimpleGui
    {
      RebinDialog::RebinDialog(QWidget* parent) : QDialog(parent), m_validBins(false)
      {
        this->ui.setupUi(this);
        
        QObject::connect(this->ui.buttonBox, SIGNAL(accepted()),
                         this, SLOT(onAccept()));
      }

      RebinDialog::~RebinDialog()
      {
      }
      /**
       * Sets the list of algorithms for the user to select
       * @param algorithms The list of algorithms
       */
      void RebinDialog::setAlgorithms(QStringList algorithms)
      {
        this->ui.comboBoxAlgorithms->clear();

        this->ui.comboBoxAlgorithms->addItems(algorithms);
      }

      void RebinDialog::setBins(std::vector<QString> binNames, std::vector<int> bins)
      {
        // Remove existing bins from Layout
        QLayoutItem* child;
        while ((child = this->ui.layoutBins->takeAt(0)) != 0)
        {
          child->widget()->deleteLater();
          delete child;
        }

        // Set the bins
        int minimum = 1;
        int maximum = 1000;

        // Add bin 1 
        lblBin1 = new QLabel();
        
        boxBin1 = new QSpinBox();
        boxBin1->setMaximum(maximum);
        boxBin1->setMinimum(minimum);
        boxBin1->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);

        ui.layoutBins->addWidget(lblBin1, 0, 0);
        ui.layoutBins->addWidget(boxBin1, 0, 1);

        // Add bin 2
        lblBin2 = new QLabel();
        
        boxBin2 = new QSpinBox();
        boxBin2->setMaximum(maximum);
        boxBin2->setMinimum(minimum);
        boxBin2->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);

        ui.layoutBins->addWidget(lblBin2, 1, 0);
        ui.layoutBins->addWidget(boxBin2, 1, 1);

        // Add bin 3
        lblBin3 = new QLabel();
        
        boxBin3 = new QSpinBox();
        boxBin3->setMaximum(maximum);
        boxBin3->setMinimum(minimum);
        boxBin3->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);

        ui.layoutBins->addWidget(lblBin3, 2, 0);
        ui.layoutBins->addWidget(boxBin3, 2, 1);

        // Set the value
        if((bins.size() == binNames.size()) && bins.size() == 3)
        {
          boxBin1->setVisible(true);
          boxBin2->setVisible(true);
          boxBin3->setVisible(true);


          lblBin1->setText(binNames[0]);
          boxBin1->setValue(bins[0]);

          lblBin2->setText(binNames[1]);
          boxBin2->setValue(bins[1]);

          lblBin3->setText(binNames[2]);
          boxBin3->setValue(bins[2]);

          m_validBins = true;
        }
        else
        {
          boxBin1->setVisible(false);
          boxBin2->setVisible(false);
          boxBin3->setVisible(false);

          m_validBins = false;
        }
      }

      void RebinDialog::onUpdateDialog(QStringList algorithms,std::vector<QString> binNames, std::vector<int> bins)
      {
        this->setAlgorithms(algorithms);
        this->setBins(binNames, bins);
      }

      void RebinDialog::onAccept()
      {
        // Get the selected algorithm
        QString algorithm = this->ui.comboBoxAlgorithms->currentText();

        // Get the bin information
        std::vector<int> bins;
        bins.push_back(boxBin1->value());
        bins.push_back(boxBin2->value());
        bins.push_back(boxBin3->value());

        std::vector<QString> binNames;
        binNames.push_back(lblBin1->text());
        binNames.push_back(lblBin2->text());
        binNames.push_back(lblBin3->text());


        // Send the request for rebinning only if all options are availble.
        if (m_validBins)
        {
          emit performRebinning(algorithm, binNames, bins);
        }
      }

    } // SimpleGui
  } // Vates
} // Mantid
