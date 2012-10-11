#!/bin/bash

#
#processRun.sh 
#

function process {
# Initialize Overall Return Code, define logfile
export NEXUSLIB=/usr/lib64/libNeXus.so

status=0
logfile=/var/log/SNS_applications/autoreduce.log

nexusFile=$1
# Pass input data
echo 
echo "=========Post Processing========="
echo "=========Post Processing=========" | sed "s/^/$(date)  /" >> $logfile
echo "nexusFile =" $nexusFile | sed "s/^/$(date)  /" >> $logfile

# Parse nexus file path to get facility, instrument...
var=$(echo $nexusFile | awk -F"/" '{print $1,$2,$3,$4,$5,$6}')                    
set -- $var
facility=$1
instrument=$2
proposal=$3
filename=$5
var2=$(echo $filename | awk -F"." '{print $1}')                    
set -- $var2
run=$1
var3=$(echo $run | awk -F"_" '{print $1, $2}')                    
set -- $var3
runNumber=$2

echo "facility="$facility",instrument="$instrument",proposal="$proposal",runNumber="$runNumber | sed "s/^/$(date)  /" >> $logfile

icatConfig=/etc/autoreduce/icatclient.properties
if [ -f $icatConfig ]; then
  hostAndPort=`awk -F "=" '/hostAndPort/ { print $2 }' $icatConfig`
  password=`awk -F "=" '/password/ { print $2 }' $icatConfig`
else 
  hostAndPort=icat-testing.sns.gov:8181
  password=password
fi

plugin=db

# Accumulate any non-zero return code
status=$(( $status + $? ))
#echo "status=$status"


# Catalog raw metadata
echo "--------Cataloging raw data--------"
ingestNexus_adara=/usr/bin/ingestNexus_adara
echo $ingestNexus_adara $nexusFile
echo "--------Cataloging raw data--------" | sed "s/^/$(date)  /" >> $logfile
echo $ingestNexus_adara $nexusFile | sed "s/^/$(date)  /" >> $logfile
start=`date +%x-%T`
$ingestNexus_adara $nexusFile $plugin $hostAndPort $password | sed "s/^/$(date)  /" >> $logfile
end=`date +%x-%T`
echo "Started at $start --- Ended at $end"
echo

# Create staging directory for data reduction
redOutDir="/"$facility"/"$instrument"/"$proposal"/shared/autoreduce/"
echo "redOutDir= "$redOutDir | sed "s/^/$(date) /" >> $logfile
if [ ! -d $redOutDir ]; then
  mkdir -p "$redOutDir"
  echo $redOutDir" is created" | sed "s/^/$(date) /" >> $logfile
fi

# Reduce raw data
echo "--------Reducing data--------"
reduce_script="/SNS/"$instrument"/shared/autoreduce/reduce_"$instrument".py"
if [ ! -f $reduce_script ];
then
  echo "$reduce_script does not exist, exiting..."
  return 
fi
redCommand="python $reduce_script"
echo $redCommand $nexusFile $redOutDir
echo "--------Reducing data--------" | sed "s/^/$(date)  /" >> $logfile
echo $redCommand $nexusFile $redOutDir  | sed "s/^/$(date)  /" >> $logfile
start=`date +%x-%T`
$redCommand $nexusFile $redOutDir &>> $logfile
OUT=$?
if [ $OUT -eq 1 ];then
  errorLog=$redOutDir$instrument"_"$runNumber".error"
  echo "Auto reduction failed, see error log at "$errorLog
  echo "Auto reduction failed, see error log at "$errorLog >> $logfile
  return 
fi
end=`date +%x-%T`
echo "Started at $start --- Ended at $end"
echo

# Catalog reduced metadata
echo "--------Cataloging reduced data--------"
ingestReduced_adara=/usr/bin/ingestReduced_adara
echo $ingestReduced_adara $facility $instrument $proposal $runNumber
echo "--------Cataloging reduced data--------" | sed "s/^/$(date)  /" >> $logfile
echo $ingestReduced_adara $facility $instrument $proposal $runNumber | sed "s/^/$(date)  /" >> $logfile
start=`date +%x-%T`
$ingestReduced_adara $facility $instrument $proposal $runNumber $plugin $hostAndPort $password | sed "s/^/$(date)  /" >> $logfile
end=`date +%x-%T`
echo "Started at $start --- Ended at $end"
echo

# Accumulate any non-zero return code
status=$(( $status + $? ))
#echo "status="$status
}

if [ -n "$1" ] && ! [ -d $1 ] && [ -e $1 ]  && ! [ -h $1 ]; then
  echo "Got nexus file: " $1
  process "$1"
else
  echo "--------processRun.sh takes one argument: an archived directory--------"
  echo "You may try one of the followings:"
  echo "./processRun.sh /SNS/HYSA/IPTS-6804/nexus/HYSA_363.nxs.h5"
fi
