#pylint: disable=no-init,invalid-name
import mantid.simpleapi as msapi
import mantid.api as api
import mantid.kernel as kernel
from mantid import config

import os

EXTENSIONS = [".dat", ".txt"]
INSTRUMENTS = ["ARCS", "BASIS", "CNCS", "SEQUOIA"]
TYPE1 = (8, 128)
TYPE2 = (64, 64)

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

    def summary(self):
        return "This algorithm reads in an old SNS reduction ROI file and converts it into a Mantid mask workspace."

    def PyInit(self):
        """
        Set the algorithm properties.
        """
        self.declareProperty(api.FileProperty(name="SnsRoiFile",
                                              defaultValue="",
                                              action=api.FileAction.Load,
                                              extensions=EXTENSIONS),\
                                              "SNS reduction ROI file to load.")
        allowedInstruments = kernel.StringListValidator(INSTRUMENTS)
        self.declareProperty("Instrument", "",
                             validator=allowedInstruments,
                             doc="One of the following instruments: "+" ".join(INSTRUMENTS))
        self.declareProperty("OutputFilePrefix", "",
                             "Overrides the default filename for the output "\
                             +"file (Optional). Default is <inst_name>_Mask.")
        self.declareProperty(api.FileProperty(name="OutputDirectory",
                                              defaultValue=config['defaultsave.directory'],
                                              action=api.FileAction.Directory),\
                                              "Directory to save mask file."\
                                              +" Default is current Mantid save directory.")

    def PyExec(self):
        """
        Execute the algorithm.
        """
        self._roiFile = self.getProperty("SnsRoiFile").value
        self._instName = self.getProperty("Instrument").value
        self._filePrefix = self.getProperty("OutputFilePrefix").value
        self._outputDir = self.getProperty("OutputDirectory").value

        # Read in ROI file
        roi_file = open(self._roiFile)
        id_list = []
        for line in roi_file:
            if line.startswith("#"):
                continue
            id_list.append(self._get_id(line))
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

        # Save mask to a file
        if self._filePrefix == "":
            self._filePrefix = self._instName + "_Mask"

        output_file = os.path.join(self._outputDir, self._filePrefix)
        msapi.SaveMask(mask_ws, OutputFile=output_file+".xml")

    def _get_id(self, idx):
        """
        Convert the old bankN_x_y pixel ID into a Mantid index.
        """
        det_size = None
        if self._instName == "BASIS":
            det_size = TYPE2
        else:
            det_size = TYPE1

        parts = idx.split('_')
        bankid = int(parts[0].split('bank')[-1])
        return int(parts[2]) + det_size[1] * (int(parts[1]) + det_size[0] * (bankid-1))

# Register algorithm with Mantid.
api.AlgorithmFactory.subscribe(ConvertSnsRoiFileToMask)
