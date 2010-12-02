// SaveSNSNexus
// @author Freddie Akeroyd, STFC ISIS Faility
// @author Ronald Fowler, STFC eScience. Modified to fit with SaveSNSNexusProcessed
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/SaveSNSNexus.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"

#include <cmath>
#include <boost/shared_ptr.hpp>
#include "Poco/File.h"
#include "hdf5.h"

#include "stdlib.h"
#include <string.h>
#include <time.h>

namespace Mantid
{
namespace NeXus
{

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(SaveSNSNexus)

  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;
  using namespace Geometry;

  /// Empty default constructor
  SaveSNSNexus::SaveSNSNexus() : Algorithm() {}

  /** Initialisation method.
   *
   */
  void SaveSNSNexus::init()
  {
    // Declare required parameters, filename with ext {.nx,.nx5,xml} and input workspac
    std::vector<std::string> exts;
    exts.push_back(".nxs");

    declareProperty(new FileProperty("InputFilename", "", FileProperty::Load, exts),
          "The name of the original Nexus file for this data,\n"
          "as a full or relative path");

    declareProperty(new WorkspaceProperty<MatrixWorkspace> ("InputWorkspace", "", Direction::Input),
        "Name of the workspace to be saved");

    declareProperty(new FileProperty("OutputFilename", "", FileProperty::Save, exts),
          "The name of the Nexus file to write, as a full or relative\n"
          "path");

  }









  hid_t       file_in_id, file_out_id;
  hid_t       grp_in_id, grp_out_id;
  hid_t       subgrp_in_id, subgrp_out_id;
  hid_t       subsubgrp_in_id, subsubgrp_out_id;
  hid_t       dataset_in_id, dataset_out_id;
  hid_t       dataspace;
  hid_t       filespace, memspace;      /* file and memory dataspace identifiers */
  herr_t      status;
  char cbank0[10],cbank[100];
  char ibank0[10],ibank[100];

  // Workspace to write out.
  MatrixWorkspace_sptr inputWorkspace;

  // DetectorID to WS index map
  IndexToIndexMap * map;


  //-----------------------------------------------------------------------------------------------
  /* Copy the attributes from an object to an output object.
   */
  herr_t attr_info(hid_t object_in_id, hid_t object_out_id)
  {
    hid_t   attro; /* Attribute identifiers */
    hid_t   attr;
    hid_t   aido;    /* Attribute dataspace identifiers */
    hid_t   atype, atype_mem;    /* Attribute type */
    H5T_class_t  type_class;
    H5O_info_t oinfo;           /* Object info */
    unsigned i;              /* Counters */
    ssize_t Asize;
    char    ANAMES[80];     /* Buffer to read string attribute back */
    char    string_out[80];     /* Buffer to read string attribute back */
    int     point_out;          /* Buffer to read scalar attribute back */

    /*
     * Find string attribute by iterating through all attributes
     */
    status = H5Oget_info(object_in_id, &oinfo);
    if(status < 0) {
      printf("Can not get attribute info  \n");
      status = H5close();
      return -1;
    }

    for(i = 0; i < (unsigned)oinfo.num_attrs; i++) {
      attr = H5Aopen_by_idx(object_in_id, ".", H5_INDEX_CRT_ORDER,
          H5_ITER_INC, (hsize_t)i, H5P_DEFAULT, H5P_DEFAULT);
      atype = H5Aget_type(attr);
      type_class = H5Tget_class(atype);
      if (type_class == H5T_STRING) {
        atype_mem = H5Tget_native_type(atype, H5T_DIR_ASCEND);
        status = H5Aread(attr, atype_mem, string_out);
        if(status < 0) {
          printf("Can not read string attribute  \n");
          status = H5close();
          return -1;
        }
        /*
         * Create string attribute.
         */
        aido  = H5Screate(H5S_SCALAR);
        Asize = H5Aget_name(attr,80,ANAMES);
        attro = H5Acreate2(object_out_id, ANAMES, atype, aido,
            H5P_DEFAULT, H5P_DEFAULT);
        /*
         * Write string attribute.
         */
        status = H5Awrite(attro, atype, string_out);
        if(status < 0) {
          printf("Can not write string attribute  \n");
          status = H5close();
          return -1;
        }
        status   = H5Tclose(atype_mem);
      }
      if (type_class == H5T_INTEGER) {
        status   = H5Aread(attr, H5T_STD_I16LE, &point_out);
        if(status < 0) {
          printf("Can not read integer attribute  \n");
          status = H5close();
          return -1;
        }
        /*
         * Create integer attribute.
         */
        aido  = H5Screate(H5S_SCALAR);
        Asize = H5Aget_name(attr,80,ANAMES);
        attro = H5Acreate2(object_out_id, ANAMES, H5T_STD_I16LE,
            aido, H5P_DEFAULT, H5P_DEFAULT);
        /*
         * Write integer attribute.
         */
        status = H5Awrite(attro, H5T_STD_I16LE, &point_out);
        if(status < 0) {
          printf("Can not write integer attribute  \n");
          status = H5close();
          return -1;
        }
      }
      status   = H5Tclose(atype);
      status   = H5Aclose(attr);
      status   = H5Sclose(aido);
      status   = H5Aclose(attro);
    }
    return 0;
  }







  //-----------------------------------------------------------------------------------------------
  /** Return a RectangularDetector for the given bank name*/
  boost::shared_ptr<RectangularDetector> GetDetector(char *bank)
  {
    IComponent_sptr det_comp = inputWorkspace->getInstrument()->getComponentByName( std::string(bank) );
    boost::shared_ptr<RectangularDetector> det = boost::dynamic_pointer_cast<RectangularDetector>(det_comp);
    if (!det)
    {
      status = H5close();
      throw std::runtime_error("Bank " + std::string(bank) + " not found or is not a rectangular detector!");
    }
    return det;
  }


  //-----------------------------------------------------------------------------------------------
  void data(char *bank)
  {
    hsize_t dims_out[3];           /* dataset dimensions */
    int status_n, rank;
    char file[100];

    /* opening a dataset */
    dataset_in_id = H5Dopen1(subgrp_in_id, "data");
    if(dataset_in_id < 0)
    {
      printf("Can not open that dataset %s\n", "data");
      status = H5close();
      return;
    }
    dataspace = H5Dget_space(dataset_in_id);    /* dataspace handle */

    // Number of dimensions. Should be 3
    rank      = H5Sget_simple_extent_ndims(dataspace);
    status_n  = H5Sget_simple_extent_dims(dataspace, dims_out, NULL);

    boost::shared_ptr<RectangularDetector> det = GetDetector(bank);

    // Dimension 0 = the X pixels
    dims_out[0] = det->xpixels();
    // Dimension 1 = the Y pixels
    dims_out[1] = det->ypixels();
    // Dimension 2 = time of flight bins
    dims_out[2] = inputWorkspace->blocksize();

    std::cout << "RectangularDetector " << det->getName() << " being copied. Dimensions : "
        << dims_out[0] << ", " << dims_out[1] << ", " << dims_out[2] << ".\n";

    // Make a buffer of floats will all the counts in that bank.
    float * data;
    data = new float[dims_out[0]*dims_out[1]*dims_out[2]];

    for (int x = 0; x < det->xpixels(); x++)
    {
      for (int y = 0; y < det->ypixels(); y++)
      {
        //Get the workspace index for the detector ID at this spot
        int wi;
        try
        {
          wi = (*map)[ det->getAtXY(x,y)->getID() ];
          const MantidVec & Y = inputWorkspace->readY(wi);
          // Offset into array.
          int index = x*dims_out[1]*dims_out[2] + y*dims_out[2];
          // Save in the float array
          for (int i=0; i < static_cast<int>(Y.size()); i++)
            data[index+i] = Y[i];
        }
        catch (...)
        {
          std::cout << "Error finding " << bank << " x " << x << " y " << y << "\n";
        }
      }
    }

    // Create the dataspace for the dataset.
    filespace = H5Screate_simple(3, dims_out, NULL);
    memspace  = H5Screate_simple(3, dims_out, NULL);

    /*
     * Create the dataset with default properties and close filespace.
     */
    strcpy(file,"/entry/");
    strncat(file,bank,strlen(bank));
    strncat(file,"/data",5);

    // We write the data as a float now.
    dataset_out_id = H5Dcreate1(file_out_id, file, H5T_NATIVE_FLOAT, filespace, H5P_DEFAULT);
    status = H5Dwrite(dataset_out_id, H5T_NATIVE_FLOAT, memspace, filespace, H5P_DEFAULT, data);

    if(status < 0)
    {
      printf("Can not write that dataset %s\n", "data");
      status = H5close();
      return;
    }

    // This copies the attributes
    status = attr_info(dataset_in_id,dataset_out_id);
    if(status < 0)
    {
      printf("Can not copy attributes of  dataset %s\n", "data");
      status = H5close();
      return;
    }

    delete [] data;
    return;
  }




  //-----------------------------------------------------------------------------------------------
  /* Write out the time of flight list for the given bank */
  void time_of_flight(char *bank)
  {
    hsize_t     dims_out[3];           /* dataset dimensions */
    float       *tof_data;                /* pointer to data buffer to write */
    int         rank;
    char file[100];

    // Get the X bins
    const MantidVec & X = inputWorkspace->readX(0);

    // 1 dimension, with that number of bin boundaries
    rank = 1;
    dims_out[0] = X.size();

    // The output TOF axis will be whatever size in the workspace.
    tof_data = new float[dims_out[0]];

    // And fill it with the X data
    for (size_t i=0; i < X.size(); i++)
      tof_data[i] = X[i];

    // Create the dataspace for the dataset.
    filespace = H5Screate_simple(1, dims_out, NULL);
    memspace  = H5Screate_simple(1, dims_out, NULL);

    // Create the dataset with default properties and close filespace.
    strcpy(file,"/entry/");
    strncat(file,bank,strlen(bank));
    strncat(file,"/time_of_flight",15);
    dataset_out_id = H5Dcreate1(file_out_id, file, H5T_NATIVE_FLOAT, filespace, H5P_DEFAULT);

    // Write it out, as float.
    status = H5Dwrite(dataset_out_id, H5T_NATIVE_FLOAT, memspace, filespace, H5P_DEFAULT, tof_data);
    if(status < 0)
    {
      printf("Can not write that dataset %s\n", "time_of_flight");
      status = H5close();
      return;
    }

    // Now the attributes
    status = attr_info(dataset_in_id,dataset_out_id);
    if(status < 0)
    {
      printf("Can not copy attributes of  dataset %s\n", "time_of_flight");
      status = H5close();
      return;
    }

    delete [] tof_data;
    return;
  }






  //-----------------------------------------------------------------------------------------------
  /*
   * Copy data groups but act differently if the group is "data" or "time_of_flight".
   */
  herr_t file_info_bank(hid_t loc_id, const char *name, void *opdata)
  {
    char datast[100];
    H5G_stat_t statbuf;

    /*
     * Get type of the object and display its name and type.
     * The name of the object is passed to this function by
     * the Library. Some magic :-)
     */
    H5Gget_objinfo(loc_id, name, 0, &statbuf);
    switch (statbuf.type) {
    case H5G_GROUP:
      break;
    case H5G_DATASET:
      if(!strncmp((char *)name,"data_",5))
      {
        // I will ignore data_x_y, data_y, etc. and other not-so-useful block
        std::cout << "Ignoring " << name << "\n";
      }
      else if(!strncmp((char *)name,"data",4))
      {
        strcpy(datast,cbank);
        strncat(datast,"/",1);
        strncat(datast,(char *)name,strlen(name));
        data(cbank0);
      }
      else if(!strncmp((char *)name,"time",4))
      {
        strcpy(datast,cbank);
        strncat(datast,"/",1);
        strncat(datast,(char *)name,strlen(name));
        time_of_flight(cbank0);
      }
      else
      {
        strcpy(datast,cbank);
        strncat(datast,"/",1);
        strncat(datast,(char *)name,strlen(name));
        status =  H5Ocopy(file_in_id, datast, file_out_id, datast, H5P_DEFAULT, H5P_DEFAULT);
      }
      break;
    case H5G_TYPE:
      break;
    default:
      printf(" Unable to identify an object ");
    }
    return 0;
  }







  //-----------------------------------------------------------------------------------------------
  /*
   * Operator function.
   */
  herr_t file_info_inst_bank(hid_t loc_id, const char *name, void *opdata)
  {
    char datast[100];
    char linkst[100];
    H5G_stat_t statbuf;

    /*
     * Get type of the object and display its name and type.
     * The name of the object is passed to this function by
     * the Library. Some magic :-)
     */
    H5Gget_objinfo(loc_id, name, 0, &statbuf);
    switch (statbuf.type) {
    case H5G_GROUP:
      strcpy(datast,ibank);
      strncat(datast,"/",1);
      strncat(datast,(char *)name,strlen(name));
      status =  H5Ocopy(file_in_id, datast, file_out_id, datast, H5P_DEFAULT, H5P_DEFAULT);
      break;
    case H5G_DATASET:
      if(!strncmp((char *)name,"data",4)
          ||!strncmp((char *)name,"dist",4)
          ||!strncmp((char *)name,"pola",4)
          ||!strncmp((char *)name,"time",4)
          ||!strncmp((char *)name,"x_pi",4)
          ||!strncmp((char *)name,"y_pi",4))
      {
        strcpy(datast,ibank);
        strncat(datast,"/",1);
        strncat(datast,(char *)name,strlen(name));
        strcpy(linkst,"/entry/");
        strncat(linkst,ibank0,strlen(ibank0));
        strncat(linkst,"/",1);
        strncat(linkst,(char *)name,strlen(name));
        status = H5Glink (file_out_id, H5G_LINK_HARD, linkst, datast);
      }
      else
      {
        strcpy(datast,ibank);
        strncat(datast,"/",1);
        strncat(datast,(char *)name,strlen(name));
        status =  H5Ocopy(file_in_id, datast, file_out_id, datast, H5P_DEFAULT, H5P_DEFAULT);
      }
      break;
    case H5G_TYPE:
      break;
    default:
      printf(" Unable to identify an object ");
    }
    return 0;
  }





  //-----------------------------------------------------------------------------------------------
  /*
   * Operator function.
   */
  herr_t file_info_inst(hid_t loc_id, const char *name, void *opdata)
  {
    char datast[100];
    H5G_stat_t statbuf;

    /*
     * Get type of the object and display its name and type.
     * The name of the object is passed to this function by
     * the Library. Some magic :-)
     */
    H5Gget_objinfo(loc_id, name, 0, &statbuf);
    switch (statbuf.type) {
    case H5G_GROUP:
      if(!strncmp((char *)name,"bank",4))
      {
        strcpy(ibank,"/entry/instrument/");
        strncat(ibank,(char *)name,strlen(name));
        strcpy(ibank0,(char *)name);
        subsubgrp_out_id = H5Gcreate1(file_out_id, ibank, 0);


        // opening a new subgroup
        subsubgrp_in_id = H5Gopen1(subgrp_in_id, ibank0);
        if(subsubgrp_in_id < 0) {
          printf("Can not open that group %s\n", ibank0);
          status = H5close();
          return -1;
        }
        status = attr_info(subsubgrp_in_id,subsubgrp_out_id);
        H5Giterate(file_in_id, ibank, NULL, file_info_inst_bank, NULL);
        H5Gclose(subsubgrp_in_id);

      }
      else
      {
        strcpy(datast,cbank);
        strncat(datast,"/",1);
        strncat(datast,(char *)name,strlen(name));
        status =  H5Ocopy(file_in_id, datast, file_out_id, datast, H5P_DEFAULT, H5P_DEFAULT);
      }
      break;
    case H5G_DATASET:
      strcpy(datast,cbank);
      strncat(datast,"/",1);
      strncat(datast,(char *)name,strlen(name));
      status =  H5Ocopy(file_in_id, datast, file_out_id, datast, H5P_DEFAULT, H5P_DEFAULT);
      break;
    case H5G_TYPE:
      break;
    default:
      printf(" Unable to identify an object ");
    }
    return 0;
  }



  //-----------------------------------------------------------------------------------------------
  /*
   * Base function for copying a SNS Nexus file, to be called by HF5GIterate
   */
  herr_t file_info(hid_t loc_id, const char *name, void *opdata)
  {
    H5G_stat_t statbuf;

    /*
     * Get type of the object and display its name and type.
     * The name of the object is passed to this function by
     * the Library. Some magic :-)
     */
    H5Gget_objinfo(loc_id, name, 0, &statbuf);
    switch (statbuf.type) {
    case H5G_GROUP:
      if(!strncmp((char *)name,"bank",4))
      {
        strcpy(cbank,"/entry/");
        strncat(cbank,(char *)name,strlen(name));
        strcpy(cbank0,(char *)name);
        subgrp_out_id = H5Gcreate1(file_out_id, cbank, 0);

        // opening a new subgroup
        subgrp_in_id = H5Gopen1(grp_in_id, cbank0);
        if(subgrp_in_id < 0) {
          printf("Can not open that group %s\n", cbank0);
          status = H5close();
          return -1;
        }
        status = attr_info(subgrp_in_id,subgrp_out_id);
        H5Giterate(file_in_id, cbank, NULL, file_info_bank, NULL);
      }
      else if(!strncmp((char *)name,"inst",4))
      {
        strcpy(cbank,"/entry/");
        strncat(cbank,(char *)name,strlen(name));
        strcpy(cbank0,(char *)name);
        subgrp_out_id = H5Gcreate1(file_out_id, cbank, 0);


        // opening a new subgroup
        subgrp_in_id = H5Gopen1(grp_in_id, cbank0);
        if(subgrp_in_id < 0) {
          printf("Can not open that group %s\n", cbank0);
          status = H5close();
          return -1;
        }
        status = attr_info(subgrp_in_id,subgrp_out_id);
        H5Giterate(file_in_id, cbank, NULL, file_info_inst, NULL);
      }
      else
      {
        strcpy(cbank,"/entry/");
        strncat(cbank,(char *)name,strlen(name));
        status =  H5Ocopy(file_in_id, cbank, file_out_id, cbank, H5P_DEFAULT, H5P_DEFAULT);
      }
      break;
    case H5G_DATASET:
      strcpy(cbank,"/entry/");
      strncat(cbank,(char *)name,strlen(name));
      status =  H5Ocopy(file_in_id, cbank, file_out_id, cbank, H5P_DEFAULT, H5P_DEFAULT);
      break;
    case H5G_TYPE:
      break;
    default:
      printf(" Unable to identify an object ");
    }
    return 0;
  }











  /** Execute the algorithm.
   *
   *  @throw runtime_error Thrown if algorithm cannot execute
   */
  void SaveSNSNexus::exec()
  {
    // Retrieve the filename from the properties
    m_inputFilename = getPropertyValue("InputFileName");
    m_inputWorkspace = getPropertyValue("InputWorkspace");
    m_outputFilename = getPropertyValue("OutputFileName");

    inputWorkspace = getProperty("InputWorkspace");

    if(H5open() < 0 )
    {
      throw std::runtime_error("Something went wrong when initializing HDF5\n");
    }

    g_log.information() << "Creating file " << m_outputFilename << " \n";

    file_out_id = H5Fcreate(m_outputFilename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if(file_out_id < 0){
      status = H5close();
      throw std::runtime_error("Can not create file " + m_outputFilename );
    }
    grp_out_id = H5Gcreate1(file_out_id, "/entry", 0);
    if(grp_out_id < 0) {
      status = H5close();
      throw std::runtime_error("Can not create that group 'entry'");
    }

    // open file for read and write access
    file_in_id = H5Fopen(m_inputFilename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    if(file_in_id < 0)
    {
      status = H5close();
      throw std::runtime_error("Can not open file " + m_inputFilename);
    }

    /* copying attributes from original to new file */
    status = attr_info(file_in_id,file_out_id);

    /* opening a new group */
    grp_in_id = H5Gopen1(file_in_id, "entry");
    if(grp_in_id < 0) {
      status = H5close();
      throw std::runtime_error("Can not open that group 'entry'");
    }

    /* copying attributes from original to new entry group */
    status = attr_info(grp_in_id,grp_out_id);

    // We'll need to get workspace indices
    map = inputWorkspace->getDetectorIDToWorkspaceIndexMap( false );

    /* Iterate through the file to see members of the root group */
    H5Giterate(file_in_id, "/entry", NULL, file_info, NULL);

    // Close/release resources.
    H5Dclose(dataset_in_id);
    H5Gclose(subgrp_in_id);
    H5Gclose(grp_in_id);
    H5Fclose(file_in_id);
    H5Dclose(dataset_out_id);
    H5Sclose(filespace);
    H5Sclose(memspace);
    H5Gclose(subgrp_out_id);
    H5Gclose(grp_out_id);
    H5Fclose(file_out_id);
    H5close();

    // Free map memory
    delete map;

    return;
  }



} // namespace NeXus
} // namespace Mantid
