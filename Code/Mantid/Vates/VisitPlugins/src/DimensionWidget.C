#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QComboBox>
#include <stdio.h>
#include <string>
#include <vector>
#include "DimensionWidget.h"
#include "DimensionPickerWidget.h"
#include "IntegratedDimensionWidget.h"


//#warning "Custom Warning. Incomplete implementation. Does not use Mantid Dimension Domain types for construction!"
//DimensionWidget::DimensionWidget(Mantid::MDAlgorithms::DimensionParameterSet* set)
//: m_set(std::auto_ptr<Mantid::MDAlgorithms::DimensionParameterSet>(set))
//{
//  construct(m_set);
//}

//void DimensionWidget::construct(std::auto_ptr<Mantid::MDAlgorithms::DimensionParameterSet> set)
//{
//  using namespace Mantid::MDAlgorithms;
//
//  std::vector<boost::shared_ptr<DimensionParameter> > dimensions = set->getDimensions();
//  std::vector<boost::shared_ptr<DimensionParameter> >::const_iterator it = dimensions.begin();
//  std::vector<IntegratedDimensionWidget> integratedWidgets;
//  std::vector<DimensionPickerWidget> dimensionPickerWidgets;
//  while(it != dimensions.end())
//  {
//    boost::shared_ptr<DimensionParameter> dimensionParameter =  *it;
//    if(false == dimensionParameter->isIntegrated())
//    {
//      //constructIntegratedDimensionWidget(dimension);
//    }
//    else
//    {
//      //construct
//    }
//    it++;
//  }
//}
