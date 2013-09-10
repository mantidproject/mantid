#include <iostream>
#include "MantidQtRefDetectorViewer/RefMatrixWSImageView.h"
#include "MantidQtSpectrumViewer/ArrayDataSource.h"
#include "MantidQtRefDetectorViewer/RefIVConnections.h"
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
RefMatrixWSImageView::RefMatrixWSImageView( MatrixWorkspace_sptr /*mat_ws*/ )
{
    return;
    //  RefMatrixWSDataSource* source = new RefMatrixWSDataSource( mat_ws );
//  image_view = new RefImageView( source );  // this is the QMainWindow
//                                         // for the viewer.  It is
//                                         // deleted when the window
//                                         // is closed
}

RefMatrixWSImageView::RefMatrixWSImageView( QString wps_name, int peak_min, int peak_max, int back_min, int back_max, int tof_min, int tof_max)
{

    IEventWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<IEventWorkspace>(wps_name.toStdString());

    const double total_ymin = 0.0;
    const double total_ymax = 255.0;
    const size_t total_rows = 256;
//    const double total_ymax = 303;
//    const size_t total_rows = 304;
    
    
    std::vector<double> xaxis = ws->readX(0);
    const size_t sz = xaxis.size()-1;
    const size_t total_cols = sz;
    
    double total_xmin = xaxis[0];
    double total_xmax = xaxis[sz];
    
    float *data = new float[static_cast<size_t>(total_ymax) * sz];
    
//    std::cout << "Starting the for loop " << std::endl;
//    std::cout << "total_xmax: " << total_xmax << std::endl;
//    std::cout << "sz is : " << sz << std::endl;
    
    std::vector<double> yaxis;
    for (size_t px=0; px<total_ymax; px++)
    {
        //retrieve data now
        yaxis = ws->readY(px);
        for (size_t tof=0; tof<sz; tof++)
        {
            data[px*sz + tof] = static_cast<float>(yaxis[tof]);
        }
    }
    
    ImageView::ArrayDataSource* source = new ImageView::ArrayDataSource(total_xmin, total_xmax,
                                                        total_ymin, total_ymax,
                                                        total_rows, total_cols,
                                                        data);
    
//    std::cout << "ws->readX(0).size(): " << ws->readX(0).size() << std::endl;

    
    
    image_view = new RefImageView( source,
                                  peak_min, peak_max,
                                  back_min, back_max,
                                  tof_min, tof_max);

}


RefIVConnections* RefMatrixWSImageView::getConnections()
{
  return image_view->getIVConnections();

}


RefMatrixWSImageView::~RefMatrixWSImageView()
{
  // nothing to do here, since image_view is deleted when the window closes
}

