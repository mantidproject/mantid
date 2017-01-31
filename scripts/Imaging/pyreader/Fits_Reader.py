import numpy as np
import warnings
import os
import pyfits
import glob



def fits_reader(data_file_path,data_file_prefix,white_file_name=None, dark_file_name=None, projection_file_name=None,start=0,end=-1):
    """
    Read Fits data into tomopy
    """

    data_files_list = glob.glob(data_file_path+data_file_prefix+"*.fits")
    data_files_list.sort()

    if len(data_files_list) == 0:
	print "Error: Cannot find the data files at "+data_file_path
	return [],[],[],[]
    if end==-1:
      data_files_list = data_files_list[start:]
    else:
      data_files_list = data_files_list[start:end]

    data_0=pyfits.open(data_files_list[0])
    data = np.zeros((len(data_files_list),)+data_0[0].data.shape)
    idx = 0
    for data_file_name in data_files_list:
      data_tmp = pyfits.open(data_file_name)
      data[idx,:,:] = data_tmp[0].data
      idx = idx+1
 
    white = None
    if white_file_name != None:
      data_tmp = pyfits.open(white_file_name)
      white = data_tmp[0].data
      white = white.reshape((1,)+white.shape)

    dark = None
    if dark_file_name != None:
      data_tmp = pyfits.open(dark_file_name)
      dark = data_tmp[0].data
      dark = dark.reshape((1,)+dark.shape)
   
    projection=[None]*len(data_files_list)
    if projection_file_name != None:
      proj_file = open(projection_file_name,'r')
      for line in proj_file:
        line = line.strip()
        columns = line.split()
        projection[int(columns[0])] = columns[1]
    else:
      for idx in range(0,len(data_files_list)):
        projection[idx] = idx*180.0/len(data_files_list)
    return data, white, dark, projection
