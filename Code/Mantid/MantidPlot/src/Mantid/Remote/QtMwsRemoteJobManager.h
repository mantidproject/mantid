
#ifndef QTMWSREMOTEJOBMANAGER_H
#define QTMWSREMOTEJOBMANAGER_H

#include "RemoteJobManager.h"

class QtMwsRemoteJobManager : public MwsRemoteJobManager
{
public:
  QtMwsRemoteJobManager( const Poco::XML::Element* elem)
    : MwsRemoteJobManager( elem)  { }

  ~QtMwsRemoteJobManager() { }

protected:
  virtual bool getPassword(); // Will pop up a Qt dialog box to request a password
};


#endif // QTMWSREMOTEJOBMANAGER_H

