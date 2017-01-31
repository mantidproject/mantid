# this assumes that Anaconda Python 2.7 is NOT present on the system
wget https://repo.continuum.io/archive/Anaconda2-4.2.0-Linux-x86_64.sh
bash Anaconda2-4.2.0-Linux-x86_64.sh

# install scipy 1.18
conda install -c anaconda scipy=0.18.1 --yes

# install numpy 1.11
conda install numpy

# install astra 1.8
conda install -c astra-toolbox astra-toolbox --yes

# install tomopy 0.1.11
conda install --channel https://conda.anaconda.org/ericdill tomopy

# TODO pyfits