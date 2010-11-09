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

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
    */
    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS InstrumentSelector : public QComboBox
    {
      Q_OBJECT
      Q_PROPERTY(QStringList techniques READ getTechniques WRITE setTechniques)

    public:
      /// Default Constructor
      InstrumentSelector(QWidget *parent = NULL, bool init = true);
      /// Destructor
      virtual ~InstrumentSelector();
      /// Return the list of techniques
      QStringList getTechniques() const;
      /// Set the list of techniques
      void setTechniques(const QStringList & techniques);

    public slots:
      /// Update list for a new facility
      void fillWithInstrumentsFromFacility(const QString & name = QString());

    signals:
      /// Indicate that the instrument selection has changed. The parameter will contain the new name
      void instrumentSelectionChanged(const QString &);
      void configValueChanged(const QString&, const QString&, const QString&);

    private slots:
      /// Update Mantid's default instrument
      void updateDefaultInstrument(const QString & name) const;

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
      bool m_init;
    };

  }
}

#endif //MANTIDQTMANTIDWIDGETS_INSTRUMENTSELECTOR_H_