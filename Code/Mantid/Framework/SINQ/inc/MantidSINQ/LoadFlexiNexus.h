/**
 * This is a flexible NeXus file loader. It takes as input a filename 
 * and a dictionary file and loads the data from the file into a 
 * suitable workspace. The dictionary can contain to types of lines:
 * Lines starting with # are ignored and treated as comments
 * Then there are lines: property=value These define how the workspace 
 * is constructed. Value can either be an value or a path into the NeXus 
 * file. Data will be loaded from that path then. For property there are 
 * some special values:
 *
 * data defines the path to the main data item. This will determine the 
 *       dimensionality and type of the resulting workspace.
 * x,y,z-axis  is the path to the data for the appropriate axis
 * x,y,z-axis-name is the name of the axis
 *
 * Fill in the usual Mantid disclaimer and copyright here, if you like. 
 * 
 * No warranties are given. Use the code at you own peril. No maintenance 
 * guarantees of any kind are accepted.
 *    
 * Mark Koennecke: mark.koennecke@psi.ch
 */

#ifndef FLEXINEXUSLOADER_H_
#define FLEXINEXUSLOADER_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"

#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>

#include "MantidDataObjects/Workspace2D.h"
#include <boost/shared_array.hpp>
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

class MANTID_SINQ_DLL LoadFlexiNexus : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
 LoadFlexiNexus() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  virtual ~LoadFlexiNexus() {}
  /// Algorithm's name
  virtual const std::string name() const { return "LoadFlexiNexus"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling\\Nexus"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  // A dictionary
  std::map<std::string,std::string> dictionary;

  void loadDictionary(std::string dictionaryFile);

  void load2DWorkspace(NeXus::File *fin);

  void loadMD(NeXus::File *fin);

  void readData(NeXus::File *fin);

  /// Personal wrapper for sqrt to allow msvs to compile
  static double dblSqrt(double in);

  Mantid::Geometry::MDHistoDimension_sptr makeDimension(NeXus::File *fin, int index, int length);

  std::set<std::string>populateSpecialMap();

  void addMetaData(NeXus::File *fin, Mantid::API::Workspace_sptr ws, Mantid::API::ExperimentInfo_sptr info);

  int safeOpenpath(NeXus::File *fin, std::string path);
  int calculateCAddress(int *pos, int* dim, int rank);
  int calculateF77Address(int *pos, int rank);
  size_t *indexMaker;

  virtual void initDocs();
};

#endif /*FLEXINEXUSLOADER_H_*/
