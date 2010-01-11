/********************************************************************************
** Form generated from reading ui file 'MWDiag.ui'
**
** Created: Tue 22. Dec 17:17:19 2009
**      by: Qt User Interface Compiler version 4.4.3
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_MWDIAG_H
#define UI_MWDIAG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    QGridLayout *gridLayout;
    QLabel *lbIFile;
    QLineEdit *leIFile;
    QSpacerItem *horizontalSpacer_16;
    QPushButton *pbIFile;
    QLabel *lbOFile;
    QLineEdit *leOFile;
    QSpacerItem *horizontalSpacer_14;
    QPushButton *pbOFile;
    QLabel *lbError;
    QLineEdit *leSignificance;
    QSpacerItem *horizontalSpacer_15;
    QCheckBox *ckAngles;
    QGroupBox *gbIndividual;
    QGridLayout *gridLayout_4;
    QLabel *lbWBV1;
    QLabel *lbHighAbs;
    QLineEdit *leHighAbs;
    QLabel *validatorSpace_6;
    QLabel *lbLowAbs;
    QLabel *validatorSpace_4;
    QLabel *lbHighMed;
    QLineEdit *leHighMed;
    QLabel *validatorSpace_5;
    QLineEdit *leLowMed;
    QLabel *validatorSpace_3;
    QLineEdit *leWBV1;
    QLineEdit *leLowAbs;
    QLabel *validatorSpace_9;
    QPushButton *pbWBV1;
    QLabel *lbLowMed;
    QGroupBox *gbVariation;
    QGridLayout *gridLayout_2;
    QSpacerItem *horizontalSpacer_7;
    QSpacerItem *horizontalSpacer_3;
    QLabel *lbWBV2;
    QLineEdit *leWBV2;
    QPushButton *pbWBV2;
    QLabel *lbVariation;
    QLineEdit *leVariation;
    QLabel *validator_label;
    QGroupBox *gbBackTest;
    QGridLayout *gridLayout_3;
    QLabel *lbStartTime;
    QLineEdit *leStartTime;
    QLabel *lbEndTime;
    QLineEdit *leEndTime;
    QLabel *validator_label_3;
    QLabel *lbAcceptance;
    QLineEdit *leAcceptance;
    QCheckBox *ckZeroCounts;

    void setupUi(QWidget *Form)
    {
    if (Form->objectName().isEmpty())
        Form->setObjectName(QString::fromUtf8("Form"));
    Form->resize(388, 377);
    gridLayout = new QGridLayout(Form);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    lbIFile = new QLabel(Form);
    lbIFile->setObjectName(QString::fromUtf8("lbIFile"));
    lbIFile->setEnabled(true);
    QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(lbIFile->sizePolicy().hasHeightForWidth());
    lbIFile->setSizePolicy(sizePolicy);
    lbIFile->setMaximumSize(QSize(85, 16777215));

    gridLayout->addWidget(lbIFile, 0, 0, 1, 1);

    leIFile = new QLineEdit(Form);
    leIFile->setObjectName(QString::fromUtf8("leIFile"));
    leIFile->setEnabled(true);
    leIFile->setMinimumSize(QSize(0, 20));

    gridLayout->addWidget(leIFile, 0, 1, 1, 3);

    horizontalSpacer_16 = new QSpacerItem(20, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

    gridLayout->addItem(horizontalSpacer_16, 0, 4, 1, 1);

    pbIFile = new QPushButton(Form);
    pbIFile->setObjectName(QString::fromUtf8("pbIFile"));
    pbIFile->setEnabled(true);

    gridLayout->addWidget(pbIFile, 0, 5, 1, 1);

    lbOFile = new QLabel(Form);
    lbOFile->setObjectName(QString::fromUtf8("lbOFile"));
    sizePolicy.setHeightForWidth(lbOFile->sizePolicy().hasHeightForWidth());
    lbOFile->setSizePolicy(sizePolicy);
    lbOFile->setMaximumSize(QSize(85, 16777215));

    gridLayout->addWidget(lbOFile, 1, 0, 1, 1);

    leOFile = new QLineEdit(Form);
    leOFile->setObjectName(QString::fromUtf8("leOFile"));

    gridLayout->addWidget(leOFile, 1, 1, 1, 3);

    horizontalSpacer_14 = new QSpacerItem(20, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

    gridLayout->addItem(horizontalSpacer_14, 1, 4, 1, 1);

    pbOFile = new QPushButton(Form);
    pbOFile->setObjectName(QString::fromUtf8("pbOFile"));

    gridLayout->addWidget(pbOFile, 1, 5, 1, 1);

    lbError = new QLabel(Form);
    lbError->setObjectName(QString::fromUtf8("lbError"));
    sizePolicy.setHeightForWidth(lbError->sizePolicy().hasHeightForWidth());
    lbError->setSizePolicy(sizePolicy);
    lbError->setMaximumSize(QSize(85, 16777215));
    lbError->setWordWrap(true);

    gridLayout->addWidget(lbError, 2, 0, 1, 1);

    leSignificance = new QLineEdit(Form);
    leSignificance->setObjectName(QString::fromUtf8("leSignificance"));
    QSizePolicy sizePolicy1(QSizePolicy::Maximum, QSizePolicy::Fixed);
    sizePolicy1.setHorizontalStretch(63);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(leSignificance->sizePolicy().hasHeightForWidth());
    leSignificance->setSizePolicy(sizePolicy1);
    leSignificance->setMaximumSize(QSize(63, 16777215));

    gridLayout->addWidget(leSignificance, 2, 1, 1, 1);

    horizontalSpacer_15 = new QSpacerItem(20, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

    gridLayout->addItem(horizontalSpacer_15, 2, 2, 1, 1);

    ckAngles = new QCheckBox(Form);
    ckAngles->setObjectName(QString::fromUtf8("ckAngles"));

    gridLayout->addWidget(ckAngles, 2, 3, 1, 2);

    gbIndividual = new QGroupBox(Form);
    gbIndividual->setObjectName(QString::fromUtf8("gbIndividual"));
    gridLayout_4 = new QGridLayout(gbIndividual);
    gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
    gridLayout_4->setContentsMargins(5, 2, 0, 2);
    lbWBV1 = new QLabel(gbIndividual);
    lbWBV1->setObjectName(QString::fromUtf8("lbWBV1"));
    QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy2.setHorizontalStretch(0);
    sizePolicy2.setVerticalStretch(0);
    sizePolicy2.setHeightForWidth(lbWBV1->sizePolicy().hasHeightForWidth());
    lbWBV1->setSizePolicy(sizePolicy2);

    gridLayout_4->addWidget(lbWBV1, 0, 0, 1, 1);

    lbHighAbs = new QLabel(gbIndividual);
    lbHighAbs->setObjectName(QString::fromUtf8("lbHighAbs"));
    sizePolicy2.setHeightForWidth(lbHighAbs->sizePolicy().hasHeightForWidth());
    lbHighAbs->setSizePolicy(sizePolicy2);

    gridLayout_4->addWidget(lbHighAbs, 1, 0, 1, 1);

    leHighAbs = new QLineEdit(gbIndividual);
    leHighAbs->setObjectName(QString::fromUtf8("leHighAbs"));

    gridLayout_4->addWidget(leHighAbs, 1, 1, 1, 1);

    validatorSpace_6 = new QLabel(gbIndividual);
    validatorSpace_6->setObjectName(QString::fromUtf8("validatorSpace_6"));
    QSizePolicy sizePolicy3(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy3.setHorizontalStretch(20);
    sizePolicy3.setVerticalStretch(20);
    sizePolicy3.setHeightForWidth(validatorSpace_6->sizePolicy().hasHeightForWidth());
    validatorSpace_6->setSizePolicy(sizePolicy3);
    validatorSpace_6->setMinimumSize(QSize(10, 20));

    gridLayout_4->addWidget(validatorSpace_6, 1, 2, 1, 1);

    lbLowAbs = new QLabel(gbIndividual);
    lbLowAbs->setObjectName(QString::fromUtf8("lbLowAbs"));
    sizePolicy2.setHeightForWidth(lbLowAbs->sizePolicy().hasHeightForWidth());
    lbLowAbs->setSizePolicy(sizePolicy2);

    gridLayout_4->addWidget(lbLowAbs, 1, 3, 1, 1);

    validatorSpace_4 = new QLabel(gbIndividual);
    validatorSpace_4->setObjectName(QString::fromUtf8("validatorSpace_4"));
    sizePolicy3.setHeightForWidth(validatorSpace_4->sizePolicy().hasHeightForWidth());
    validatorSpace_4->setSizePolicy(sizePolicy3);
    validatorSpace_4->setMinimumSize(QSize(10, 20));

    gridLayout_4->addWidget(validatorSpace_4, 1, 8, 1, 1);

    lbHighMed = new QLabel(gbIndividual);
    lbHighMed->setObjectName(QString::fromUtf8("lbHighMed"));
    sizePolicy2.setHeightForWidth(lbHighMed->sizePolicy().hasHeightForWidth());
    lbHighMed->setSizePolicy(sizePolicy2);
    lbHighMed->setWordWrap(true);

    gridLayout_4->addWidget(lbHighMed, 2, 0, 1, 1);

    leHighMed = new QLineEdit(gbIndividual);
    leHighMed->setObjectName(QString::fromUtf8("leHighMed"));

    gridLayout_4->addWidget(leHighMed, 2, 1, 1, 1);

    validatorSpace_5 = new QLabel(gbIndividual);
    validatorSpace_5->setObjectName(QString::fromUtf8("validatorSpace_5"));
    sizePolicy3.setHeightForWidth(validatorSpace_5->sizePolicy().hasHeightForWidth());
    validatorSpace_5->setSizePolicy(sizePolicy3);
    validatorSpace_5->setMinimumSize(QSize(10, 20));

    gridLayout_4->addWidget(validatorSpace_5, 2, 2, 1, 1);

    leLowMed = new QLineEdit(gbIndividual);
    leLowMed->setObjectName(QString::fromUtf8("leLowMed"));

    gridLayout_4->addWidget(leLowMed, 2, 4, 1, 3);

    validatorSpace_3 = new QLabel(gbIndividual);
    validatorSpace_3->setObjectName(QString::fromUtf8("validatorSpace_3"));
    QSizePolicy sizePolicy4(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy4.setHorizontalStretch(20);
    sizePolicy4.setVerticalStretch(20);
    sizePolicy4.setHeightForWidth(validatorSpace_3->sizePolicy().hasHeightForWidth());
    validatorSpace_3->setSizePolicy(sizePolicy4);
    validatorSpace_3->setMinimumSize(QSize(10, 20));

    gridLayout_4->addWidget(validatorSpace_3, 2, 8, 1, 1);

    leWBV1 = new QLineEdit(gbIndividual);
    leWBV1->setObjectName(QString::fromUtf8("leWBV1"));

    gridLayout_4->addWidget(leWBV1, 0, 1, 1, 4);

    leLowAbs = new QLineEdit(gbIndividual);
    leLowAbs->setObjectName(QString::fromUtf8("leLowAbs"));

    gridLayout_4->addWidget(leLowAbs, 1, 4, 1, 3);

    validatorSpace_9 = new QLabel(gbIndividual);
    validatorSpace_9->setObjectName(QString::fromUtf8("validatorSpace_9"));
    sizePolicy3.setHeightForWidth(validatorSpace_9->sizePolicy().hasHeightForWidth());
    validatorSpace_9->setSizePolicy(sizePolicy3);
    validatorSpace_9->setMinimumSize(QSize(20, 20));

    gridLayout_4->addWidget(validatorSpace_9, 0, 5, 1, 1);

    pbWBV1 = new QPushButton(gbIndividual);
    pbWBV1->setObjectName(QString::fromUtf8("pbWBV1"));

    gridLayout_4->addWidget(pbWBV1, 0, 6, 1, 1);

    lbLowMed = new QLabel(gbIndividual);
    lbLowMed->setObjectName(QString::fromUtf8("lbLowMed"));
    sizePolicy2.setHeightForWidth(lbLowMed->sizePolicy().hasHeightForWidth());
    lbLowMed->setSizePolicy(sizePolicy2);
    lbLowMed->setWordWrap(true);

    gridLayout_4->addWidget(lbLowMed, 2, 3, 1, 1);


    gridLayout->addWidget(gbIndividual, 3, 0, 1, 6);

    gbVariation = new QGroupBox(Form);
    gbVariation->setObjectName(QString::fromUtf8("gbVariation"));
    gridLayout_2 = new QGridLayout(gbVariation);
    gridLayout_2->setMargin(2);
    gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
    horizontalSpacer_7 = new QSpacerItem(23, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    gridLayout_2->addItem(horizontalSpacer_7, 1, 2, 1, 1);

    horizontalSpacer_3 = new QSpacerItem(20, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

    gridLayout_2->addItem(horizontalSpacer_3, 1, 4, 1, 1);

    lbWBV2 = new QLabel(gbVariation);
    lbWBV2->setObjectName(QString::fromUtf8("lbWBV2"));
    sizePolicy2.setHeightForWidth(lbWBV2->sizePolicy().hasHeightForWidth());
    lbWBV2->setSizePolicy(sizePolicy2);

    gridLayout_2->addWidget(lbWBV2, 0, 0, 1, 1);

    leWBV2 = new QLineEdit(gbVariation);
    leWBV2->setObjectName(QString::fromUtf8("leWBV2"));

    gridLayout_2->addWidget(leWBV2, 0, 1, 1, 3);

    pbWBV2 = new QPushButton(gbVariation);
    pbWBV2->setObjectName(QString::fromUtf8("pbWBV2"));

    gridLayout_2->addWidget(pbWBV2, 0, 4, 1, 1);

    lbVariation = new QLabel(gbVariation);
    lbVariation->setObjectName(QString::fromUtf8("lbVariation"));
    lbVariation->setWordWrap(true);

    gridLayout_2->addWidget(lbVariation, 1, 0, 1, 1);

    leVariation = new QLineEdit(gbVariation);
    leVariation->setObjectName(QString::fromUtf8("leVariation"));
    QSizePolicy sizePolicy5(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    sizePolicy5.setHorizontalStretch(0);
    sizePolicy5.setVerticalStretch(0);
    sizePolicy5.setHeightForWidth(leVariation->sizePolicy().hasHeightForWidth());
    leVariation->setSizePolicy(sizePolicy5);
    leVariation->setMinimumSize(QSize(80, 0));

    gridLayout_2->addWidget(leVariation, 1, 1, 1, 1);

    validator_label = new QLabel(gbVariation);
    validator_label->setObjectName(QString::fromUtf8("validator_label"));

    gridLayout_2->addWidget(validator_label, 1, 5, 1, 1);


    gridLayout->addWidget(gbVariation, 4, 0, 1, 6);

    gbBackTest = new QGroupBox(Form);
    gbBackTest->setObjectName(QString::fromUtf8("gbBackTest"));
    gridLayout_3 = new QGridLayout(gbBackTest);
    gridLayout_3->setMargin(2);
    gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
    lbStartTime = new QLabel(gbBackTest);
    lbStartTime->setObjectName(QString::fromUtf8("lbStartTime"));

    gridLayout_3->addWidget(lbStartTime, 0, 0, 1, 1);

    leStartTime = new QLineEdit(gbBackTest);
    leStartTime->setObjectName(QString::fromUtf8("leStartTime"));

    gridLayout_3->addWidget(leStartTime, 0, 1, 1, 1);

    lbEndTime = new QLabel(gbBackTest);
    lbEndTime->setObjectName(QString::fromUtf8("lbEndTime"));

    gridLayout_3->addWidget(lbEndTime, 0, 2, 1, 1);

    leEndTime = new QLineEdit(gbBackTest);
    leEndTime->setObjectName(QString::fromUtf8("leEndTime"));

    gridLayout_3->addWidget(leEndTime, 0, 3, 1, 1);

    validator_label_3 = new QLabel(gbBackTest);
    validator_label_3->setObjectName(QString::fromUtf8("validator_label_3"));
    validator_label_3->setMinimumSize(QSize(10, 20));

    gridLayout_3->addWidget(validator_label_3, 0, 4, 1, 1);

    lbAcceptance = new QLabel(gbBackTest);
    lbAcceptance->setObjectName(QString::fromUtf8("lbAcceptance"));

    gridLayout_3->addWidget(lbAcceptance, 1, 0, 1, 1);

    leAcceptance = new QLineEdit(gbBackTest);
    leAcceptance->setObjectName(QString::fromUtf8("leAcceptance"));

    gridLayout_3->addWidget(leAcceptance, 1, 1, 1, 1);

    ckZeroCounts = new QCheckBox(gbBackTest);
    ckZeroCounts->setObjectName(QString::fromUtf8("ckZeroCounts"));

    gridLayout_3->addWidget(ckZeroCounts, 1, 2, 1, 2);


    gridLayout->addWidget(gbBackTest, 5, 0, 1, 6);


    retranslateUi(Form);

    QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
    Form->setWindowTitle(QApplication::translate("Form", "Form", 0, QApplication::UnicodeUTF8));
    lbIFile->setText(QApplication::translate("Form", "Input Mask File", 0, QApplication::UnicodeUTF8));
    leIFile->setText(QString());
    pbIFile->setText(QApplication::translate("Form", "Browse", 0, QApplication::UnicodeUTF8));
    lbOFile->setText(QApplication::translate("Form", "Output Mask File", 0, QApplication::UnicodeUTF8));
    pbOFile->setText(QApplication::translate("Form", "Browse", 0, QApplication::UnicodeUTF8));
    lbError->setText(QApplication::translate("Form", "Errorbar Criterion", 0, QApplication::UnicodeUTF8));
    ckAngles->setText(QApplication::translate("Form", "No solid angles", 0, QApplication::UnicodeUTF8));
    gbIndividual->setTitle(QApplication::translate("Form", "Individual White Beam Tests", 0, QApplication::UnicodeUTF8));
    lbWBV1->setText(QApplication::translate("Form", "White Beam Van1", 0, QApplication::UnicodeUTF8));
    lbHighAbs->setText(QApplication::translate("Form", "High Counts", 0, QApplication::UnicodeUTF8));
    validatorSpace_6->setText(QString());
    lbLowAbs->setText(QApplication::translate("Form", "Low Counts", 0, QApplication::UnicodeUTF8));
    validatorSpace_4->setText(QString());
    lbHighMed->setText(QApplication::translate("Form", "Median Test High", 0, QApplication::UnicodeUTF8));
    validatorSpace_5->setText(QString());
    validatorSpace_3->setText(QString());
    validatorSpace_9->setText(QString());
    pbWBV1->setText(QApplication::translate("Form", "Browse", 0, QApplication::UnicodeUTF8));
    lbLowMed->setText(QApplication::translate("Form", "Median Test Low", 0, QApplication::UnicodeUTF8));
    gbVariation->setTitle(QApplication::translate("Form", "Efficiency Variation Test", 0, QApplication::UnicodeUTF8));
    lbWBV2->setText(QApplication::translate("Form", "WhiteBeamVan 2", 0, QApplication::UnicodeUTF8));
    pbWBV2->setText(QApplication::translate("Form", "Browse", 0, QApplication::UnicodeUTF8));
    lbVariation->setText(QApplication::translate("Form", "Proportional change criterion", 0, QApplication::UnicodeUTF8));
    validator_label->setText(QString());
    gbBackTest->setTitle(QApplication::translate("Form", "Background check", 0, QApplication::UnicodeUTF8));
    lbStartTime->setText(QApplication::translate("Form", "Start TOF", 0, QApplication::UnicodeUTF8));
    lbEndTime->setText(QApplication::translate("Form", "End TOF", 0, QApplication::UnicodeUTF8));
    validator_label_3->setText(QString());
    lbAcceptance->setText(QApplication::translate("Form", "Acceptance Factor", 0, QApplication::UnicodeUTF8));
    ckZeroCounts->setText(QApplication::translate("Form", "Find spectra with zero counts", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(Form);
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MWDIAG_H
