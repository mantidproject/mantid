#ifndef IMD_NODE_H_
#define IMD_NODE_H_

#include <vector>
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/INode.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidAPI/BoxController.h"

namespace Mantid
{
namespace API
{

class IMDNode : public Kernel::INode
{
public:
//---------------- ISAVABLE
    virtual Kernel::ISaveable *const getISaveable(){return NULL;}
    virtual Kernel::ISaveable *const getISaveable()const{return NULL;}
   
    virtual void clearDataFromMemory()=0;
//-------------------------------------------------------------
    /// Get number of dimensions
    virtual size_t getNumDims() const = 0;

    /// Getter for the masking
    virtual bool getIsMasked() const = 0;

    ///Setter for masking the box
    virtual void mask() = 0;

    ///Setter for unmasking the box
    virtual void unmask() = 0;

    /// get box controller
    virtual Mantid::API::BoxController  *getBoxController() const=0;

    /** Set the box controller used.
     * @param controller :: Mantid::API::BoxController *
     */
    virtual void setBoxController(Mantid::API::BoxController *controller)=0;

    // -------------------------------- Parents/Children-Related -------------------------------------------
    ///  Avoid rtti
    virtual bool isBox()const=0;
    /// Get the total # of unsplit MDBoxes contained.
    virtual size_t getNumMDBoxes() const = 0;
    /// Get the # of children MDBoxBase'es (non-recursive)
    virtual size_t getNumChildren() const = 0;
    /// Return the indexth child MDBoxBase.
    virtual IMDNode * getChild(size_t index) = 0;

    /// Sets the children from a vector of children
    virtual void setChildren(const std::vector<IMDNode *> & boxes, const size_t indexStart, const size_t indexEnd) = 0;

    /// Return a pointer to the parent box
    virtual void setParent(IMDNode * parent)=0;

    /// Return a pointer to the parent box
    virtual IMDNode * getParent()=0;


    /// Return a pointer to the parent box (const)
    virtual const IMDNode * getParent() const = 0;

    // -------------------------------- Events-Related -------------------------------------------
    /// Clear all contained data
    virtual void clear() = 0;
    /// Get total number of points both in memory and on file if present;
    virtual uint64_t getNPoints() const = 0;
    /// get size of the data located in memory, it is equivalent to getNPoints above for memory based workspace but may be different for file based one ;
    virtual size_t getDataInMemorySize()const = 0;
   /// @return the amount of memory that the object takes up in the MRU.
    virtual uint64_t getTotalDataSize() const=0;

    /** The method to convert events in a box into a table of coodrinates/signal/errors casted into coord_t type 
     *   Used to save events from plain binary file
     *   @returns coordTable -- vector of events parameters
     *   @return nColumns    -- number of parameters for each event
     */
    virtual void getEventsData(std::vector<coord_t> &coordTable,size_t &nColumns)const =0;
    /** The method to convert the table of data into vector of events 
     *   Used to load events from plain binary file
     *   @param coordTable -- vector of events parameters
     *   @param nColumns    -- number of parameters for each event
     */
    virtual void setEventsData(const std::vector<coord_t> &coordTable)=0;

    /// Return a copy of contained events
    //virtual std::vector<coor> & getEventsCopy() = 0;
    /// Add a single event
    virtual void addEvent(const std::vector<coord_t> &point, signal_t Signal, signal_t errorSq,uint16_t runIndex,uint32_t detectorId) = 0;
    // add a single event and set pointer to the box which needs splitting (if one actually need)    
    virtual void addAndTraceEvent(const std::vector<coord_t> &point, signal_t Signal, signal_t errorSq,uint16_t runIndex,uint32_t detectorId,size_t index) = 0;

    /// Add a single event, with no mutex locking
    virtual void addEventUnsafe(const std::vector<coord_t> &point, signal_t Signal, signal_t errorSq,uint16_t runIndex,uint32_t detectorId) = 0;

    /// Add several events, within a given range
    //virtual size_t addEventsPart(const std::vector<coord_t> &coords,const signal_t *Signal,const signal_t *errorSq,const  uint16_t *runIndex,const uint32_t *detectorId, const size_t start_at, const size_t stop_at)=0;
    virtual size_t addEvents(const std::vector<signal_t> &sigErrSq,const  std::vector<coord_t> &Coord,const std::vector<uint16_t> &runIndex,const std::vector<uint32_t> &detectorId)=0;

    /// Add several events, within a given range, with no bounds checking
    //virtual size_t addEventsPartUnsafe(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at);
    //size_t addEventsUnsafe(const std::vector<MDE> & events);

    /** Perform centerpoint binning of events
     * @param bin :: MDBin object giving the limits of events to accept.
     * @param fullyContained :: optional bool array sized [nd] of which dimensions are known to be fully contained (for MDSplitBox)
     */
    //virtual void centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const = 0;

    /// General binning method for any shape.
    //virtual void generalBin(MDBin<MDE,nd> & bin, Mantid::Geometry::MDImplicitFunction & function) const = 0;

    /** Sphere (peak) integration */
    virtual void integrateSphere(Mantid::API::CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const = 0;

    /** Find the centroid around a sphere */
    virtual void centroidSphere(Mantid::API::CoordTransform & radiusTransform, const coord_t radiusSquared, coord_t * centroid, signal_t & signal) const = 0;

// box-related
    /** Split sub-boxes, if this is possible and neede for this box */
    virtual void splitAllIfNeeded(Mantid::Kernel::ThreadScheduler * /*ts*/ = NULL)=0;
  

    /** Recalculate signal etc. */
    virtual void refreshCache(Kernel::ThreadScheduler * /*ts*/ = NULL)=0;

    // -------------------------------------------------------------------------------------------
    /** Cache the centroid of this box and all sub-boxes. */
    virtual void refreshCentroid(Kernel::ThreadScheduler * /*ts*/ = NULL)= 0;

    virtual void calculateCentroid(coord_t * /*centroid*/) const=0;


   /// Fill a vector with all the boxes up to a certain depth
    virtual void getBoxes(std::vector<IMDNode *> & boxes, size_t maxDepth, bool leafOnly) = 0;
    /// Fill a vector with all the boxes up to a certain depth
    virtual void getBoxes(std::vector<IMDNode *> & boxes, size_t maxDepth, bool leafOnly, Mantid::Geometry::MDImplicitFunction * function) = 0;


    // -------------------------------- Geometry/vertexes-Related -------------------------------------------

    virtual std::vector<Mantid::Kernel::VMD> getVertexes() const =0;

    virtual coord_t * getVertexesArray(size_t & numVertices) const=0;

    virtual coord_t * getVertexesArray(size_t & numVertices, const size_t outDimensions, const bool * maskDim) const=0;

    virtual void transformDimensions(std::vector<double> & scaling, std::vector<double> & offset)=0;


  

};

}
}

#endif