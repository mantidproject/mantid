#ifndef SMOOTH_NEIGHBOURS_DIALOG_H_
#define SMOOTH_NEIGHBOURS_DIALOG_H_

#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtAPI/AlgorithmPropertiesWidget.h"

using namespace MantidQt::API;

class SmoothNeighboursDialog : public AlgorithmDialog
{
Q_OBJECT

public:
  /// Constructor
  SmoothNeighboursDialog(QWidget* parent = 0);
 
private:
  /// Initialise the layout
  void initLayout();

  /// Parse the input
  void parseInput();

  /// Widget for all the PropertyWidgets
  AlgorithmPropertiesWidget* m_algPropertiesWidget;
};

#endif