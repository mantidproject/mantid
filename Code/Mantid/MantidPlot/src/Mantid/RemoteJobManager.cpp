#include "RemoteJobManager.h"
#include "MantidKernel/ConfigService.h"

#include <sstream>
using namespace std;



void RemoteJobManager::saveProperties( int itemNum)
{
    Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
    ostringstream tempStr;
    string prefix("Cluster.");
    tempStr << itemNum;
    prefix += tempStr.str() + string(".");

    config.setString( prefix + string( "DisplayName"), m_displayName);
    config.setString( prefix + string( "ConfigFileUrl"), m_configFileUrl);
}


void MwsRemoteJobManager::saveProperties( int itemNum)
{
    HttpRemoteJobManager::saveProperties( itemNum);

    Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
    ostringstream tempStr;
    string prefix("Cluster.");
    tempStr << itemNum;
    prefix += tempStr.str() + string(".");

    config.setString( prefix + string( "Type"), getType());
    config.setString( prefix + string( "ServiceBaseUrl"), m_serviceBaseUrl);
    config.setString( prefix + string( "UserName"), m_userName);
}



// On success, creates a new object and returns pointer to it.  On failure,
// returns NULL
RemoteJobManager *RemoteJobManagerFactory::createFromProperties( int itemNum)
{
    // All the properties should start with the key "Cluster", followed by a key for their
    // item number, followed by the keys remaining keys they need.  ie: Cluster.0.DisplayName

    Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
    ostringstream tempStr;
    string prefix("Cluster.");
    tempStr << itemNum;
    prefix += tempStr.str();

    std::vector< std::string> keys = config.getKeys( prefix);

    if (keys.size() == 0)
        return NULL;

    // Need a key for Type and it must have a value we recognize
    if (find( keys.begin(), keys.end(), std::string( "Type")) == keys.end())
        return NULL;

    std::string managerType;
    config.getValue( prefix + std::string(".Type"), managerType);

    if ( managerType == "MWS") return RemoteJobManagerFactory::createMwsManager( itemNum);
    // else if.....
    // else if.....


    // Type not recognized
    return NULL;

}


MwsRemoteJobManager *RemoteJobManagerFactory::createMwsManager( int itemNum)
{
    // There's 4 values that we need:  ConfigFileUrl, DisplayName, ServiceBaseUrl and UserName

    Mantid::Kernel::ConfigServiceImpl& config = Mantid::Kernel::ConfigService::Instance();
    ostringstream tempStr;
    string prefix("Cluster.");
    tempStr << itemNum;
    prefix += tempStr.str();

    std::vector< std::string> keys = config.getKeys( prefix);

    if (keys.size() == 0)
        return NULL;

    if (find( keys.begin(), keys.end(), std::string( "ConfigFileUrl")) == keys.end())
        return NULL;
    if (find( keys.begin(), keys.end(), std::string( "DisplayName")) == keys.end())
        return NULL;
    if (find( keys.begin(), keys.end(), std::string( "ServiceBaseUrl")) == keys.end())
        return NULL;
    if (find( keys.begin(), keys.end(), std::string( "UserName")) == keys.end())
        return NULL;


    std::string configFileUrl, displayName, serviceBaseUrl, userName;
    config.getValue( prefix + std::string(".ConfigFileUrl"), configFileUrl);
    config.getValue( prefix + std::string(".DisplayName"), displayName);
    config.getValue( prefix + std::string(".ServiceBaseUrl"), serviceBaseUrl);
    config.getValue( prefix + std::string(".UserName"), userName);

    // Do some quick sanity checks on the values...
    if (configFileUrl.length() == 0)
        return NULL;
    if (displayName.length() == 0)
        return NULL;
    if (serviceBaseUrl.length() == 0)
        return NULL;
    if (userName.length() == 0)
        return NULL;

    // Validation checks passed.  Create the object
    return new MwsRemoteJobManager( displayName, configFileUrl, serviceBaseUrl, userName);
}
