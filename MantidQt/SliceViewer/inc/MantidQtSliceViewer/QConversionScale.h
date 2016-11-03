#include <qwt_color_map.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot.h>
#include <qwt_scale_draw.h>

class QConversionScaleDraw : public QwtScaleDraw {
public:
  explicit QConversionScaleDraw() {}

  virtual QwtText label(double value) const override {
    // value is x

    return QwtScaleDraw::label(value * 2);
  }

private:
  double m_conversionFactor;
};