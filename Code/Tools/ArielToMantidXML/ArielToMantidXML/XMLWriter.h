#ifndef XMLWRITER_H_
#define XMLWRITER_H_

#include <string>
#include <fstream>

class Component;

class XMLWriter
{
public:
  XMLWriter(const std::string& name, Component* const startPoint);
  virtual ~XMLWriter(void);

  void writeDetectors();

private:
  void writeDefaults();
  void writeSourceSample();

  std::ofstream m_outputFile;
  Component* const m_startPoint;
};

#endif
