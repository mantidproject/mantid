import numpy as np
import sys
import os
import h5py
import glob

import fits_to_nexus as fits
import tif_to_nexus as tif
def read_data(input_dir,prefix,white,dark,start_slice=None,end_slice=None,start_angle=0.0,end_angle=180.0):
    if start_angle is None:
	start_angle = 0.0
    if end_angle is None:
	end_angle = 180.0
    datasw = None
    data_white = None
    data_dark = None
    projection_angles = None
    pathname = os.path.join(input_dir,'input.nxs')
    if os.path.isfile(pathname):
        nexus = h5py.File(pathname,'r')
        data = nexus["entry1/tomo_entry/instrument/detector/data"]
        data_dark = data[-1,:,:]
        data_white  = data[-2,:,:]
        datasw = data[:-2,:,:]
        data_dark = data_dark.reshape((1,data.shape[1],data.shape[2],))
        data_white = data_white.reshape((1,data.shape[1],data.shape[2],))
	if start_slice is None:
		start_slice = 0
	if end_slice is None:
		end_slice = datasw.shape[0]
	datasw = datasw[start_slice:end_slice,:,:]
    elif has_files(input_dir,'fits'):
        datasw, data_white, data_dark = fits.fits_to_nexus(input_dir,prefix,white,dark,pathname)
    elif has_files(input_dir,'tif') or has_files(input_dir,'tiff'):
	datasw, data_white, data_dark = tif.tif_to_nexus(input_dir,prefix,white,dark,pathname)
    projection_angles = np.linspace(start_angle,end_angle, num=datasw.shape[0])
    return datasw, data_white, data_dark, projection_angles


def has_files(input_dir, suffix):
    endsuffix = '*.'+suffix
    pathname = os.path.join(input_dir,endsuffix)
    files = glob.glob(pathname)
    if files == []:
      return False
    else:
      return True
    


