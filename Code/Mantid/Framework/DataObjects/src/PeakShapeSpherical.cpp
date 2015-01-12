#include "MantidDataObjects/PeakShapeSpherical.h"
#include <stdexcept>
#include <jsoncpp/json/json.h>

namespace Mantid
{
namespace DataObjects
{

PeakShapeSpherical::PeakShapeSpherical(const Kernel::VMD &peakCentre, const double &peakRadius, API::SpecialCoordinateSystem frame, std::string algorithmName, int algorithmVersion) :
    m_centre(peakCentre), m_radius(peakRadius), m_frame(frame), m_algorithmName(algorithmName), m_algorithmVersion(algorithmVersion)
{

}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
PeakShapeSpherical::~PeakShapeSpherical()
{
}

/**
 * @brief Copy constructor
 * @param other : source of the copy
 */
PeakShapeSpherical::PeakShapeSpherical(const PeakShapeSpherical &other) : m_centre(other.centre()), m_radius(other.radius()), m_frame(other.frame()), m_algorithmName(other.algorithmName()), m_algorithmVersion(other.algorithmVersion())
{
}

/**
 * @brief Assignment operator
 * @param other : source of the assignment
 * @return Ref to assigned object.
 */
PeakShapeSpherical& PeakShapeSpherical::operator=(const PeakShapeSpherical &other)
{
    if(this != &other)
    {
        m_centre = other.centre();
        m_radius = other.radius();
        m_algorithmName = other.algorithmName();
        m_algorithmVersion = other.algorithmVersion();
        m_frame = other.frame();
    }
    return *this;
}

/**
 * @brief PeakShapeSpherical::frame
 * @return The coordinate frame used
 */
API::SpecialCoordinateSystem PeakShapeSpherical::frame() const
{
    return m_frame;
}

/**
 * @brief Serialize to JSON object
 * @return JSON object as std::string
 */
std::string PeakShapeSpherical::toJSON() const
{
    Json::Value root;
    Json::Value shape("spherical");
    Json::Value algorithmName(this->algorithmName());
    Json::Value algorithmVersion(this->algorithmVersion());
    Json::Value centre;
    for(size_t i = 0; i < m_centre.size(); ++i)
    {
        centre.append(m_centre[i]);
    }
    Json::Value radius(this->radius());
    Json::Value frame(this->frame());
    root["shape"] = shape;
    root["algorithm_name"] = algorithmName;
    root["algorithm_version"] = algorithmVersion;
    root["centre"] = centre;
    root["radius"] = radius;
    root["frame"] = frame;

    Json::StyledWriter writer;
    return writer.write(root);
}

/**
 * @brief Get the algorithm name
 * @return Algorithm name
 */
std::string PeakShapeSpherical::algorithmName() const
{
    return m_algorithmName;
}

/**
 * @brief Get the algorithmVersion
 * @return Algorithm version
 */
int PeakShapeSpherical::algorithmVersion() const
{
    return m_algorithmVersion;
}

/**
 * @brief Clone object as deep copy
 * @return pointer to new object
 */
PeakShapeSpherical* PeakShapeSpherical::clone() const
{
    return new PeakShapeSpherical(*this);
}

bool PeakShapeSpherical::operator==(const PeakShapeSpherical &other) const
{
    return other.centre() == this->centre() &&
    other.radius() == this->radius() &&
    other.frame() == this->frame();
}

/**
 * @brief Get radius of sphere
 * @return radius
 */
double PeakShapeSpherical::radius() const
{
    return m_radius;
}

/**
 * @brief Get centre of sphere
 * @return centre as VMD
 */
Mantid::Kernel::VMD PeakShapeSpherical::centre() const
{
    return m_centre;
}



} // namespace DataObjects
} // namespace Mantid
