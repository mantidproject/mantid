#include "RangeInLambda.h"
namespace MantidQt {
namespace CustomInterfaces {

RangeInLambda::RangeInLambda(double min, double max) : m_min(min), m_max(max){};
double RangeInLambda::min() const { return m_min; }
double RangeInLambda::max() const { return m_max; }
}
}
