import h5py
import numpy as np
import logging
import argparse
import tif_reader as tif

def tif_to_nexus(input_dir,prefix,white,dark,output_file):
	"""
	eg: fits_to_nexus('/work/imat/tomo_data_CCD/16 dec 2013 Test Wired/sample/','TestWires-','/work/imat/tomo_data_CCD/16 dec 2013 Test Wired/beam/Beam-0001.fits','/work/imat/tomo_data_CCD/16 dec 2013 Test Wired/dark/Dark2.fits',
		          '/work/imat/output.nxs') 
	"""
	logger = logging.getLogger(__name__)
	print 'Reading the tif/tiff files in the directory'
	data, white, dark, theta = tif.tif_reader(input_dir,prefix,white,dark)
	try:
		nexus_output = h5py.File(output_file,"w")

#white = np.reshape(white,(1,)+white.shape)
#dark = np.reshape(dark,(1,)+dark.shape)
		print 'White field data shape',white.shape
		print 'Dark field data shape',dark.shape
		print 'Projection data shape',data.shape
		data = np.append(data,white,axis=0)
		data = np.append(data,dark,axis=0)
	
		image_key = np.zeros(len(theta)+2)
		image_key[-2]=1
		image_key[-1]=2
		theta.append(theta[0])
		theta.append(theta[0])

		print 'Writing to nexus file',output_file
		dset = nexus_output.create_dataset("entry1/tomo_entry/instrument/detector/data",data.shape,dtype="f")
		dset[...]=data
		rangle = nexus_output.create_dataset("entry1/tomo_entry/sample/rotation_angle",(len(theta),),dtype="f")
		rangle[...]=theta
		img_key = nexus_output.create_dataset("entry1/tomo_entry/instrument/detector/image_key",image_key.shape,dtype="f")
		img_key[...]=image_key
		control = nexus_output.create_dataset("entry1/tomo_entry/control/data",image_key.shape,dtype="f")
		control[...]=np.ones(image_key.shape)
	except IOError:
		print 'Warning: Cannot create input.nxs file in the input directory.'
	return data, white, dark

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument("-i","--input_dir", help="Input directory for the projection files")
	parser.add_argument("-o","--output_file", help="Output nexus file full path")
	parser.add_argument("-p","--prefix", help="Input file prefix")
	parser.add_argument("-w","--white", help="White field file")
	parser.add_argument("-d","--dark", help="Dark field file")
	args = parser.parse_args()
	fits_to_nexus(args.input_dir,args.prefix,args.white,args.dark,args.output_file)
