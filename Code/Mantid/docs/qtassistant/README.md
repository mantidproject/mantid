mycollection.qhcp        - main collection file that has generic about information
myapplication-manual.qhp - information about an individual manual
mycollection.qhc         - final output file to use
myapplication-manual.qch - cache file for the applications help info

To generate the help run

 qcollectiongenerator mycollection.qhcp -o mycollection.qhc

To view use

 assistant-qt4 -collectionFile mycollection.qhc
