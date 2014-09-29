#include "MantidGeometry/Crystal/V3R.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace Geometry
{

V3R::V3R() :
    m_x(0), m_y(0), m_z(0)
{
}

V3R::V3R(const RationalNumber &x, const RationalNumber &y, const RationalNumber &z) :
    m_x(x), m_y(y), m_z(z)
{
}

V3R::V3R(const V3R &other) :
    m_x(other.m_x), m_y(other.m_y), m_z(other.m_z)
{
}

V3R &V3R::operator =(const V3R &other)
{
    m_x = other.m_x;
    m_y = other.m_y;
    m_z = other.m_z;

    return *this;
}

V3R::~V3R()
{
}

const RationalNumber &V3R::x() const
{
    return m_x;
}

void V3R::setX(const RationalNumber &newX)
{
    m_x = newX;
}

const RationalNumber &V3R::y() const
{
    return m_y;
}

void V3R::setY(const RationalNumber &newY)
{
    m_y = newY;
}

const RationalNumber &V3R::z() const
{
    return m_z;
}

void V3R::setZ(const RationalNumber &newZ)
{
    m_z = newZ;
}

RationalNumber &V3R::operator [](size_t index)
{
    switch(index) {
    case 0: return m_x;
    case 1: return m_y;
    case 2: return m_z;
    default:
        throw Kernel::Exception::IndexError(index, 2, "V3R::operator [] index out of range.");
    }
}

const RationalNumber &V3R::operator [](size_t index) const
{
    switch(index) {
    case 0: return m_x;
    case 1: return m_y;
    case 2: return m_z;
    default:
        throw Kernel::Exception::IndexError(index, 2, "V3R::operator [] index out of range.");
    }
}

// Operations with other vectors
V3R V3R::operator +(const V3R &other) const
{
    V3R result(*this);
    return result += other;
}

V3R &V3R::operator +=(const V3R &other)
{
    m_x += other.m_x;
    m_y += other.m_y;
    m_z += other.m_z;

    return *this;
}

V3R V3R::operator -(const V3R &other) const
{
    V3R result(*this);
    return result -= other;
}

V3R &V3R::operator -=(const V3R &other)
{
    m_x -= other.m_x;
    m_y -= other.m_y;
    m_z -= other.m_z;

    return *this;
}

// Operations with int
V3R V3R::operator +(int other) const
{
    V3R result(*this);
    return result += other;
}

V3R &V3R::operator +=(int other)
{
    m_x += other;
    m_y += other;
    m_z += other;

    return *this;
}

V3R V3R::operator -(int other) const
{
    V3R result(*this);
    return result -= other;
}

V3R &V3R::operator -=(int other)
{
    m_x -= other;
    m_y -= other;
    m_z -= other;

    return *this;
}

V3R V3R::operator *(int other) const
{
    V3R result(*this);
    return result *= other;
}

V3R &V3R::operator *=(int other)
{
    m_x *= other;
    m_y *= other;
    m_z *= other;

    return *this;
}

V3R V3R::operator /(int other) const
{
    V3R result(*this);
    return result /= other;
}

V3R &V3R::operator /=(int other)
{
    m_x /= other;
    m_y /= other;
    m_z /= other;

    return *this;
}

// Operations with rational numbers
V3R V3R::operator +(const RationalNumber &other) const
{
    V3R result(*this);
    return result += other;
}

V3R &V3R::operator +=(const RationalNumber &other)
{
    m_x += other;
    m_y += other;
    m_z += other;

    return *this;
}

V3R V3R::operator -(const RationalNumber &other) const
{
    V3R result(*this);
    return result -= other;
}

V3R &V3R::operator -=(const RationalNumber &other)
{
    m_x -= other;
    m_y -= other;
    m_z -= other;

    return *this;
}

V3R V3R::operator *(const RationalNumber &other) const
{
    V3R result(*this);
    return result *= other;
}

V3R &V3R::operator *=(const RationalNumber &other)
{
    m_x *= other;
    m_y *= other;
    m_z *= other;

    return *this;
}

V3R V3R::operator /(const RationalNumber &other) const
{
    V3R result(*this);
    return result /= other;
}

V3R &V3R::operator /=(const RationalNumber &other)
{
    m_x /= other;
    m_y /= other;
    m_z /= other;

    return *this;
}

V3R::operator Kernel::V3D() const
{
    return Kernel::V3D(boost::rational_cast<double>(m_x),
                       boost::rational_cast<double>(m_y),
                       boost::rational_cast<double>(m_z));
}

bool V3R::operator ==(const V3R &other) const
{
    return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z;
}

bool V3R::operator !=(const V3R &other) const
{
    return !(this->operator==(other));
}

bool V3R::operator <(const V3R &other) const
{
    return m_x < other.m_x && m_y < other.m_y && m_z < other.m_z;
}

V3R operator *(const Kernel::IntMatrix &lhs, const V3R &rhs)
{
    size_t rows = lhs.numRows();
    size_t cols = lhs.numCols();

    if (cols != 3) {
        throw Kernel::Exception::MisMatch<size_t>(cols,3,"operator*(IntMatrix, V3R)");
    }

    V3R result;
    for(size_t r = 0; r < rows; ++r) {
      for(size_t c = 0; c < cols; ++c) {
        result[r]+=lhs[r][c]*rhs[c];
      }
    }

    return result;
}



} // namespace Geometry
} // namespace Mantid
