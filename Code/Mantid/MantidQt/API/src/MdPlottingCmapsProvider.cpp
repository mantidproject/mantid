#include "MantidQtAPI/MdPlottingCmapsProvider.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include <QStringList>
#include <QDir>
#include <fstream>
#include <vector>

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/Exception.h>



namespace MantidQt{
  namespace API{
    namespace
    {
      /// Static logger
      Mantid::Kernel::Logger g_log("MdViewerWidget");
    }

    MdPlottingCmapsProvider::MdPlottingCmapsProvider()
    {
    }

    MdPlottingCmapsProvider::~MdPlottingCmapsProvider()
    {
    }

    void MdPlottingCmapsProvider::getColorMapsForMdPlotting(QStringList& colorMapNames, QStringList& colorMapFiles)
    {
      // Get the installed color maps directory.
      QString colorMapDirectory = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("colormaps.directory"));
      if (colorMapDirectory.isEmpty())
      {
        return;
      }

      // We show only those color maps as options which can be found in the .map files and in the .xml files of the VSI
      QStringList colorMapNamesSliceViewer;
      QStringList colorMapFilesSliceViewer;
      appendAllFileNamesForFileType(colorMapNamesSliceViewer, colorMapFilesSliceViewer, colorMapDirectory, "map");

      QStringList colorMapNamesVsi;
      getColorMapsForVSI(colorMapNamesVsi);

      std::vector<int> indexList = getSliceViewerIndicesForCommonColorMaps(colorMapNamesSliceViewer, colorMapNamesVsi);

      for (std::vector<int>::iterator it = indexList.begin(); it != indexList.end(); ++it)
      {
        colorMapNames.append(colorMapNamesSliceViewer[*it]);
        colorMapFiles.append(colorMapFilesSliceViewer[*it]);
      }
    }

    void MdPlottingCmapsProvider::getColorMapsForVSI(QStringList& colorMapNames)
    {
      // Get the installed color maps directory.
      QString colorMapDirectory = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("colormaps.directory"));
      if (colorMapDirectory.isEmpty())
      {
        return;
      }

      QStringList colormapXMLFiles;
      QStringList colorMapXMLNames;

      // Extract all file names 
      appendAllFileNamesForFileType(colorMapXMLNames, colormapXMLFiles, colorMapDirectory, "xml");

      for (QStringList::iterator it = colormapXMLFiles.begin(); it != colormapXMLFiles.end(); ++it)
      {
        appendVSIColorMaps(colorMapNames, *it);
      }
    }

    void MdPlottingCmapsProvider::appendVSIColorMaps(QStringList& colorMapNames,  QString fullFilePath)
    {
      std::string path = fullFilePath.toStdString();
      std::ifstream input(path.c_str(), std::ifstream::in);

      Poco::XML::InputSource source(input);

      try
      {
        Poco::XML::DOMParser parser;

        Poco::AutoPtr<Poco::XML::Document> doc = parser.parse(&source);

        Poco::XML::Element* root = doc->documentElement();

        // Get all color maps
        Poco::XML::NodeList* nodes = root->getElementsByTagName("ColorMap");

        Poco::XML::Node* node = NULL;

        Poco::XML::Element* element = NULL;

        for (unsigned long i = 0; i < nodes->length(); ++i)
        {
          node = nodes->item(i);
          element = dynamic_cast<Poco::XML::Element*>(node);
          std::string nameOfMap = static_cast<std::string>(element->getAttribute("name"));
          colorMapNames.append(QString(nameOfMap.c_str()));
        }
      }
      catch(Poco::Exception& exc)
      {
        g_log.warning() << "There was an issue with reading color maps:" << exc.displayText() <<"\n";
      }
    }

    void MdPlottingCmapsProvider::appendAllFileNamesForFileType(QStringList& colorMapNames, QStringList& colorMapFiles, QString colorMapDirectory, QString fileType)
    {
      QDir directory(colorMapDirectory);

      QStringList filter(QString("*.%1").arg(fileType));

      QFileInfoList info = directory.entryInfoList(filter, QDir::Files);

      for (QFileInfoList::iterator it = info.begin(); it != info.end(); ++it)
      {
        colorMapNames.append(it->baseName());
        colorMapFiles.append(it->absFilePath());
      }
    }

    std::vector<int> MdPlottingCmapsProvider::getSliceViewerIndicesForCommonColorMaps(QStringList colorMapNamesSliceViewer,QStringList colorMapNamesVsi)
    {
      int index = 0;

      std::vector<int> indexVector;

      for (QStringList::iterator it = colorMapNamesSliceViewer.begin(); it != colorMapNamesSliceViewer.end(); ++it)
      {
        if (colorMapNamesVsi.indexOf(*it) != -1)
        {
          indexVector.push_back(index);
        }

        index++;
      }

      return indexVector;
    }
  }
}