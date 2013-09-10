
#include <iostream>

#include <qapplication.h>                                                       
#include <QMainWindow>
#include <QtGui>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidAPI/IEventWorkspace.h"

#include "MantidQtSpectrumViewer/MatrixWSImageView.h"

using namespace MantidQt;
using namespace SpectrumView;
using namespace Mantid::Kernel;
using namespace Mantid::API;

int main( int argc, char** argv )
{
  std::cout << "Start of ImageViewNxEventFile..." << std::endl;
  if ( argc < 2 )
  {
    std::cout << "Please enter a NeXus event file name on the command line!" 
              << std::endl;
    return 0; 
  }
  std::string file_name(argv[1]);

  QApplication a( argc, argv );

  Mantid::API::FrameworkManager::Instance();
  IAlgorithm_sptr ld = AlgorithmManager::Instance().createUnmanaged("LoadEventNexus");
  ld->initialize();

  ld->setPropertyValue("Filename", file_name );
  std::string outws_name = "EventWS";
  ld->setPropertyValue("OutputWorkspace",outws_name);
  ld->setPropertyValue("Precount", "0");

  std::cout << "Loading file: " << file_name << std::endl;
  ld->execute();
  ld->isExecuted();

  std::cout << "File Loaded, getting workspace. " << std::endl;

  IEventWorkspace_sptr WS;
  WS = AnalysisDataService::Instance().retrieveWS<IEventWorkspace>(outws_name);

  std::cout << "Got EventWorkspace, making EventWSDataSource..." << std::endl;

  MantidQt::SpectrumView::MatrixWSImageView image_view( WS );

  return a.exec();
}

