#include "MantidQtAPI/MantidHelpInterface.h"
#include <QString>
#include <QUrl>

using namespace MantidQt::API;
using std::string;

MantidHelpInterface::MantidHelpInterface()
{}

MantidHelpInterface::~MantidHelpInterface()
{}

void MantidHelpInterface::showPage(const std::string & url)
{
  UNUSED_ARG(url);
}

void MantidHelpInterface::showPage(const QString & url)
{
  UNUSED_ARG(url);
}

void MantidHelpInterface::showPage(const QUrl & url)
{
  UNUSED_ARG(url);
}

void MantidHelpInterface::showWikiPage(const std::string &page)
{
  UNUSED_ARG(page);
}

void MantidHelpInterface::showConcept(const std::string &page)
{
  UNUSED_ARG(page);
}

void MantidHelpInterface::showConcept(const QString &page)
{
  UNUSED_ARG(page);
}

void MantidHelpInterface::showAlgorithm(const std::string &name, const int version)
{
  UNUSED_ARG(name);
  UNUSED_ARG(version);
}

void MantidHelpInterface::showAlgorithm(const QString &name, const int version)
{
  UNUSED_ARG(name);
  UNUSED_ARG(version); }

void MantidHelpInterface::showFitFunction(const std::string &name)
{
  UNUSED_ARG(name);
}

void MantidHelpInterface::showCustomInterface(const std::string &name)
{
  UNUSED_ARG(name);
}

void MantidHelpInterface::showCustomInterface(const QString &name)
{
  UNUSED_ARG(name);
}

void MantidHelpInterface::shutdown()
{
}
