#ifndef UNCERTAINVALUE_H
#define UNCERTAINVALUE_H

namespace Mantid
{
namespace Poldi
{
class UncertainValue
{
public:
    UncertainValue();
    UncertainValue(double value, double error);
    ~UncertainValue() {}

    double value() const;
    double error() const;

    static const UncertainValue plainAddition(UncertainValue const& left, UncertainValue const& right);
    static bool lessThanError(UncertainValue const& left, UncertainValue const& right);
    static double valueToErrorRatio(UncertainValue const& uncertainValue);

private:
    double m_value;
    double m_error;
};
}
}

#endif // UNCERTAINVALUE_H
