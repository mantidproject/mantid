//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"

#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>

#include <algorithm>

namespace Mantid
{
  namespace Kernel
  {

    Logger& InstrumentInfo::g_log = Logger::get("InstrumentInfo");

    /** Constructor.
    * @param f :: The facility
    * @param elem :: The Poco::XML::Element to read the data from
    * @throw std::runtime_error if name or at least one technique are not defined
    */
    InstrumentInfo::InstrumentInfo(const FacilityInfo* f,const Poco::XML::Element* elem)
      : m_facility(f), m_liveListener(), m_liveDataAddress()
    {
      std::string paddingStr = elem->getAttribute("zeropadding");
      if ( paddingStr.empty() || !Mantid::Kernel::Strings::convert(paddingStr,m_zeroPadding) )
      {
        m_zeroPadding = f->zeroPadding();
      }

      m_name = elem->getAttribute("name");
      if (m_name.empty())
      {
        g_log.error("Instrument name is not defined");
        throw std::runtime_error("Instrument name is not defined");
      }

      // The string to separate the instrument name and the run number.
      m_delimiter = elem->getAttribute("delimiter");
      if (m_delimiter.empty())
      {
        m_delimiter = f->delimiter();
      }

      m_shortName = elem->getAttribute("shortname");
      if (m_shortName.empty())
      {
        m_shortName = m_name;
      }

      fillTechniques(elem);
      fillLiveData(elem);
    }

    /**
    * Equality operator. Two objects are considered equal if their names and short names are the same.
    * @param rhs :: The object to compare this with
    * @returns True if the objects are considered equal, false otherwise
    */
    bool InstrumentInfo::operator==(const InstrumentInfo & rhs) const
    {
      return (this->name() == rhs.name() && this->shortName() == rhs.shortName());
    }

    /// Returns the default delimiter between instrument name and run number
    std::string InstrumentInfo::delimiter() const
    {
      return m_delimiter;
    }

    /// Return the name of the instrument
    const std::string InstrumentInfo::name() const
    {
      return m_name;
    }

    /// Return the short name of the instrument
    const std::string InstrumentInfo::shortName() const
    {
      return m_shortName;
    }

    /// Returns zero padding for this instrument
    int InstrumentInfo::zeroPadding() const
    {
      return m_zeroPadding;
    }

    /// Returns the name of the live listener
    const std::string & InstrumentInfo::liveListener() const
    {
      return m_liveListener;
    }

    /** Returns the host & port to connect to for a live data stream
     *  No guarantees are given that the provided string is well-formed and valid
     *    - the caller should check this themselves
     */
    const std::string & InstrumentInfo::liveDataAddress() const
    {
      return m_liveDataAddress;
    }

    /// Return list of techniques
    const std::set<std::string>& InstrumentInfo::techniques() const
    {
      return m_technique;
    }

    /// Return the facility
    const FacilityInfo& InstrumentInfo::facility() const
    {
      return *m_facility;
    }

    /// Called from constructor to fill live listener name
    void InstrumentInfo::fillTechniques(const Poco::XML::Element* elem)
    {
      Poco::XML::NodeList* pNL_technique = elem->getElementsByTagName("technique");
      unsigned long n = pNL_technique->length();

      for (unsigned long i = 0; i < n; ++i)
      {
        Poco::XML::NodeList* pNL = pNL_technique->item(i)->childNodes();
        if (pNL->length() > 0)
        {
          Poco::XML::Text* txt = dynamic_cast<Poco::XML::Text*>(pNL->item(0));
          if (txt)
          {
            std::string tech = txt->getData();
            if ( !tech.empty() )
            {
              m_technique.insert(tech);
            }
          }
        }
        pNL->release();
      }
      pNL_technique->release();

      if (m_technique.empty())
      {
        g_log.error("No technique is defined for instrument "+m_name);
        throw std::runtime_error("No technique is defined for instrument "+m_name);
      }
    }

    /// Called from constructor to fill live listener name
    void InstrumentInfo::fillLiveData(const Poco::XML::Element* elem)
    {
      // Get the first livedata element (will be NULL if there's none)
      Poco::XML::Element * live = elem->getChildElement("livedata");
      if ( live )
      {
        // Get the name of the listener - empty string will be returned if missing
        m_liveListener = live->getAttribute("listener");
        // Get the host+port. Would have liked to put straight into a Poco::Net::SocketAddress
        // but that tries to contact the given address on construction, which won't always be possible (or scalable)
        m_liveDataAddress = live->getAttribute("address");
        // Warn rather than throw if there are problems with the address
        if ( m_liveDataAddress.empty() )
        {
          g_log.warning() << "No connection details specified for live data listener of " << m_name << "\n";
        }
        // Check for a colon, which would suggest that a host & port are present
        else if ( m_liveDataAddress.find(":") == std::string::npos )
        {
          g_log.warning() << "Live data address for " << m_name << " appears not to have both host and port specified.\n";
        }
      }
      // Apply the facility default listener if none specified for this instrument
      if ( m_liveListener.empty() )
      {
        m_liveListener = m_facility->liveListener();
      }
    }

    //-------------------------------------------------------------------------
    // Non-member functions
    //-------------------------------------------------------------------------
    /**
     * Prints the instrument name to the stream
     * @param buffer :: A reference to an output stream
     * @param instrumentDescriptor :: A reference to an InstrumentInfo object
     * @return A reference to the stream written to
     */
    std::ostream & operator<<(std::ostream & buffer, const InstrumentInfo & instrumentDescriptor)
    {
      buffer << instrumentDescriptor.name();
      return buffer;
    }

  } // namespace Kernel
} // namespace Mantid
