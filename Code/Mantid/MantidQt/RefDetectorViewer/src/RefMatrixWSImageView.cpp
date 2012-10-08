#include <iostream>
#include "MantidQtRefDetectorViewer/RefMatrixWSImageView.h"
#include "MantidQtRefDetectorViewer/RefMatrixWSDataSource.h"
#include "MantidQtRefDetectorViewer/RefArrayDataSource.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidAPI/IEventWorkspace.h"

using Mantid::API::MatrixWorkspace_sptr;
using namespace MantidQt;
using namespace RefDetectorViewer;
using Mantid::API::WorkspaceProperty;
using Mantid::API::Algorithm;
using namespace Mantid::Kernel;
using namespace Mantid::API;

/**
 * Construct an ImageView for the specified matrix workspace
 */
RefMatrixWSImageView::RefMatrixWSImageView( MatrixWorkspace_sptr mat_ws )
{
  RefMatrixWSDataSource* source = new RefMatrixWSDataSource( mat_ws );
  image_view = new RefImageView( source );  // this is the QMainWindow
                                         // for the viewer.  It is
                                         // deleted when the window
                                         // is closed
}

RefMatrixWSImageView::RefMatrixWSImageView( QString wps_name)
{

    IEventWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<IEventWorkspace>(wps_name.toStdString());
    
    double total_ymin = 0;
    double total_ymax = 255;

    std::vector<double> xaxis = ws->readX(0);
    size_t sz = xaxis.size();
    
    double total_xmin = xaxis[0];
    double total_xmax = xaxis[sz-1];
    
    float *data = new float[size_t(total_ymax * sz)];
    
    for (int px=0; px<=total_xmax; px++)
    {
        //retrieve data now
        std::vector<double> yaxis = ws->readY(px);
        for (int tof=0; tof<=sz; tof++)
        {
            data[px*sz + tof] = yaxis[tof];
        }
    }

    
    
    
    //std::cout << "ws->readX(0).size(): " << ws->readX(0).size() << std::endl;
    
    //    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
    
    RefArrayDataSource* source = new RefArrayDataSource(wps_name);
//    image_view = new RefImageView( source );
}

RefMatrixWSImageView::~RefMatrixWSImageView()
{
  // nothing to do here, since image_view is deleted when the window closes
}

