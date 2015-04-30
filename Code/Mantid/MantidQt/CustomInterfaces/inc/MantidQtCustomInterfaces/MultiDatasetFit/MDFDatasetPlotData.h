#ifndef MDFDATASETPLOTDATA_H_
#define MDFDATASETPLOTDATA_H_

#include <QString>

#include <qwt_double_rect.h>

// Forward declarations
class QwtPlot;
class QwtPlotCurve;

namespace Mantid
{
namespace API
{
  class MatrixWorkspace;
}
}


namespace MantidQt
{
namespace CustomInterfaces
{
namespace MDF
{

/**
 * Contains graphics for a single data set: fitting data, claculated result, difference.
 */
class DatasetPlotData
{
public:
  DatasetPlotData(const QString& wsName, int wsIndex, const QString& outputWSName);
  ~DatasetPlotData();
  void show(QwtPlot *plot);
  void hide();
  QwtDoubleRect boundingRect() const;
private:
  // no copying
  DatasetPlotData(const DatasetPlotData&);
  DatasetPlotData& operator=(const DatasetPlotData&);
  void setData(const Mantid::API::MatrixWorkspace *ws, int wsIndex, const Mantid::API::MatrixWorkspace *outputWS);
  /// Curve object for the fit data (spectrum).
  QwtPlotCurve *m_dataCurve;
  /// Curve object for the calculated spectrum after a fit.
  QwtPlotCurve *m_calcCurve;
  /// Curve object for the difference spectrum.
  QwtPlotCurve *m_diffCurve;
};

} // MDF
} // CustomInterfaces
} // MantidQt


#endif /*MDFDATASETPLOTDATA_H_*/
