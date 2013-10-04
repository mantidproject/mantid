//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/CatalogInfo.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>

namespace Mantid
{
  namespace Kernel
  {
    /**
     * Constructor
     * @param element :: "Catalog" element from Facilities.xml
     */
    CatalogInfo::CatalogInfo(const Poco::XML::Element* element)
    {
      m_catalogName   = getAttribute(element, "catalog", "name");
      m_soapEndPoint  = getAttribute(element, "soapendpoint", "url");
      m_catalogPrefix = getAttribute(element, "prefix", "regex");
      m_windowsPrefix = getAttribute(element, "windows", "replacement");
      m_macPrefix     = getAttribute(element, "mac", "replacement");
      m_linuxPrefix   = getAttribute(element, "linux", "replacement");
    }

    /**
     * Obtain catalog name from the facility file.
     */
    std::string CatalogInfo::catalogName()
    {
      return (m_catalogName);
    }

    /**
     * Obtain soap end point from the facility file.
     */
    std::string CatalogInfo::soapEndPoint()
    {
      return (m_soapEndPoint);
    }

    /**
     * Obtain the regex prefix for default archive path.
     */
    std::string CatalogInfo::catalogPrefix()
    {
      return (m_catalogPrefix);
    }

    /**
     * Obtain Windows prefix from the facility file.
     */
    std::string CatalogInfo::windowsPrefix()
    {
      return (m_windowsPrefix);
    }

    /**
     * Obtain Macintosh prefix from facility file.
     */
    std::string CatalogInfo::macPrefix()
    {
      return (m_macPrefix);
    }

    /**
     * Obtain Linux prefix from facility file.
     */
    std::string CatalogInfo::linuxPrefix()
    {
      return (m_linuxPrefix);
    }

    /**
     * Transform's the archive path based on operating system used.
     * @param path :: The archive path from ICAT to perform the transform on.
     */
    std::string CatalogInfo::transformArchivePath(std::string path)
    {
      bool isWindowsPath = false;

      // Check to see if path is a windows path.
      std::size_t pos = path.find("\\");
      if (pos != std::string::npos)
      {
        isWindowsPath = true;
      }

      #ifdef __linux__
        std::string filePath = replacePrefix(path,catalogPrefix(),linuxPrefix());
        boost::replace_all(filePath, "\\", "/");
        return filePath;
      #elif __APPLE__
        std::string filePath = replacePrefix(path,catalogPrefix(),macPrefix());
        boost::replace_all(filePath, "\\", "/");
        return filePath;
      #elif _WIN32
        if (isWindowsPath)
        {
          return path;
        }
        else
        {
          std::string filePath = replacePrefix(path,linuxPrefix(),windowsPrefix());
          boost::replace_all(filePath, "/", "\\");
          return filePath;
        }
      #endif

      return ("");
    }

    /**
     * Replace the content of a string using regex.
     * @param path   :: An string to search and replace on.
     * @param regex  :: The regex to search for.
     * @param prefix :: Replace result of regex with this prefix.
     */
    std::string CatalogInfo::replacePrefix(std::string path, std::string regex, std::string prefix)
    {
      boost::regex re(regex);
      // Assign the result of the replacement back to path and return it.
      path = boost::regex_replace(path, re, prefix);
      return (path);
    }

    /**
     * Replace all occurrences of the search string in the input with the format string.
     * @param path    :: An string to search and replace on.
     * @param search  :: A substring to be searched for.
     * @param format  :: A substitute string.
     */
    void CatalogInfo::replaceInString(std::string path, std::string search, std::string format)
    {
      boost::replace_all(path, search, format);
    }

    /**
     * Obtain the attribute from a given element tag and attribute name.
     * @param tagName :: The name of the tag to search for.
     * @param attributeName :: The name of the attribute for the given tag.
     */
    std::string CatalogInfo::getAttribute(const Poco::XML::Element* element, const std::string &tagName, const std::string &attributeName)
    {
      Poco::XML::NodeList* elementTag = element->getElementsByTagName(tagName);

      // If the tag exists in the XML file.
      if (elementTag->length() == 1)
      {
        Poco::XML::Element* item = dynamic_cast<Poco::XML::Element*>(elementTag->item(0));

        // If the item does exist, then we want to return it.
        if(!item->getAttribute(attributeName).empty())
        {
          elementTag->release();
          return (item->getAttribute(attributeName));
        }
      }
      return ("");
    }

  } // namespace Kernel
} // namespace Mantid
