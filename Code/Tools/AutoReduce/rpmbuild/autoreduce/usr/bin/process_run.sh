#!/bin/bash

#
#processRun.sh 
#

function process {
# Initialize Overall Return Code, define logfile
status=0
#logfile=~/autoreduce.log
logfile=/var/log/SNS_applications/autoreduce.log

nexusFile=$1
# Pass input data
echo "nexusFile =" $nexusFile | sed "s/^/$(date)  /" >> $logfile

# Parse nexus file path to get facility, instrument...
var=$(echo $nexusFile | awk -F"/" '{print $1,$2,$3,$4,$5,$6}')                    
set -- $var
facility=$1
instrument=$2
proposal=$3
visit=$4
runNumber=$5
echo "facility="$facility",instrument="$instrument",proposal="$proposal",visit="$visit",runNumber="$runNumber | sed "s/^/$(date)  /" >> $logfile

# Create tmp metadata output directory
metadataDir=/tmp/METADATA_XML
if [ ! -d $metadataDir ]; then
  mkdir $metadataDir
  chmod 777 $metadataDir
fi

rawMetadataDir=/tmp/METADATA_XML/raw
reducedMetadataDir=/tmp/METADATA_XML/reduced
if [ ! -d $rawMetadataDir ]; then
  mkdir $rawMetadataDir
  chmod 777 $rawMetadataDir
fi 
if [ ! -d $reducedMetadataDir ]; then
  mkdir $reducedMetadataDir
fi 

# Set raw metadata.xml
metadataFile=$rawMetadataDir"/"$instrument"_"$runNumber".xml"
echo "raw metadata file = "$metadataFile  | sed "s/^/$(date)  /" >> $logfile
# Create reduced metadata.xml
reducedMetadataFile=$reducedMetadataDir"/"$instrument"_"$runNumber".xml"
echo "reduced metadata file = "$reducedMetadataFile  | sed "s/^/$(date)  /" >> $logfile

# Create staging directory for data reduction
redOutDir="/"$facility"/"$instrument"/"$proposal"/shared/autoreduce/"
echo "redOutDir= "$redOutDir  | sed "s/^/$(date)  /" >> $logfile
if [ ! -d $redOutDir ]; then
  mkdir "$redOutDir"
  echo $redOutDir" is created"  | sed "s/^/$(date)  /" >> $logfile
fi


# Create metadata file for raw data
nxingestCommand=nxingest-autoreduce
mappingFile=/etc/autoreduce/mapping.xml
echo $nxingestCommand $mappingFile $nexusFile $metadataFile  | sed "s/^/$(date)  /" >> $logfile
start=`date +%x-%T`
$nxingestCommand $mappingFile $nexusFile $metadataFile | sed "s/^/$(date)  /" >> $logfile
end=`date +%x-%T`

# Accumulate any non-zero return code
status=$(( $status + $? ))
#echo "status=$status"

# Catalog raw metadata
echo "--------Catalogging raw data--------"
icatCommand="java -jar -Djavax.net.ssl.trustStore=/etc/autoreduce/cacerts.jks"
icatJar=/usr/lib64/autoreduce/icat3-xmlingest-client-1.0.0-SNAPSHOT.jar
icatAdmin=snsAdmin
echo $icatCommand $icatJar $icatAdmin $metadataFile
echo "--------Catalogging raw data--------" | sed "s/^/$(date)  /" >> $logfile
echo $icatCommand $icatJar $icatAdmin $metadataFile  | sed "s/^/$(date)  /" >> $logfile
start=`date +%x-%T`
$icatCommand $icatJar $icatAdmin $metadataFile | sed "s/^/$(date)  /" >> $logfile
end=`date +%x-%T`
echo "Started at $start --- Ended at $end"
echo

# Reduce raw data
echo "--------Reducing data--------"
redCommand="python /SNS/"$instrument"/shared/autoreduce/reduce_"$instrument".py"
echo $redCommand $nexusFile $metadataDir
echo $redCommand $nexusFile $metadataDir  | sed "s/^/$(date)  /" >> $logfile
start=`date +%x-%T`
$redCommand $nexusFile $redOutDir &>> $logfile
end=`date +%x-%T`
echo "Started at $start --- Ended at $end"
echo

# Create metadata file for reduced data
echo python /usr/bin/create_reduced_metadata.py $instrument $proposal $visit $runNumber $redOutDir $reducedMetadataFile  | sed "s/^/$(date)  /" >> $logfile
python /usr/bin/create_reduced_metadata.py $instrument $proposal $visit $runNumber $redOutDir $reducedMetadataFile &>> $logfile

# Catalog reduced metadata
echo "--------Catalogging reduced data--------"
echo $icatCommand $icatJar $icatAdmin $reducedMetadataFile
echo $icatCommand $icatJar $icatAdmin $reducedMetadataFile | sed "s/^/$(date)  /" >> $logfile
start=`date +%x-%T`
$icatCommand $icatJar $icatAdmin $reducedMetadataFile | sed "s/^/$(date)  /" >> $logfile
end=`date +%x-%T`
echo "Started at $start --- Ended at $end"
echo

# Accumulate any non-zero return code
status=$(( $status + $? ))
#echo "status="$status
}

if [ -n "$1" ]
  if ! [ -d $1 ] || ! [ -e $1 ]; then
    echo $1 "is not a valid directory or file path."
    exit
  fi 
then
  for nexusFile in `find $1 -name "*event.nxs" -print`
  do
    if [ -e $nexusFile ] && ! [ -h $nexusFile ]; then
      process "$nexusFile"
    fi
  done
else
  echo "--------processRun.sh takes one argument: an archived directory--------"
  echo "You may try one of the followings:"
  echo "./processRun.sh /SNS/PG3/IPTS-6804/"
  echo "./processRun.sh /SNS/PG3/IPTS-6804/0/6160"
  echo "./processRun.sh /SNS/PG3/IPTS-6804/0/6160/NeXus/PG3_6160_event.nxs"
fi
