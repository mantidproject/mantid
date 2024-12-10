// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Math/PolygonIntersection.h"
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/PolygonEdge.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V2D.h"

using namespace Mantid::Kernel;

namespace Mantid::Geometry {

namespace {

//------------------------------------------------------------------------------
// Anonymous helpers
//------------------------------------------------------------------------------

/// Static logger
Kernel::Logger g_log("PolygonIntersection");

// Uncomment this to get detailed statements of the exact progress of the
// intersection
// calculation
// #define DEBUG_INTERSECTION

/// Define a macro to include the logging statements if requested. They hamper
/// performance if
/// always included
#ifdef DEBUG_INTERSECTION
#define VERBOSE(X) X
#else
#define VERBOSE(X)
#endif

enum eEdgeIn {
  Unknown,   /**< Which edge is inside the other is not known */
  PIsInside, /**< Edge P is inside of edge Q */
  QIsInside  /**< Edge Q is inside of edge P */
};

/**
 * Advance the current vertex on the given polygon. If the inside flag is set
 * record a point of intersection on the given vertex
 * @param iter :: An iterator on the current polygon
 * @param out :: A reference to the overlap polygon being built
 * @param lastIntersect :: A reference to the last intersection
 * @param inside :: True if the current polygon point is inside another
 */
void advanceVertex(ConvexPolygon::Iterator &iter, ConvexPolygon &out, const V2D &lastIntersect, const bool inside) {
  ++iter;
  const auto &curPolyPt = *iter;
  if (inside && (lastIntersect != curPolyPt)) {
    // Add an intersection as the point is inside the polygon
    out.insert(curPolyPt);
    VERBOSE(std::cout << "Advance adds cross pt: (" << curPolyPt.X() << "," << curPolyPt.Y() << ")\n");
  }
}

} // Anonymous namespace

//------------------------------------------------------------------------------
// Public functions
//------------------------------------------------------------------------------
/**
 * The intersection is computed using the method described by Michael Laszlo in
 * "Computational Geometry and Computer Graphics in C++" by Michael J. Laszlo.
 * It assumes that each polygon is closed and has no holes.
 * @param P First polygon
 * @param Q Second polygon
 * @param out A reference to the object to fill with the intersections. The
 * object
 * is not touched with the exception of appending vertices for the calculated
 * overlap. If the same object is reused with multiple calculations this must be
 * handled by the user.
 * @return True if a valid intersection was found, false otherwise
 */
bool MANTID_GEOMETRY_DLL intersection(const ConvexPolygon &P, const ConvexPolygon &Q, ConvexPolygon &out) {

  // The algorithm requires that the polygon with the greatest unsigned area
  // be on the "Left"
  VERBOSE(std::cout << "Area of P (" << P.area() << "). Area of Q (" << Q.area() << ")\n");
  if (P.area() < Q.area()) {
    VERBOSE(std::cout << "Area of P < Area of Q. Swapping order.\n");
    return intersection(Q, P, out);
  }

  V2D iPnt, startPnt, curIntersection;
  ConvexPolygon::Iterator pIter(P), qIter(Q);
  eEdgeIn inflag = Unknown;
  int phase = 1;
  size_t maxItns = 2 * (P.npoints() + Q.npoints());
  for (size_t i = 1; i <= maxItns; ++i) {
    VERBOSE(std::cout << "Iteration " << i << " Phase = " << phase << '\n');
    const PolygonEdge edgeP = pIter.edge();
    const PolygonEdge edgeQ = qIter.edge();
    PointClassification pclass = classify(edgeP.end(), edgeQ);

    VERBOSE(std::cout << "Class P Pt\n");
    VERBOSE(std::cout << "Class Pt: (" << edgeP.end().X() << "," << edgeP.end().Y() << ")\n");
    VERBOSE(std::cout << "Edge Orig Pt (" << edgeQ.start().X() << "," << edgeQ.start().Y() << ")\n");
    VERBOSE(std::cout << "Edge Dest Pt (" << edgeQ.end().X() << "," << edgeQ.end().Y() << ")\n");
    VERBOSE(std::cout << "P pt class: " << pclass << '\n');

    PointClassification qclass = classify(edgeQ.end(), edgeP);
    VERBOSE(std::cout << "Class Q Pt\n");
    VERBOSE(std::cout << "Class Pt: (" << edgeQ.end().X() << "," << edgeQ.end().Y() << ")\n");
    VERBOSE(std::cout << "Edge Orig Pt (" << edgeP.start().X() << "," << edgeP.start().Y() << ")\n");
    VERBOSE(std::cout << "Edge Dest Pt (" << edgeP.end().X() << "," << edgeP.end().Y() << ")\n");
    VERBOSE(std::cout << "Q pt class: " << qclass << '\n');

    PolygonEdge::Orientation crossType = crossingPoint(edgeP, edgeQ, iPnt);
    VERBOSE(std::cout << "PQ Orient: " << crossType << '\n');

    if (crossType == PolygonEdge::SkewCross) {
      if (phase == 1) {
        phase = 2;
        VERBOSE(std::cout << "Found a crossing pt: (" << iPnt.X() << ",");
        VERBOSE(std::cout << iPnt.Y() << ")\n");

        curIntersection = iPnt;
        out.insert(iPnt);
        startPnt = iPnt;
      } else if (iPnt != curIntersection) {
        VERBOSE(std::cout << "Found a crossing pt: (" << iPnt.X() << ",");
        VERBOSE(std::cout << iPnt.Y() << ")\n");
        if (iPnt != startPnt) {
          curIntersection = iPnt;
          out.insert(iPnt);
        } else // Back to the start, we're done
        {
          // Return the head if it is a valid polygon
          return out.isValid();
        }
      }
      if (pclass == OnRight) {
        inflag = PIsInside;
      } else if (qclass == OnRight) {
        inflag = QIsInside;
      } else {
        inflag = Unknown;
      }
    } else if ((crossType == PolygonEdge::Collinear) && (pclass != Behind) && (qclass != Behind)) {
      inflag = Unknown;
    }
    VERBOSE(std::cout << "Current in flag: " << inflag << '\n');

    bool pAIMSq = edgeAimsAt(edgeP, edgeQ, pclass, crossType);
    bool qAIMSp = edgeAimsAt(edgeQ, edgeP, qclass, crossType);

    VERBOSE(std::cout << "P aims at Q:" << pAIMSq << '\n');
    VERBOSE(std::cout << "Q aims at P:" << qAIMSp << '\n');
    if (pAIMSq && qAIMSp) {
      if ((inflag == QIsInside) || ((inflag == Unknown) && (pclass == OnLeft))) {
        VERBOSE(std::cout << "Move edge on P\n");
        advanceVertex(pIter, out, curIntersection, false);
      } else {
        VERBOSE(std::cout << "Move edge on Q\n");
        advanceVertex(qIter, out, curIntersection, false);
      }
    } else if (pAIMSq) {
      VERBOSE(std::cout << "Move edge on P\n");
      advanceVertex(pIter, out, curIntersection, inflag == PIsInside);
    } else if (qAIMSp) {
      VERBOSE(std::cout << "Move edge on Q\n");
      advanceVertex(qIter, out, curIntersection, inflag == QIsInside);
    } else {
      if ((inflag == QIsInside) || ((inflag == Unknown) && (pclass == OnLeft))) {
        VERBOSE(std::cout << "Move edge on P\n");
        advanceVertex(pIter, out, curIntersection, false);
      } else {
        VERBOSE(std::cout << "Move edge on Q\n");
        advanceVertex(qIter, out, curIntersection, false);
      }
    }
  } // end-for

  // Reaching this point means we have no intersections
  // of the polygon edges. There is the possiblity that
  // the larger polygon completely encloses the smaller
  // in which case no edge intersections would be found
  // but there is an overlap
  if (P.contains(Q)) {
    // standard assignment won't work as the object is a reference
    // and we don't know the exact type
    out = Q.toPoly();
    return true;
  }

  return false;
}

} // namespace Mantid::Geometry
