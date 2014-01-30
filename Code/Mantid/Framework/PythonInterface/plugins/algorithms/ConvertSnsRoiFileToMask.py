"""*WIKI* 

This algorithm reads in an old SNS reduction ROI file and converts it into 
a Mantid mask workspace. It can optionally save that mask to a Mantid 
mask file.

*WIKI*"""

import mantid.simpleapi as msapi
import mantid.api as api
import mantid.kernel as kernel

import os

EXTENSIONS = [".dat", ".txt"]
INSTRUMENTS = ["ARCS", "BASIS", "CNCS", "SEQUOIA"]
TYPE1 = (8, 128)
TYPE2 = (64, 64)
SEQUOIA_OFFSET = 38

class ConvertSnsRoiFileToMask(api.PythonAlgorithm):
    """
    Class to handle reading old SNS reduction ROI files and turning it
    into a Mantid mask workspace.
    """
    def category(self):
        """
        Set the category for the algorithm.
        """
        return "Inelastic;PythonAlgorithms"

    def name(self):
        """
        Name of the algorithm.
        """
        return "ConvertSnsRoiFileToMask"
        
    def PyInit(self):
        self.declareProperty(api.FileProperty(name="SnsRoiFile",
                                              defaultValue="",
                                              action=api.FileAction.Load,
                                              extensions=EXTENSIONS),
                                              "SNS reduction ROI file to load.")
        allowedInstruments = kernel.StringListValidator(INSTRUMENTS)
        self.declareProperty("Instrument", "", 
                             validator=allowedInstruments,
                             doc="One of the following instruments: "+" ".join(INSTRUMENTS))
    
    def PyExec(self):
        self._roiFile = self.getProperty("SnsRoiFile").value
        self._instName = self.getProperty("Instrument").value
        
        # Read in ROI file 
        roi_file = open(self._roiFile)
        id_list = []
        for line in roi_file:
            if line.startswith("#"):
                continue
            id_list.append(self.__get_id(line))
        roi_file.close()
        
        # Make XML DOM for "mask"
        import xml.dom.minidom
        doc = xml.dom.minidom.Document()
        mainnode = doc.createElement("detector-masking")
        doc.appendChild(mainnode)
        grp_node = doc.createElement("group")
        mainnode.appendChild(grp_node)
        det_node = doc.createElement("detids")
        detids = doc.createTextNode(",".join([str(x) for x in id_list]))
        det_node.appendChild(detids)
        grp_node.appendChild(det_node)
        
        # Create temporary "mask" file
        temp_file = "temp.xml"
        fh = open(temp_file, 'w')
        fh.write(doc.toprettyxml())
        fh.close()   
    
        # Load and invert mask
        mask_ws = msapi.LoadMask(InputFile=temp_file, 
                                 Instrument=self._instName)
        mask_ws = msapi.InvertMask(mask_ws)
        
        # Clean up temporary file
        os.remove(temp_file)
    
    def __get_id(self, idx):
        """
        Convert the old bankN_x_y pixel ID into a Mantid index.
        """
        det_size = None
        if self._instName == "BASIS":
            det_size = TYPE2
        else:
            det_size = TYPE1
            
        offset = 1
        if self._instName == "SEQUOIA":
            offset = SEQUOIA_OFFSET
            
        parts = idx.split('_')
        bankid = int(parts[0].split('bank')[-1])
        return int(parts[2]) + det_size[1] * (int(parts[1]) + det_size[0] * (bankid-offset))
        
# Register algorithm with Mantid.
api.AlgorithmFactory.subscribe(ConvertSnsRoiFileToMask)
