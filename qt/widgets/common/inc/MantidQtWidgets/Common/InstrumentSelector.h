// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_INSTRUMENTSELECTOR_H_
#define MANTIDQTMANTIDWIDGETS_INSTRUMENTSELECTOR_H_

#include "DllOption.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/NObserver.h>
#include <QComboBox>
#include <QStringList>

namespace Mantid {
namespace Kernel {
class FacilityInfo;
}
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {
/**
This class defines a widget for selecting an instrument known to Mantid

@author Martyn Gigg, Tessella Support Services plc
@date 10/08/2010
*/
class EXPORT_OPT_MANTIDQT_COMMON InstrumentSelector : public QComboBox {
  Q_OBJECT

  Q_PROPERTY(QStringList techniques READ getTechniques WRITE setTechniques)
  Q_PROPERTY(bool updateOnFacilityChange READ getAutoUpdate WRITE setAutoUpdate)
  Q_PROPERTY(QString facility READ getFacility WRITE setFacility)

public:
  /// Default Constructor
  InstrumentSelector(QWidget *parent = nullptr, bool init = true);
  /// Destructor
  ~InstrumentSelector() override;
  /// Return the list of techniques
  QStringList getTechniques() const;
  /// Set the list of techniques
  void setTechniques(const QStringList &techniques);
  /// Returns true of auto reloading on facility change is enabled
  bool getAutoUpdate();
  /// Enable or disable reloading on facility change
  void setAutoUpdate(bool autoUpdate);
  /// Get the name of the facility instrumetns are currently loaded from
  QString getFacility();
  /// Load instruments from a given facility
  void setFacility(const QString &facilityName);
  /// Sets whether to update the default instrument on selection change
  void updateInstrumentOnSelection(const bool storeChanges);

public slots:
  /// Update list for a new facility
  void fillWithInstrumentsFromFacility(const QString &name = QString());

signals:
  /// Indicate that the instrument selection has changed. The parameter will
  /// contain the new name
  void instrumentSelectionChanged(const QString & /*_t1*/);
  void configValueChanged(const QString & /*_t1*/, const QString & /*_t2*/,
                          const QString & /*_t3*/);
  /// Signals that the list of instruments has been updated
  void instrumentListUpdated();

private slots:
  /// Handle an instrument seelction
  void updateInstrument(const QString &name);

private:
  void handleConfigChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);
  /// Filter the list to only show those supporting the given technique
  void
  filterByTechniquesAtFacility(const QStringList &techniques,
                               const Mantid::Kernel::FacilityInfo &facility);

private: // members
  /// Poco Observer for Config Service Notifications
  Poco::NObserver<InstrumentSelector,
                  Mantid::Kernel::ConfigValChangeNotification>
      m_changeObserver;
  /// A list of technqiues. Only those instruments supporting these techniques
  /// are shown.
  QStringList m_techniques;
  /// The current facility
  const Mantid::Kernel::FacilityInfo *m_currentFacility;
  /// Should the object be initialized
  bool m_init;
  /// Should the default instrument be changed when the selection changes
  bool m_storeChanges;
  /// If the instrument list should be reloaded when the facility changes
  bool m_updateOnFacilityChange;
  /// The last selected instrument
  QString m_selectedInstrument;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_INSTRUMENTSELECTOR_H_
