import pprint, time, sys, os, glob 
from xml.dom.minidom import Node
from xml.dom.minidom import Document
from xml.dom.minidom import parseString 
from time import gmtime, strftime
from xml.dom.ext import PrettyPrint
from StringIO import StringIO
from stat import * # ST_MTIME etc

def modification_date(filename):
  st = os.stat(filename)
  file_date = time.strftime("%Y-%m-%dT%H:%M:%S",time.localtime(st[ST_MTIME]))
  return file_date

if (len(sys.argv) < 3):
  sys.exit("Error: The script takes 3 arguments: pre_nexus_dir, in_out_raw_metadata_file")
run_number = sys.argv[1]
pre_nexus_dir = sys.argv[2]
in_out_raw_metadata_file = sys.argv[3]

print pre_nexus_dir, in_out_raw_metadata_file 

# Create the minidom document
file = open(in_out_raw_metadata_file, 'r')
data = file.read()
file.close()
doc = parseString(data)

dataset = doc.getElementsByTagName('dataset')[0]
ref_datafile = doc.getElementsByTagName('datafile')[0]

listing = os.listdir(pre_nexus_dir)
for infile in listing:
  datafile = doc.createElement("datafile")
  dataset.insertBefore(datafile, ref_datafile)

  name = doc.createElement("name")
  datafile.appendChild(name)
  name.appendChild(doc.createTextNode(os.path.basename(infile)))

  location = doc.createElement("location")
  datafile.appendChild(location)
  location.appendChild(doc.createTextNode(pre_nexus_dir+os.path.basename(infile)))

  format = doc.createElement("datafile_format")
  datafile.appendChild(format)
  format.appendChild(doc.createTextNode("preNexus"))

  format_version = doc.createElement("datafile_format_version")
  datafile.appendChild(format_version)
  format_version.appendChild(doc.createTextNode("1.0"))

  create_time = doc.createElement("datafile_create_time")
  datafile.appendChild(create_time)
  d = modification_date(pre_nexus_dir+os.path.basename(infile))
  create_time.appendChild(doc.createTextNode(d))

  parameter = doc.createElement("parameter")
  datafile.appendChild(parameter)

  name = doc.createElement("name")
  parameter.appendChild(name)
  name.appendChild(doc.createTextNode("run_number"))

  numeric_value = doc.createElement("numeric_value")
  parameter.appendChild(numeric_value)
  numeric_value.appendChild(doc.createTextNode(run_number))

  units = doc.createElement("units")
  parameter.appendChild(units)
  units.appendChild(doc.createTextNode("N/A"))


# Print our newly created XML
tmpStream = StringIO()
PrettyPrint(doc, stream=tmpStream, encoding="utf-8")
print tmpStream.getvalue()

fout = open(in_out_raw_metadata_file, "w")
fout.write(tmpStream.getvalue())
fout.close();

