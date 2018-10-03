// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidQtWidgets/Common/InstrumentSelector.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"

#include <QMessageBox>

#include <Poco/AutoPtr.h>
#include <Poco/NObserver.h>
#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>
#include <set>

namespace {
Mantid::Kernel::Logger g_log("InstrumentSelector");
}

namespace MantidQt {
namespace MantidWidgets {
using namespace Mantid::Kernel;

//------------------------------------------------------
// Public member functions
//------------------------------------------------------

/**
 * Default constructor
 * @param parent :: A widget to act as this widget's parent (default = NULL)
 * @param init :: If true then the widget will be populated with the instrument
 * list (default = true)
 */
InstrumentSelector::InstrumentSelector(QWidget *parent, bool init)
    : QComboBox(parent),
      m_changeObserver(*this, &InstrumentSelector::handleConfigChange),
      m_techniques(), m_currentFacility(nullptr), m_init(init),
      m_storeChanges(false), m_updateOnFacilityChange(true),
      m_selectedInstrument() {
  setEditable(false);

  if (init) {
    fillWithInstrumentsFromFacility();

    Mantid::Kernel::ConfigServiceImpl &config =
        Mantid::Kernel::ConfigService::Instance();
    config.addObserver(m_changeObserver);
  }

  connect(this, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(updateInstrument(const QString &)));
}

/**
 * Destructor for InstrumentSelector
 * De-subscribes this object from the Poco NotificationCentre
 */
InstrumentSelector::~InstrumentSelector() {
  if (m_init) {
    Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
  }
}

/**
 * Return the list of techniques that are supported by the instruments in the
 * widget
 * @returns A list of supported techniques
 */
QStringList InstrumentSelector::getTechniques() const { return m_techniques; }

/**
 * Returns if the list of instruments will be refeshed when the facility
 * changes.
 * @return If facility auto update is enabled
 */
bool InstrumentSelector::getAutoUpdate() { return m_updateOnFacilityChange; }

/**
 * Sets if the list of instruments should be updated folowing a facility change.
 * @param autoUpdate If instruments are to be updated
 */
void InstrumentSelector::setAutoUpdate(bool autoUpdate) {
  m_updateOnFacilityChange = autoUpdate;
}

/**
 * Set the list of techniques
 * @param techniques :: Only those instruments that support these techniques
 * will be shown
 */
void InstrumentSelector::setTechniques(const QStringList &techniques) {
  m_techniques = techniques;
  if (count() > 0 && m_currentFacility) {
    filterByTechniquesAtFacility(techniques, *m_currentFacility);
  }
}

/**
 * Returns the name of the facility from which instruments are listed.
 * @return Facility name
 */
QString InstrumentSelector::getFacility() {
  return QString::fromStdString(m_currentFacility->name());
}

/**
 * Sets the facility instruments should be loaded from and refeshes the list.
 * @param facilityName Name of facilty
 */
void InstrumentSelector::setFacility(const QString &facilityName) {
  fillWithInstrumentsFromFacility(facilityName);
}

void InstrumentSelector::handleConfigChange(
    Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
  if (!m_updateOnFacilityChange)
    return;

  QString prop = QString::fromStdString(pNf->key());
  QString newV = QString::fromStdString(pNf->curValue());
  QString oldV = QString::fromStdString(pNf->preValue());

  if (newV != oldV) {
    if ((prop == "default.facility") &&
        (newV != QString::fromStdString(m_currentFacility->name()))) {
      fillWithInstrumentsFromFacility(newV);
    } else if ((prop == "default.instrument") &&
               (newV != this->currentText())) {
      this->setCurrentIndex(this->findText(newV));
    }
  }
}

//------------------------------------------------------
// Public slot member functions
//------------------------------------------------------

/**
 * Populate list with instruments from the named facility. Note the current list
 * is cleared.
 * @param name :: The name of the facility whose instruments should be placed in
 * the list. An empty string uses the default
 * facility defined in Mantid.
 */
void InstrumentSelector::fillWithInstrumentsFromFacility(const QString &name) {
  ConfigServiceImpl &mantidSettings = ConfigService::Instance();

  this->blockSignals(true);
  this->clear();

  try {
    if (name.isEmpty()) {
      m_currentFacility = &(mantidSettings.getFacility());
    } else {
      m_currentFacility = &(mantidSettings.getFacility(name.toStdString()));
    }
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    // could not find the facility
    // pick the first facility from the valid list
    m_currentFacility =
        &(mantidSettings.getFacility(mantidSettings.getFacilityNames()[0]));
  }

  const auto &instruments = m_currentFacility->instruments();
  std::set<std::string> alphabetizedNames;
  for (auto itr = instruments.cbegin(); itr != instruments.cend(); ++itr) {
    alphabetizedNames.insert(itr->name());
  }
  for (const auto &name_std_str : alphabetizedNames) {
    QString name = QString::fromStdString(name_std_str);
    std::string prefix =
        m_currentFacility->instrument(name_std_str).shortName();
    QString shortName = QString::fromStdString(prefix);
    this->addItem(name, QVariant(shortName));
  }
  filterByTechniquesAtFacility(m_techniques, *m_currentFacility);

  QString defaultName;
  try {
    defaultName =
        QString::fromStdString(m_currentFacility->instrument().name());
  } catch (Exception::NotFoundError &) {
    defaultName = "";
  }
  int index = this->findText(defaultName);
  if (index < 0) {
    index = 0;
  }

  // Don't affect the default instrument
  this->setCurrentIndex(index);
  this->blockSignals(false);

  emit instrumentListUpdated();
  updateInstrument(this->currentText());
}

/**
 * Sets whether to update the default instrument on selection change
 * @param storeChanges :: True = store change on selection change
 */
void InstrumentSelector::updateInstrumentOnSelection(const bool storeChanges) {
  m_storeChanges = storeChanges;
}

//------------------------------------------------------
// Private slot member functions
//------------------------------------------------------
/**
 * Handle an instrument being selected from the drop down.
 * Set the named instrument as the default for Mantid if desired and emit
 * the signal if the new instrument is different to that which was previously
 * selected.
 * @param name :: A string containing the new instrument to set as the default
 */
void InstrumentSelector::updateInstrument(const QString &name) {
  // If enabled, set instrument default
  if (!name.isEmpty() && m_storeChanges) {
    ConfigService::Instance().setString("default.instrument",
                                        name.toStdString());
  }

  // If this instrument is different emit the changed signal
  if (name != m_selectedInstrument) {
    m_selectedInstrument = name;
    g_log.debug() << "New instrument selected: "
                  << m_selectedInstrument.toStdString() << '\n';
    emit instrumentSelectionChanged(m_selectedInstrument);
  }
}

//------------------------------------------------------
// Private non-slot member functions
//------------------------------------------------------

/**
 * Filter the list to only show those supporting the given technique
 * @param techniques :: A string list containing the names of a techniques to
 * filter the instrument list by
 * @param facility :: A FacilityInfo object
 */
void InstrumentSelector::filterByTechniquesAtFacility(
    const QStringList &techniques,
    const Mantid::Kernel::FacilityInfo &facility) {
  if (techniques.isEmpty())
    return;

  this->blockSignals(true);

  QStringList supportedInstruments;
  QStringListIterator techItr(techniques);
  while (techItr.hasNext()) {
    const std::vector<InstrumentInfo> instruments =
        facility.instruments(techItr.next().toStdString());
    const size_t nInstrs = instruments.size();
    for (size_t i = 0; i < nInstrs; ++i) {
      supportedInstruments.append(
          QString::fromStdString(instruments[i].name()));
    }
  }

  // Remove those not supported
  for (int i = 0; i < this->count();) {
    if (!supportedInstruments.contains(itemText(i))) {
      removeItem(i);
    } else {
      ++i;
    }
  }

  this->blockSignals(false);

  emit instrumentListUpdated();
}

} // namespace MantidWidgets
} // namespace MantidQt
