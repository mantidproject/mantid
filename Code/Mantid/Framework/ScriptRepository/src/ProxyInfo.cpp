#include "MantidScriptRepository/ProxyInfo.h"
#include <stdexcept>

namespace Mantid
{
  namespace ScriptRepository
  {

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    ProxyInfo::~ProxyInfo()
    {
    }

//----------------------------------------------------------------------------------------------
    /** Constructor
     */
    ProxyInfo::ProxyInfo() :
        m_host(""), m_port(0), m_isHttpProxy(false), m_isEmptyProxy(true)
    {
    }

    /**
     * Proxy information constructor
     * @param host : host url
     * @param port : port number
     * @param isHttpProxy : is this a http proxy
     */
    ProxyInfo::ProxyInfo(const std::string& host, const int port, const bool isHttpProxy) :
        m_host(host), m_port(port), m_isHttpProxy(isHttpProxy), m_isEmptyProxy(false)
    {
    }

    /**
     * Host url
     * @return host url or throws if an unset proxy.
     */
    std::string ProxyInfo::host() const
    {
      if (m_isEmptyProxy)
      {
        throw std::logic_error("Calling host on an undefined proxy");
      }
      return m_host;
    }

    /**
     * Port Number
     * @return Port number or throws if an unset proxy.
     */
    int ProxyInfo::port() const
    {
      if (m_isEmptyProxy)
      {
        throw std::logic_error("Calling port on an undefined proxy");
      }
      return m_port;
    }

    /**
     * Is this a http proxy
     * @return True if a http proxy or throws if an unset proxy.
     */
    bool ProxyInfo::isHttpProxy() const
    {
      return m_isHttpProxy;
    }

    /**
     *
     * @return True if an empty proxy.
     */
    bool ProxyInfo::emptyProxy() const
    {
      return m_isEmptyProxy;
    }

    /**
     * Copy constructor
     * @param other : to copy from
     */
    ProxyInfo::ProxyInfo(const ProxyInfo& other) :
        m_host(other.host()), m_isEmptyProxy(other.emptyProxy()), m_isHttpProxy(other.isHttpProxy()), m_port(
            other.port())
    {

    }

    /**
     * Assignment operator
     * @param other : to assign from.
     * @return
     */
    ProxyInfo& ProxyInfo::operator=(const ProxyInfo& other)
    {
      if (&other != this)
      {
        m_host = other.host();
        m_isEmptyProxy = other.emptyProxy();
        m_isHttpProxy = other.isHttpProxy();
        m_port = other.port();
      }
      return *this;
    }

  } // namespace ScriptRepository
} // namespace Mantid
