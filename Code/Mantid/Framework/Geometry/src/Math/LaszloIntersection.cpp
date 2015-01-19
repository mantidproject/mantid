//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Math/LaszloIntersection.h"
#include "MantidGeometry/Math/Vertex2D.h"
#include "MantidKernel/Exception.h"
#include <iostream>

namespace Mantid {
namespace Geometry {
using Kernel::V2D;

namespace {

enum eEdgeIn {
  Unknown,   /**< Which edge is inside the other is not known */
  PIsInside, /**< Edge P is inside of edge Q */
  QIsInside  /**< Edge Q is inside of edge P */
};

/**
 * Advance the current vertex on the given polygon. If the inside flag is set
 * record a point of intersection on the given vertex
 * @param iter :: An iterator on the current polygon
 * @param lastIntersect :: A pointer to the last vertex in the intersection list
 * @param inside :: True if the current polygon point is inside another
 */
void advanceVertex(Vertex2DIterator &iter, Vertex2D *&lastIntersect,
                   const bool inside) {
  iter.advance();
  const Kernel::V2D &curPolyPt = iter.point();
  if (inside && (lastIntersect->point() != curPolyPt)) {
    // Add an intersection as the point is inside the polygon
    lastIntersect = lastIntersect->insert(new Vertex2D(curPolyPt));
#ifdef VERBOSE
    std::cout << "Advance adds cross pt: (" << curPolyPt.X() << ","
              << curPolyPt.Y() << ")" << std::endl;
#endif
  }
}
} // Anonymous namespace

/**
* Compute the polygon that defines the intersection between two
* other concex polygons using the method of chasing edges implemented by Laszlo
* @param P :: A reference to the first polygon
* @param Q :: A reference to the second polygon
* @returns A new polygon defining the region of intersection
*/
ConvexPolygon intersectionByLaszlo(const ConvexPolygon &P,
                                   const ConvexPolygon &Q) {
  // The algorithm requires that the polygon with the greatest unsigned area
  // be on the "Left"
  if (P.determinant() < Q.determinant())
    return intersectionByLaszlo(Q, P);

  Vertex2D *curIntersection(NULL);
  V2D iPnt, startPnt;
  Vertex2DIterator pIter(P.head()), qIter(Q.head());
  eEdgeIn inflag = Unknown;
  int phase = 1;
  size_t maxItns = 2 * (P.numVertices() + Q.numVertices());
  for (size_t i = 1; i <= maxItns; ++i) {
#ifdef VERBOSE
    std::cout << "Iteration " << i << " Phase = " << phase << std::endl;
#endif
    const PolygonEdge edgeP = pIter.edge();
    const PolygonEdge edgeQ = qIter.edge();
    PointClassification pclass = classify(edgeP.end(), edgeQ);
#ifdef VERBOSE
    std::cout << "Class P Pt" << std::endl;
    std::cout << "Class Pt: (" << edgeP.end().X() << "," << edgeP.end().Y()
              << ")";
    std::cout << std::endl;
    std::cout << "Edge Orig Pt (" << edgeQ.start().X() << ","
              << edgeQ.start().Y() << ")";
    std::cout << std::endl;
    std::cout << "Edge Dest Pt (" << edgeQ.end().X() << "," << edgeQ.end().Y()
              << ")";
    std::cout << std::endl;
    std::cout << "P pt class: " << pclass << std::endl;
#endif
    PointClassification qclass = classify(edgeQ.end(), edgeP);
#ifdef VERBOSE
    std::cout << "Class Q Pt" << std::endl;
    std::cout << "Class Pt: (" << edgeQ.end().X() << "," << edgeQ.end().Y()
              << ")";
    std::cout << std::endl;
    std::cout << "Edge Orig Pt (" << edgeP.start().X() << ","
              << edgeP.start().Y() << ")";
    std::cout << std::endl;
    std::cout << "Edge Dest Pt (" << edgeP.end().X() << "," << edgeP.end().Y()
              << ")";
    std::cout << std::endl;
    std::cout << "Q pt class: " << qclass << std::endl;
#endif
    PolygonEdge::Orientation crossType = crossingPoint(edgeP, edgeQ, iPnt);
#ifdef VERBOSE
    std::cout << "PQ Orient: " << crossType << std::endl;
#endif
    if (crossType == PolygonEdge::SkewCross) {
      if (phase == 1) {
        phase = 2;
#ifdef VERBOSE
        std::cout << "Found a crossing pt: (" << iPnt.X() << ",";
        std::cout << iPnt.Y() << ")" << std::endl;
#endif

        curIntersection = new Vertex2D(iPnt);
        startPnt = iPnt;
      } else if (iPnt != *curIntersection) {
#ifdef VERBOSE
        std::cout << "Found a crossing pt: (" << iPnt.X() << ",";
        std::cout << iPnt.Y() << ")" << std::endl;
#endif
        if (iPnt != startPnt) {
          curIntersection = curIntersection->insert(new Vertex2D(iPnt));
        } else // Back to the start, we're done
        {
          try {
            // Make the head vertex of the polygon the first one we found
            return ConvexPolygon(curIntersection->next());
          } catch (std::invalid_argument &) {
            Vertex2D::deleteChain(curIntersection);
            throw NoIntersectionException();
          }
        }
      }
      if (pclass == OnRight) {
        inflag = PIsInside;
      } else if (qclass == OnRight) {
        inflag = QIsInside;
      } else {
        inflag = Unknown;
      }
    } else if ((crossType == PolygonEdge::Collinear) && (pclass != Behind) &&
               (qclass != Behind)) {
      inflag = Unknown;
    }
#ifdef VERBOSE
    std::cout << "Current in flag: " << inflag << std::endl;
#endif

    bool pAIMSq = edgeAimsAt(edgeP, edgeQ, pclass, crossType);
    bool qAIMSp = edgeAimsAt(edgeQ, edgeP, qclass, crossType);
#ifdef VERBOSE
    std::cout << "P aims at Q:" << pAIMSq << std::endl;
    std::cout << "Q aims at P:" << qAIMSp << std::endl;
#endif
    if (pAIMSq && qAIMSp) {
      if ((inflag == QIsInside) ||
          ((inflag == Unknown) && (pclass == OnLeft))) {
#ifdef VERBOSE
        std::cout << "Move edge on P" << std::endl;
#endif
        advanceVertex(pIter, curIntersection, false);
      } else {
#ifdef VERBOSE
        std::cout << "Move edge on Q" << std::endl;
#endif
        advanceVertex(qIter, curIntersection, false);
      }
    } else if (pAIMSq) {
#ifdef VERBOSE
      std::cout << "Move edge on P" << std::endl;
#endif
      advanceVertex(pIter, curIntersection, inflag == PIsInside);
    } else if (qAIMSp) {
#ifdef VERBOSE
      std::cout << "Move edge on Q" << std::endl;
#endif
      advanceVertex(qIter, curIntersection, inflag == QIsInside);
    } else {
      if ((inflag == QIsInside) ||
          ((inflag == Unknown) && (pclass == OnLeft))) {
#ifdef VERBOSE
        std::cout << "Move edge on P" << std::endl;
#endif
        advanceVertex(pIter, curIntersection, false);
      } else {
#ifdef VERBOSE
        std::cout << "Move edge on Q" << std::endl;
#endif
        advanceVertex(qIter, curIntersection, false);
      }
    }
  } // end-for

  // Reaching this point means we have no intersections
  // of the polygon edges. There is the possiblity that
  // the larger polygon completely encloses the smaller
  // in which case no edge intersections would be found
  if (P.contains(Q)) {
    return Q;
  }

  throw NoIntersectionException();
}

} // namespace Geometry
} // namespace Mantid
