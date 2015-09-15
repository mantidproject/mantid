#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidKernel/Matrix.h"
#include <sstream> 
#include <cmath>
#include <cfloat>

namespace Mantid
{
namespace VATES
{
  /**
   * Represents a view frustum. It contains the parameters for the six plane equations that define the view frustum. Note that the plane 
   * normals point into the box
   * @param leftPlane The left plane.
   * @param rightPlane The right plane.
   * @param topPlane The top plane.
   * @param bottomPlane The bottom plane.
   * @param farPlane The far plane.
   * @param nearPlane The near plane.
   */
  ViewFrustum::ViewFrustum(const LeftPlane leftPlane, const RightPlane rightPlane, const BottomPlane bottomPlane,
                            const TopPlane topPlane, const FarPlane farPlane,  const NearPlane nearPlane) : m_leftPlane(leftPlane),
                                                                                                                    m_rightPlane(rightPlane),
                                                                                                                    m_topPlane(topPlane),
                                                                                                                    m_bottomPlane(bottomPlane),
                                                                                                                    m_farPlane(farPlane),
                                                                                                                    m_nearPlane(nearPlane){}
  /**
   * Copy constructor for the view frustum.
   * @param other The initializing view frustum.
   */
  ViewFrustum::ViewFrustum(const ViewFrustum& other): m_leftPlane(other.m_leftPlane),
                                                      m_rightPlane(other.m_rightPlane),
                                                      m_topPlane(other.m_topPlane),
                                                      m_bottomPlane(other.m_bottomPlane),
                                                      m_farPlane(other.m_farPlane),
                                                      m_nearPlane(other.m_nearPlane){}
  /// Destructor
  ViewFrustum::~ViewFrustum(){}

  /**
   * Assignment operator
   * @param other The assigned view frustum.
   */
  ViewFrustum& ViewFrustum::operator=(const ViewFrustum& other)
  {
    if (&other != this)
    {
      m_leftPlane = other.m_leftPlane;
      m_rightPlane  = other.m_rightPlane;
      m_topPlane = other.m_topPlane;
      m_bottomPlane = other.m_bottomPlane;
      m_farPlane = other.m_farPlane;
      m_nearPlane = other.m_nearPlane;
    }

    return *this;
  }

  /**
   * Get the extents of the View frustum. We take the minimal rectangular box which in contains the 
   * view frustum fully.
   * @returns A vector with the extents
   */
  std::vector<std::pair<double, double>> ViewFrustum::toExtents() const
  {
    // Get the eight corner points of the view frustum
    std::vector<std::vector<double>> frustumPoints;
    frustumPoints.push_back(getIntersectionPointThreePlanes<LEFTPLANE, TOPPLANE, FARPLANE, double>(m_leftPlane, m_topPlane, m_farPlane));
    frustumPoints.push_back(getIntersectionPointThreePlanes<LEFTPLANE, TOPPLANE, NEARPLANE, double>(m_leftPlane, m_topPlane, m_nearPlane));

    frustumPoints.push_back(getIntersectionPointThreePlanes<LEFTPLANE, BOTTOMPLANE, FARPLANE, double>(m_leftPlane, m_bottomPlane, m_farPlane));
    frustumPoints.push_back(getIntersectionPointThreePlanes<LEFTPLANE, BOTTOMPLANE, NEARPLANE, double>(m_leftPlane, m_bottomPlane, m_nearPlane));

    frustumPoints.push_back(getIntersectionPointThreePlanes<RIGHTPLANE, TOPPLANE, FARPLANE, double>(m_rightPlane, m_topPlane, m_farPlane));
    frustumPoints.push_back(getIntersectionPointThreePlanes<RIGHTPLANE, TOPPLANE, NEARPLANE, double>(m_rightPlane, m_topPlane, m_nearPlane));

    frustumPoints.push_back(getIntersectionPointThreePlanes<RIGHTPLANE, BOTTOMPLANE, FARPLANE, double>(m_rightPlane, m_bottomPlane, m_farPlane));
    frustumPoints.push_back(getIntersectionPointThreePlanes<RIGHTPLANE, BOTTOMPLANE, NEARPLANE, double>(m_rightPlane, m_bottomPlane, m_nearPlane));

    std::vector<std::pair<double, double>> extents;

    for (int i = 0; i < 3; ++i)
    {
      std::pair<double, double> minMax(DBL_MAX, -DBL_MAX);
      for (std::vector<std::vector<double>>::iterator it = frustumPoints.begin(); it != frustumPoints.end(); ++it)
      {
        if ((*it)[i] < minMax.first)
        {
          minMax.first = (*it)[i];
        }

        if ((*it)[i] > minMax.second)
        {
          minMax.second = (*it)[i];
        }
      }

      extents.push_back(minMax);
    }

    return extents;
  }

  /**
   * Get the extents as a concatenated string.
   * @returns The extens of the view frustum as a concatenated string
   */
  std::string ViewFrustum::toExtentsAsString() const
  {
    std::vector<std::pair<double, double>> extents = toExtents();
    
    std::stringstream ss;

    for (std::vector<std::pair<double, double>>::iterator it = extents.begin(); it != extents.end(); ++it)
    {
      ss << it->first << "," << it->second;

      if ((it+1)!= extents.end())
      {
        ss << ",";
      }
    }

    return ss.str();
  }

}
}
