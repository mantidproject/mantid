#ifndef INTEGRATE_3D_EVENTS_H
#define INTEGRATE_3D_EVENTS_H

#include <vector>
#include <boost/unordered_map.hpp>

#include "MantidKernel/V3D.h"
#include "MantidKernel/Matrix.h"

namespace Mantid
{
namespace MDEvents
{

typedef Mantid::Kernel::V3D                             V3D;
typedef Mantid::Kernel::Matrix<double>                  DblMatrix;
typedef boost::unordered_map< long, std::vector<V3D> >  EventListMap;
typedef boost::unordered_map< long, V3D >               PeakQMap;

/**
 *  Low-level class to construct a map with lists of events near each peak
 *  Q-vector, shifted to be centered at (0,0,0).  A method is also provided
 *  to find the principal axes of such a list of events, and to find the net
 *  integrated counts, using ellipsoids with axis lengths determined from the
 *  standard deviations in the directions of the principal axes.
 */
class DLLExport Integrate3DEvents
{
  public:
    
    /// Construct object to store events around peaks and integrate peaks
    Integrate3DEvents( std::vector<V3D> const & peak_q_list,
                       DblMatrix        const & UBinv,
                       double                   radius   );
    
   ~Integrate3DEvents();


    /// Add event Q's to lists of events near peaks
    void addEvents( std::vector<V3D> const & event_qs );


    /// Find the net integrated intensity of a peak, using ellipsoidal volumes
    void ellipseIntegrateEvents( V3D  const & peak_q,
                                 bool         specify_size,
                                 double       peak_radius,
                                 double       back_inner_radius,
                                 double       back_outer_radius,
                                 double     & inti,
                                 double     & sigi );
  private:

    /// Calculate the number of events in an ellipsoid centered at 0,0,0
    static int numInEllipsoid( std::vector<V3D>    const & events,
                               std::vector<V3D>    const & directions,
                               std::vector<double> const & sizes );


    /// Calculate the 3x3 covariance matrix of a list of Q-vectors at 0,0,0
    static void makeCovarianceMatrix( std::vector<V3D> const & events,
                                      DblMatrix              & matrix,
                                      double                   radius );


    /// Calculate the eigen vectors of a 3x3 real symmetric matrix
    static void getEigenVectors( DblMatrix        const & cov_matrix,
                                 std::vector<V3D>       & eigen_vectors );


    /// Calculate the standard deviation of 3D events in a specified direction
    static double stdDev( std::vector<V3D> const & events,
                          V3D              const & direction,
                          double                   radius );

    
    /// Form a map key as 10^12*h + 10^6*k + l from the integers h, k, l
    static long getHklKey( int h, int k, int l );
  

    /// Form a map key for the specified q_vector.
    long getHklKey( V3D const & q_vector );


    /// Add an event to the vector of events for the closest h,k,l
    void addEvent( V3D event_Q );


    /// Find the net integrated intensity of a list of Q's using ellipsoids 
    void ellipseIntegrateEvents( std::vector<V3D>    const & ev_list,
                                 std::vector<V3D>    const & directions,
                                 std::vector<double> const & sigmas,
                                 bool                        specify_size,
                                 double                      peak_radius,
                                 double                      back_inner_radius,
                                 double                      back_outer_radius,
                                 double                    & inti,
                                 double                    & sigi );

    // Private data members
    PeakQMap      peak_qs;     // hashtable with peak Q-vectors
    EventListMap  event_lists; // hashtable with lists of events for each peak
    DblMatrix     UBinv;       // matrix mapping from Q to h,k,l
    double        radius;      // size of sphere to use for events around a peak
};

}//namespace MDEvents

}//namespace Mantid

#endif // INTEGRATE_3D_EVENTS_H

