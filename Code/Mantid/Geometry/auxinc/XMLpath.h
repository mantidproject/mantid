#ifndef XMLpath_h
#define XMLpath_h


namespace XML
{


class XMLpath
{
 private: 
  
  std::string Key;
  int cnt;

 public:
 
  XMLpath();
  XMLpath(const XMLpath&);
  XMLpath& operator=(const XMLpath&);
  ~XMLpath();

  
};

};

#endif
