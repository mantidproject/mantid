//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/CatalogInfo.h"

#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>

using Poco::XML::Element;

namespace Mantid
{
  namespace Kernel
  {
    /**
     * Obtain catalog name from the facility file.
     */
    std::string CatalogInfo::catalogName(const Poco::XML::Element* element)
    {

    }

    /**
     * Obtain soap end point from the facility file.
     */
    std::string CatalogInfo::soapEndPoint(const Poco::XML::Element* element)
    {

    }

    /**
     * Obtain Windows prefix from the facility file.
     */
    std::string CatalogInfo::windowsPrefix(const Poco::XML::Element* element)
    {

    }

    /**
     * Obtain Macintosh prefix from facility file.
     */
    std::string CatalogInfo::macPrefix(const Poco::XML::Element* element)
    {

    }

    /**
     * Obtain Linux prefix from facility file.
     */
    std::string CatalogInfo::linuxPrefix(const Poco::XML::Element* element)
    {

    }

    /**
     * Transform's the archive path based on operating system used.
     * @param path :: The archive path from ICAT to perform the transform on.
     */
    std::string CatalogInfo::transformArchivePath(std::string& path)
    {

    }

    /**
     * Replace // with \\ for Unix compatibility.
     * @param path :: The path to replace slashes on.
     */
    void CatalogInfo::replaceBackwardSlash(std::string& path)
    {

    }

  } // namespace Kernel
} // namespace Mantid
