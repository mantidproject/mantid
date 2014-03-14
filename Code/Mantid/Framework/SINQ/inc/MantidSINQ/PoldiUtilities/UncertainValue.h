#ifndef UNCERTAINVALUE_H
#define UNCERTAINVALUE_H

#include "MantidSINQ/DllConfig.h"
#include <cmath>
#include <string>

namespace Mantid
{
namespace Poldi
{
class MANTID_SINQ_DLL UncertainValue
{
public:
    UncertainValue();
    UncertainValue(double value);
    UncertainValue(double value, double error);
    ~UncertainValue() {}

    double value() const;
    double error() const;

    operator double() const;
    UncertainValue operator*(double d);
    UncertainValue operator/(double d);
    UncertainValue operator+(double d);
    UncertainValue operator-(double d);

    operator std::string() const;

    static const UncertainValue plainAddition(UncertainValue const& left, UncertainValue const& right);

    static bool lessThanError(UncertainValue const& left, UncertainValue const& right);
    static double valueToErrorRatio(UncertainValue const& uncertainValue);

private:    
    double m_value;
    double m_error;
};

UncertainValue operator*(double d, const UncertainValue &v);
UncertainValue operator/(double d, const UncertainValue &v);
UncertainValue operator+(double d, const UncertainValue &v);
UncertainValue operator-(double d, const UncertainValue &v);


}
}

#endif // UNCERTAINVALUE_H
