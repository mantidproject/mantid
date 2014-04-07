#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include <stdexcept>

namespace Mantid
{
namespace Poldi
{

UncertainValue::UncertainValue() :
    m_value(0.0),
    m_error(0.0)
{
}

UncertainValue::UncertainValue(double value, double error) :
    m_value(value),
    m_error(error)
{
}

double UncertainValue::value() const
{
    return m_value;
}

double UncertainValue::error() const
{
    return m_error;
}

const UncertainValue UncertainValue::plainAddition(const UncertainValue &left, const UncertainValue &right)
{
    return UncertainValue(left.m_value + right.m_value, left.m_error + right.m_error);
}

bool UncertainValue::lessThanError(const UncertainValue &left, const UncertainValue &right)
{
    return left.m_error < right.m_error;
}

double UncertainValue::valueToErrorRatio(const UncertainValue &uncertainValue)
{
    if(uncertainValue.error() == 0.0) {
        throw std::domain_error("Division by zero is not defined.");
    }
    return uncertainValue.value() / uncertainValue.error();
}

}
}
