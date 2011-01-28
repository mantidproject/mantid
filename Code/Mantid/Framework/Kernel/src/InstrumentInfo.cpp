//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Support.h"

#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/Text.h"

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
    InstrumentInfo::InstrumentInfo(FacilityInfo* f,const Poco::XML::Element* elem)
      :m_facility(f)
    {
      std::string paddingStr = elem->getAttribute("zeropadding");
      if ( paddingStr.empty() || !StrFunc::convert(paddingStr,m_zeroPadding) )
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

      Poco::XML::NodeList* pNL_technique = elem->getElementsByTagName("technique");
      unsigned int n = pNL_technique->length();

      for (unsigned int i = 0; i < n; ++i)
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

    /**
    * Equality operator. Two objects are considered equal if their names, short names and zero padding are the same.
    * @param rhs :: The object to compare this with
    * @returns True if the objects are considered equal, false otherwise
    */
    bool InstrumentInfo::operator==(const InstrumentInfo & rhs) const
    {
      return (this->name() == rhs.name() && this->shortName() == rhs.shortName());
    }

  } // namespace Kernel
} // namespace Mantid
