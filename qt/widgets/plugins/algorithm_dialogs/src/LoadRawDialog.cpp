// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plugins/AlgorithmDialogs/LoadRawDialog.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"

#include "MantidKernel/Property.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFontMetrics>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

// Add this class to the list of specialized dialogs in this namespace
namespace MantidQt::CustomDialogs {
DECLARE_DIALOG(LoadRawDialog)
} // namespace MantidQt::CustomDialogs

// Just to save writing this everywhere
using namespace MantidQt::CustomDialogs;

//---------------------------------------
// Public member functions
//---------------------------------------
/**
 * Constructor
 */
LoadRawDialog::LoadRawDialog(QWidget *parent) : AlgorithmDialog(parent), m_pathBox(nullptr), m_wsBox(nullptr) {}

/**
 *Destructor
 */
LoadRawDialog::~LoadRawDialog() = default;

//---------------------------------------
// Private member functions
//---------------------------------------
/**
 * Reimplemented virtual function to set up the dialog
 */
void LoadRawDialog::initLayout() {
  QVBoxLayout *main_layout = new QVBoxLayout(this);

  // Add the helpful summary message
  if (isMessageAvailable())
    this->addOptionalMessage(main_layout);

  //------------- Filename property ---------------------
  auto *prop_line = new QHBoxLayout;
  prop_line->addWidget(new QLabel("Select a file to load:"));

  m_pathBox = new QLineEdit;
  m_pathBox->setMinimumWidth(m_pathBox->fontMetrics().maxWidth() * 13);
  prop_line->addWidget(m_pathBox);
  tie(m_pathBox, "Filename", prop_line);

  auto *browseBtn = new QPushButton("Browse");
  connect(browseBtn, SIGNAL(clicked()), this, SLOT(browseClicked()));
  browseBtn->setEnabled(isWidgetEnabled("Filename"));
  prop_line->addWidget(browseBtn);

  main_layout->addLayout(prop_line);

  //------------- OutputWorkspace property ---------------------
  m_wsBox = new QLineEdit;

  prop_line = new QHBoxLayout;
  prop_line->addWidget(new QLabel("Enter name for workspace:"));
  prop_line->addWidget(m_wsBox);
  tie(m_wsBox, "OutputWorkspace", prop_line);
  prop_line->addStretch();
  main_layout->addLayout(prop_line);

  //------------- Spectra properties ---------------------
  auto *groupbox = new QGroupBox("Spectra Options");
  prop_line = new QHBoxLayout;

  auto *text_field = new QLineEdit;
  text_field->setMaximumWidth(m_wsBox->fontMetrics().horizontalAdvance("888888"));
  prop_line->addWidget(new QLabel("Start:"));
  prop_line->addWidget(text_field);
  tie(text_field, "SpectrumMin", prop_line);

  text_field = new QLineEdit;
  text_field->setMaximumWidth(m_wsBox->fontMetrics().horizontalAdvance("888888"));
  prop_line->addWidget(new QLabel("End:"));
  prop_line->addWidget(text_field);
  tie(text_field, "SpectrumMax", prop_line);

  text_field = new QLineEdit;
  prop_line->addWidget(new QLabel("List:"));
  prop_line->addWidget(text_field);
  tie(text_field, "SpectrumList", prop_line);

  prop_line->addStretch();
  groupbox->setLayout(prop_line);
  main_layout->addWidget(groupbox);

  //------------- Period properties ---------------------
  prop_line = new QHBoxLayout;
  text_field = new QLineEdit;
  prop_line->addWidget(new QLabel("Periods:"));
  prop_line->addWidget(text_field);
  prop_line->addStretch();
  tie(text_field, "PeriodList", prop_line);

  main_layout->addLayout(prop_line);

  //------------- Cache option , log files options and Monitors Options
  //---------------------

  const Mantid::Kernel::Property *cacheProp = getAlgorithmProperty("Cache");
  if (cacheProp) {
    auto *cacheBox = new QComboBox;
    std::vector<std::string> items = cacheProp->allowedValues();
    std::vector<std::string>::const_iterator vend = items.end();
    for (std::vector<std::string>::const_iterator vitr = items.begin(); vitr != vend; ++vitr) {
      cacheBox->addItem(QString::fromStdString(*vitr));
    }
    prop_line = new QHBoxLayout;
    prop_line->addWidget(new QLabel("Cache file locally:"), 0, Qt::AlignRight);
    prop_line->addWidget(cacheBox, 0, Qt::AlignLeft);
    tie(cacheBox, "Cache", prop_line);
  }

  prop_line->addStretch();
  // If the algorithm version supports the LoadLog property add a check box for
  // it
  const Mantid::Kernel::Property *loadlogs = getAlgorithmProperty("LoadLogFiles");
  if (loadlogs) {
    QCheckBox *checkbox = new QCheckBox("Load Log Files", this);
    prop_line->addWidget(checkbox);
    tie(checkbox, "LoadLogFiles", prop_line);
  }
  prop_line->addStretch();
  //------------- If the algorithm version supports the LoadMonitors property
  // add a check box for it ----
  const Mantid::Kernel::Property *loadMonitors = getAlgorithmProperty("LoadMonitors");
  if (loadMonitors) {
    auto *monitorsBox = new QComboBox;
    std::vector<std::string> monitoritems = loadMonitors->allowedValues();
    std::vector<std::string>::const_iterator mend = monitoritems.end();
    for (std::vector<std::string>::const_iterator mitr = monitoritems.begin(); mitr != mend; ++mitr) {
      monitorsBox->addItem(QString::fromStdString(*mitr));
    }
    prop_line->addWidget(new QLabel("LoadMonitors:"), 0, Qt::AlignRight);
    prop_line->addWidget(monitorsBox);
    tie(monitorsBox, "LoadMonitors", prop_line);
  }

  if (prop_line)
    main_layout->addLayout(prop_line);

  // Buttons
  main_layout->addLayout(createDefaultButtonLayout("?", "Load", "Cancel"));
}

/**
 * A slot for the browse button "clicked" signal
 */
void LoadRawDialog::browseClicked() {
  if (!m_pathBox->text().isEmpty()) {
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(
        QFileInfo(m_pathBox->text()).absoluteDir().path());
  }

  QString filepath = this->openFileDialog("Filename");
  if (!filepath.isEmpty()) {
    m_pathBox->clear();
    m_pathBox->setText(filepath.trimmed());
  }

  // Add a suggestion for workspace name
  if (m_wsBox->isEnabled() && !filepath.isEmpty())
    m_wsBox->setText(QFileInfo(filepath).baseName());
}
