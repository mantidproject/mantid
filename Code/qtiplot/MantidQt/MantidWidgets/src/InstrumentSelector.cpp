//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidQtMantidWidgets/InstrumentSelector.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

namespace MantidQt
{
  namespace MantidWidgets
  {
     using namespace Mantid::Kernel;

    //------------------------------------------------------
    // Public member functions
    //------------------------------------------------------

    /**
     * Default constructor
     * @param parent A widget to act as this widget's parent (default = NULL)
     * @param init If true then the widget will be populated with the instrument list (default = true)
     */
    InstrumentSelector::InstrumentSelector(QWidget *parent, bool init) : QComboBox(parent)
    {
      if( init )
      {
        fillFromFacility();
        connect(this, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(updateDefaultInstrument(const QString &)));
        connect(this, SIGNAL(currentIndexChanged(const QString &)), this, SIGNAL(instrumentSelectionChanged(const QString &)));
      }
    }

    //------------------------------------------------------
    // Public slot member functions
    //------------------------------------------------------

     /**
      * Populate list with instruments from the named facility. Note the current list is cleared.
      * @param name The name of the facility whose instruments should be placed in the list. An empty string uses the default
      * facility defined in Mantid.
      */
    void InstrumentSelector::fillFromFacility(const QString & name)
    {
      ConfigServiceImpl & mantidSettings = ConfigService::Instance(); 
      
      clear();
      const std::vector<InstrumentInfo> & instruments = mantidSettings.Facility(name.toStdString()).Instruments();
      std::vector<InstrumentInfo>::const_iterator iend = instruments.end();
      for( std::vector<InstrumentInfo>::const_iterator itr = instruments.begin(); itr != iend; ++itr )
      {
        QString name = QString::fromStdString(itr->name());
        QString shortName = QString::fromStdString(itr->shortName());
        this->addItem(name, QVariant(shortName));
      }

      // Set the correct default
      QString defaultName = QString::fromStdString(mantidSettings.Facility().Instrument().name());
      int index = this->findText(defaultName);
      if( index >= 0 )
      {
        this->setCurrentIndex(index);
      }
    }

    //------------------------------------------------------
    // Privte slot member functions
    //------------------------------------------------------
    /**
     * Set the named instrument as the default for Mantid  
     * @param name A string containing the new instrument to set as the default 
     */
    void InstrumentSelector::updateDefaultInstrument(const QString & name) const
    {
      if( !name.isEmpty() )
      {
        ConfigService::Instance().setString("default.instrument", name.toStdString());
      }
    }

  }
}