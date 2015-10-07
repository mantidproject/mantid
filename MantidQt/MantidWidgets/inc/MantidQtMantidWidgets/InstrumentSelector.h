#ifndef MANTIDQTMANTIDWIDGETS_INSTRUMENTSELECTOR_H_
#define MANTIDQTMANTIDWIDGETS_INSTRUMENTSELECTOR_H_

#include "WidgetDllOption.h"
#include <QComboBox>
#include <QStringList>
#include "MantidKernel/ConfigService.h"

#include <Poco/AutoPtr.h>
#include <Poco/NObserver.h>

//----------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------
namespace Mantid
{
  namespace Kernel
  {
    class FacilityInfo;
  }
}

namespace MantidQt
{
  namespace MantidWidgets
  {
    /**
    This class defines a widget for selecting an instrument known to Mantid

    @author Martyn Gigg, Tessella Support Services plc
    @date 10/08/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS InstrumentSelector : public QComboBox
    {
      Q_OBJECT

      Q_PROPERTY(QStringList techniques READ getTechniques WRITE setTechniques)
      Q_PROPERTY(bool updateOnFacilityChange READ getAutoUpdate WRITE setAutoUpdate)
      Q_PROPERTY(QString facility READ getFacility WRITE setFacility)

    public:
      /// Default Constructor
      InstrumentSelector(QWidget *parent = NULL, bool init = true);
      /// Destructor
      virtual ~InstrumentSelector();
      /// Return the list of techniques
      QStringList getTechniques() const;
      /// Set the list of techniques
      void setTechniques(const QStringList & techniques);
      /// Returns true of auto reloading on facility change is enabled
      bool getAutoUpdate();
      /// Enable or disable reloading on facility change
      void setAutoUpdate(bool autoUpdate);
      /// Get the name of the facility instrumetns are currently loaded from
      QString getFacility();
      /// Load instruments from a given facility
      void setFacility(const QString & facilityName);
      /// Sets whether to update the default instrument on selection change
      void updateInstrumentOnSelection(const bool storeChanges);

    public slots:
      /// Update list for a new facility
      void fillWithInstrumentsFromFacility(const QString & name = QString());

    signals:
      /// Indicate that the instrument selection has changed. The parameter will contain the new name
      void instrumentSelectionChanged(const QString &);
      void configValueChanged(const QString&, const QString&, const QString&);
      /// Signals that the list of instruments has been updated
      void instrumentListUpdated();

    private slots:
      /// Handle an instrument seelction
      void updateInstrument(const QString & name);

    private:
      void handleConfigChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);
      /// Filter the list to only show those supporting the given technique
      void filterByTechniquesAtFacility(const QStringList & techniques, const Mantid::Kernel::FacilityInfo & facility);

    private: // members
      /// Poco Observer for Config Service Notifications
      Poco::NObserver<InstrumentSelector, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver;
      /// A list of technqiues. Only those instruments supporting these techniques are shown.
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

  }
}

#endif //MANTIDQTMANTIDWIDGETS_INSTRUMENTSELECTOR_H_
