#ifndef H_MD_Pixels
#define H_MD_Pixels

#include "MDDataObjects/MDImageData.h"
/** Class to support operations on single data pixels, as obtained from the instrument. Currently it contains information on the location of the pixel in 
    the reciprocal space but this can chane as this information can be computed in the run time
    
    @author Alex Buts, RAL ISIS
    @date 01/10/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

namespace Mantid{
    namespace MDDataObjects{
     /** 
      * path-through class which provide transformation of the data from pixel buffer format to the data fields format; 
      * It is hard-coded in the class that the run indexes and the detector indexes are present in the data and are located 
      * after two double sized image fields namely signal and error. All other types of data e.g. events data should be processed
      * using similar specialized class
      *
      * CODED FOR LITTLE ENDIAN -> big endian is not supported at the moment
      */
      template<class T>
      class MDDataPoint{
      public:
        MDDataPoint(char * buf, unsigned int n_dimensions,unsigned int nSignals0,unsigned int nIntFields):
            pDataBuffer(buf),
            nDims(n_dimensions),
            nIndexes(nIntFields),
            nSignals(nSignals0)
        {
          pWorkingBuf = new uint16_t[nIndexes];
          // arrange array for all data fields length-es which can be present in the data structure and set its initial values to sizeof(T);
           this->field_lengths.assign(n_dimensions+nSignals+nIntFields,sizeof(T));
           unsigned int i;
           // set field sizes of signals to size of double;
           for(i=n_dimensions;i<n_dimensions+nSignals;i++){
             this->field_lengths[i]=sizeof(double);
           }
           // set field sizes of all integer indicators to 2; Some fields will be combined in a partucular way
           for(i=n_dimensions+nSignals;i<n_dimensions+nSignals+nIntFields;i++){
             this->field_lengths[i]=2;
           }
          // allocate and calculate the field locations
          unsigned int n_fields      = n_dimensions+nSignals+nIntFields;
          PixIndex                   = n_dimensions+nSignals;
          field_loc                  = new unsigned int[n_fields];

          field_loc[0] = 0;
          for(unsigned int i=1;i<n_fields;i++){
             field_loc[i]=field_loc[i-1]+field_lengths[i-1];
          }
        // set up specific pointers for fast accessors;
          MDPointStride= field_loc[n_fields-1]+field_lengths[n_fields-1];
          pSignal      = field_loc[n_dimensions];
          pError       = field_loc[n_dimensions+1];
          pPixIndex    = field_loc[PixIndex];
  

        }
        // Accessors:;
        T getDataField(unsigned int nField,size_t n_point)const{return *(reinterpret_cast<T *>(pDataBuffer+n_point*MDPointStride+field_loc[nField]));}
        double getSignal(size_t n_point)const{return   *(reinterpret_cast<double *>(pDataBuffer+ n_point*MDPointStride+pSignal));}
        double getError  (size_t n_point)const{return *(reinterpret_cast<double *>(pDataBuffer+ n_point*MDPointStride +pError));}
        uint16_t getIndex(unsigned int nf,size_t n_point)const{return *(reinterpret_cast<uint16_t *>(pDataBuffer+n_point*MDPointStride+field_loc[PixIndex+nf]));}
        // these two are hardcoded for particular type of experiments -> number of runs <~2^9, number of pixels <~ 2^(23)
        uint32_t getNrun(size_t n_point)const{return   0x01FF&(*(reinterpret_cast<uint32_t *>(pDataBuffer+n_point*MDPointStride+pPixIndex)));}
        uint32_t getNPix(size_t n_point)const{return ((0x07FFFF)&(*(reinterpret_cast<uint32_t *>(pDataBuffer+n_point*MDPointStride +pPixIndex)))>>9);}
        // function specialized for Horace data e.g. expetcs no more than 2^9 runs and no more than 2^23 pixels (unique detectors)
        void setData(unsigned int ind,T dim_fields[],double SignalFields[],int iFiels[]){
          unsigned int i,i0;
      
          char *const base = pDataBuffer+ind*MDPointStride;

          memcpy(base,dim_fields,sizeof(T)*nDims);
          i0 = nDims;
          memcpy(base+field_loc[i0],SignalFields,sizeof(double)*nSignals);
          // this part is specialized for particular type of data (indexes ~<2^16)
          i0=PixIndex;
          // compress n_runs and n_pixels into single 32 bit word
          uint32_t *pBuf = (uint32_t *)pWorkingBuf;
          uint32_t  w1   = (uint32_t)iFiels[0];
          uint32_t  w2   = (uint32_t)iFiels[1];
          *pBuf = (w1&0x01FF)|((w2<<9)&(~0x01FF));
          // add proper number of other 16-bit indexes
          for(i=2;i<nIndexes;i++){
            pWorkingBuf[i]=(uint16_t)iFiels[i];
          }
          memcpy(base + field_loc[i0],pWorkingBuf,sizeof(uint16_t)*nIndexes);

        }
        //
        ~MDDataPoint(){
          delete [] field_loc;
          delete [] pWorkingBuf;
        }
      private:
        unsigned int nDims,nSignals,nIndexes;
        char *const  pDataBuffer;
        unsigned int MDPointStride;
        unsigned int *field_loc;
        uint16_t     *pWorkingBuf;
        std::vector<unsigned int> field_lengths;
        unsigned int pSignal,pError,pPixIndex,PixIndex;

      };


//**********************************************************************************************************************
/** Rebinning matrix Internal class which describes rebinning transformation in the terms of current workspace
 */
class transf_matrix
{
public:
        int nDimensions;                   // real number of dimensions in a dataset???
        double rotations[9];                 // rotation matrix for qx,qy,qz coordinates; 
        std::vector<double> trans_bott_left; // shift in all directions (tans_elo is 4th element of transf_bott_left
        std::vector<double> cut_min;        // min limits to extract data;
        std::vector<double> cut_max;       // max limits to extract data;
        std::vector<double> axis_step;     // (cut_max-cut_min)/(nBins);
      
};



class DLLExport MDDataPoints :  public MDImageData
{
public:
     MDDataPoints(unsigned int nDims);
    ~MDDataPoints();
 
     /// check if the pixels are all in memory;
    bool isMemoryBased(void)const{return memBased;}
    /// function returns numnber of pixels contributiong into the MD-data
    size_t getNumPixels(void);


    // Accessors & Mutators used mainly for IO Operations on the dataset
    /// function returns minimal value for dimension i
    double &rMin(unsigned int i){return *(&box_min[0]+i);}
    /// function returns maximal value for dimension i
    double &rMax(unsigned int i){return *(&box_max[0]+i);}
    /// in-out -- returns or allow to set-up the number of data fields in the dataPoint
    unsigned int &numFields(void){return n_fields;}
     /// returns the pointer to the location of the data buffer; the data has to be processed through MDDataPoint then
    void * get_pBuffer(void){return data_buffer;}
    /// return the size of the buffer allocated for pixels
     size_t get_pix_bufSize(void)const{return data_buffer_size;}
    void set_field_length(const std::vector<unsigned int> &in_fields);
    std::vector<unsigned int> get_field_length(void)const{return field_length;}
    std::vector<unsigned int> get_field_start(void)const {return field_start;}
protected:
/// initiates memory for part of the pixels, which should be located in memory;  
    void alloc_pix_array();
  // the parameter identify if the class data are file or memory based
   // usually it is le based and memory used for small datasets, debugging or in a future when PC are big
    bool      memBased;
private:

/// the data, describing the detector pixels
   size_t  n_data_points;  //< number of data points contributing in dataset
   //
   unsigned int n_fields;  //< number of fields contributed into each data point;
   std::vector<double> box_min, //< minimal and 
                       box_max; //< maximal values of ranges the data pixels are in; size is nDimensions
   std::vector<unsigned int> field_length;  //< length each data point in bytes
   std::vector<unsigned int> field_start;   //< location of data points field from the start of the field (roughly sum of field_length-es, but padding has to be considered for efficiency)
   std::vector<std::string>  field_tag;     //< name of each data field in the pixels array;
   //
   size_t  data_buffer_size;              // in pixels (data_points) rather then in char;
   void *data_buffer;

   // boolean values identifying the way to treat NaN-s and Inf-s in the pixel data
   bool   ignore_inf,ignore_nan;     
// private for the time being but may be needed in a future
   MDDataPoints(const MDDataPoints& p);
   MDDataPoints & operator = (const MDDataPoints & other);
// rebin pixels in the pix_aray and add them to the current dataset ;
//    long rebin_dataset4D(const SlicingProperty &transf, const sqw_pixel *pix_array, long nPix_cell);

//    void extract_pixels_from_memCells(const std::vector<long> &selected_cells,long nPix,sqw_pixel *pix_extracted);
};
    }
}
#endif