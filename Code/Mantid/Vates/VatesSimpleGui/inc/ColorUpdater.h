#ifndef COLORUPDATER_H
#define COLORUPDATER_H

#include <QPair>

class pqColorMapModel;
class pqPipelineRepresentation;

class ColorUpdater
{
public:
  ColorUpdater();
  virtual ~ColorUpdater();

  QPair<double, double> autoScale(pqPipelineRepresentation *repr);
  void colorMapChange(pqPipelineRepresentation *repr,
                      const pqColorMapModel *model);
  void colorScaleChange(pqPipelineRepresentation *repr, double min,
                        double max);
  void logScale(pqPipelineRepresentation *repr, int state);
};

#endif // COLORUPDATER_H
