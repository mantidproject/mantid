#include "ReportUsageDisableDialog.h"
#include "FirstTimeSetup.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QStyle>
#include <QVBoxLayout>

ReportUsageDisableDialog::ReportUsageDisableDialog(FirstTimeSetup *parent)
    : QDialog(parent) {
  auto parentLayout = new QHBoxLayout(this);
  this->setLayout(parentLayout);

  this->addLeftSide(parentLayout);
  this->addRightSide(parentLayout, parent);
}

void ReportUsageDisableDialog::addLeftSide(QHBoxLayout *parentLayout) {
  auto iconLayout = new QVBoxLayout();
  auto style = this->style();
  auto icon = style->standardIcon(QStyle::SP_MessageBoxQuestion);

  auto labelButActuallyAnIcon = new QLabel(this);
  labelButActuallyAnIcon->setPixmap(icon.pixmap(32, 32));
  iconLayout->addWidget(labelButActuallyAnIcon);

  auto vspacer =
      new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
  iconLayout->addSpacerItem(vspacer);

  parentLayout->addLayout(iconLayout);
}

void ReportUsageDisableDialog::addRightSide(QHBoxLayout *parentLayout,
                                            FirstTimeSetup *parent) {
  auto textLayout = new QVBoxLayout();
  this->setWindowTitle("Mantid: Report Usage Data ");

  auto label = new QLabel(this);
  label->setTextFormat(Qt::RichText);
  label->setText("Are you sure you want to disable reporting of <a "
                 "href='https://reports.mantidproject.org'>usage data</a>?"
                 "\t(full details in our <a "
                 "href='https://www.mantidproject.org/"
                 "MantidProject:Privacy_policy#Usage_Data_recorded_in_Mantid'"
                 ">Privacy Policy</a>)");
  label->setOpenExternalLinks(false);
  connect(label, SIGNAL(linkActivated(const QString &)), parent,
          SLOT(openExternalLink(const QString &)));
  textLayout->addWidget(label);

  auto labelInformation = new QLabel(this);
  labelInformation->setText(
      "All usage data is anonymous and untraceable.\n"
      "We use the usage data to inform the future development of Mantid.\n"
      "If you click \"Yes\" aspects you need risk being deprecated in "
      "future versions if we think they are not used.\n\n"
      "Are you sure you still want to disable reporting usage data?\n"
      "Please click \"No\".");
  textLayout->addWidget(labelInformation);

  auto buttonLayout = new QHBoxLayout();
  {

    auto leftSpacer =
        new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
    buttonLayout->addSpacerItem(leftSpacer);

    auto noBtn = new QPushButton("No", this);
    buttonLayout->addWidget(noBtn);
    connect(noBtn, SIGNAL(clicked()), this, SLOT(reject()));
  }

  {
    auto yesBtn = new QPushButton("Yes", this);
    connect(yesBtn, SIGNAL(clicked()), this, SLOT(accept()));

    buttonLayout->addWidget(yesBtn);

    auto rightSpacer =
        new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
    buttonLayout->addSpacerItem(rightSpacer);
  }
  textLayout->addLayout(buttonLayout);

  parentLayout->addLayout(textLayout);
}