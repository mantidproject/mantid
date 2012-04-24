

class QtMwsRemoteJobManager : public MwsRemoteJobManager
{
  Q_OBJECT

protected:
  virtual bool getPassword(); // Will pop up a Qt dialog box to request a password
};
