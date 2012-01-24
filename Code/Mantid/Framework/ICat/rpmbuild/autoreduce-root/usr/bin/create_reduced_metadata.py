import pprint
 
import sys 
import os 
import glob 
from xml.dom.minidom import Node
from xml.dom.minidom import Document
from time import gmtime, strftime
from xml.dom.ext import PrettyPrint
from StringIO import StringIO

if (len(sys.argv) < 6):
  sys.exit("Error: The script takes 5 arguments: instrument, inv_number, visit_id, run_number, directory_of_reduced_data, reduced_metadata_file")

instrument_text = sys.argv[1]
inv_number_text= sys.argv[2]
visit_id_text = sys.argv[3]
run_number_text = sys.argv[4]
directory = sys.argv[5]
output_file = sys.argv[6]
print instrument_text, inv_number_text, visit_id_text, run_number_text, directory, output_file

# Create the minidom document
doc = Document()

# Create the <wml> base element
icat = doc.createElement("icat")
icat.setAttribute("version", "4.0")
doc.appendChild(icat)

study = doc.createElement("study")
icat.appendChild(study)
 
investigation = doc.createElement("investigation")
study.appendChild(investigation)

inv_number = doc.createElement("inv_number")
investigation.appendChild(inv_number)
inv_number.appendChild(doc.createTextNode(inv_number_text))

visit_id = doc.createElement("visit_id")
investigation.appendChild(visit_id)
visit_id.appendChild(doc.createTextNode(visit_id_text))

instrument = doc.createElement("instrument")
investigation.appendChild(instrument)
instrument.appendChild(doc.createTextNode(instrument_text))

title = doc.createElement("title")
investigation.appendChild(title)
title.appendChild(doc.createTextNode("DEFAULT"))

dataset = doc.createElement("dataset")
investigation.appendChild(dataset)

name = doc.createElement("name")
dataset.appendChild(name)
name.appendChild(doc.createTextNode("Default"))

dataset_type = doc.createElement("dataset_type")
dataset.appendChild(dataset_type)
dataset_type.appendChild(doc.createTextNode("REDUCED"))

listing = glob.glob(os.path.join(directory, "*"))
for infile in listing:
  datafile = doc.createElement("datafile")
  dataset.appendChild(datafile)

  name = doc.createElement("name")
  datafile.appendChild(name)
  name.appendChild(doc.createTextNode(os.path.basename(infile)))

  location = doc.createElement("location")
  datafile.appendChild(location)
  location.appendChild(doc.createTextNode(infile))
  print "current file path is: " + infile
  print "current file name is: " + os.path.basename(infile)

  parameter = doc.createElement("parameter")
  datafile.appendChild(parameter)

  name = doc.createElement("name")
  parameter.appendChild(name)
  name.appendChild(doc.createTextNode("run_number"))

  numeric_value = doc.createElement("numeric_value")
  parameter.appendChild(numeric_value)
  numeric_value.appendChild(doc.createTextNode(run_number_text))

  units = doc.createElement("units")
  parameter.appendChild(units)
  units.appendChild(doc.createTextNode("N/A"))

# Print our newly created XML
tmpStream = StringIO()
PrettyPrint(doc, stream=tmpStream, encoding="utf-8")
print tmpStream.getvalue()

fout = open(output_file, "w")
fout.write(tmpStream.getvalue())
fout.close();
