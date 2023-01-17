#!/bin/sh
#####################################################################
# Starts the Jenkins slave process if it is not already running.
# It also downloads the slave.jar to /tmp if it does not already
# exist.
#
# The variables in the User configuration section below
# must be updated for each agent.
#####################################################################
# Setting up a scheduled task using crontab to ensure agent stays
# connected
#####################################################################
# Download this script to a path under the account that will run the agent
# Run "crontab -e" under the user that will connect to jenkins and add
# a line such as
#
# 0,5 * * * * <downloaded path>/jenkins-slave.sh nodename secret
#
# If you have installed a newer version of Java then this can be used
# by specifying the JAVA environment variable on the line above, e.g.
#
# 0,5 * * * * env JAVA=<full-path-to-java-executable> <downloaded path>/jenkins-slave.sh nodename secret
#
# Note that for the Adoptium Java releases the java path is usually something
# like /Library/Java/JavaVirtualMachines/temurin-11.jre/Contents/Home/bin/java
# It may also be necessary to customize the PATH for the crontab entry if
# other software has been installed in /usr/local for example,
#
# 0,5 * * * * env PATH=<path1>:<path2>:$PATH JAVA=<full-path-to-java-executable> <downloaded path>/jenkins-slave.sh nodename secret
#####################################################################
# User configuration
#####################################################################
# Main url for Jenkins
JENKINS_URL=https://builds.mantidproject.org
# URL for proxy (if required)
PROXY_HOST=${3}
# Port for proxy (if required)
PROXY_PORT=${4}
# Name of the node as it appears on jenkins
NODE_NAME=${1}
# Copy the secret from the command line given at the above URL
SECRET=${2}

#####################################################################
# Constants
#####################################################################
# URL of jnlp file for slave
SLAVE_AGENT_URL="${JENKINS_URL}/computer/${NODE_NAME}/slave-agent.jnlp"
# name of the slave jar - full path is determined later
JAR_FILE=agent.jar
# Some versions of cron don't set the USER environment variable
# required by vnc
[ -z "$USER" ] && export USER=$(whoami)
# Put /usr/local/bin on the PATH if brew is installed
[ -f /usr/local/bin/brew ] && export PATH=${PATH}:/usr/local/bin
# Put /usr/texbin on the PATH if latex is installed
[ -f /usr/texbin/latex ] && export PATH=${PATH}:/usr/texbin

#####################################################################
# Script
#####################################################################
# exit if it is already running
RUNNING=$(ps u -U $(whoami) | grep java | grep ${JAR_FILE})
if [ ! -z "${RUNNING}" ]; then
  echo "Slave process is already running"
  exit 0
else
  echo "Slave process is not running"
fi

# error out if there isn't a node name and secret
if [ "$#" -lt 2 ]; then
  echo "Usage: `basename ${0}` <NODE_NAME> <SECRET> [PROXY_HOST] [PROXY_PORT]"
  exit -1
fi

# setup the proxy
if [ ! -z "${PROXY_HOST}" ]; then
  PROXY_ARGS="-Dhttps.proxyHost=${PROXY_HOST} -Dhttps.proxyPort=${PROXY_PORT}"
  echo "using proxy ${PROXY_HOST} ${PROXY_PORT}"
  # For curl
  export http_proxy=http://$PROXY_HOST:$PROXY_PORT
  export https_proxy=https://$PROXY_HOST:$PROXY_PORT
fi

# find the jar file if it exists
if [ -f ${HOME}/jenkins-linode/${JAR_FILE} ]; then
  JAR_FILE=${HOME}/jenkins-linode/${JAR_FILE}
elif [ -f ${HOME}/Jenkins/${JAR_FILE} ]; then
  JAR_FILE=${HOME}/Jenkins/${JAR_FILE}
else
  JAR_FILE_TMP=/tmp/${JAR_FILE}
  if [ ! -f ${JAR_FILE_TMP} ]; then
    echo "Downloading slave jar file to ${JAR_FILE_TMP}"
    if [ $(command -v curl) ]; then
      echo "curl --location -o ${JAR_FILE_TMP} ${JENKINS_URL}/jnlpJars/${JAR_FILE}"
      curl -o ${JAR_FILE_TMP} ${JENKINS_URL}/jnlpJars/${JAR_FILE}
    else
      echo "Need curl to download ${JENKINS_URL}/jnlpJars/${JAR_FILE}"
      exit -1
    fi
  fi
  JAR_FILE=${JAR_FILE_TMP}
fi

echo "starting ..."
if [ -z "${JAVA}" ]; then
  JAVA=`which java`
fi
JAVA_ARGS="${PROXY_ARGS} -jar ${JAR_FILE} -jnlpUrl ${SLAVE_AGENT_URL} -secret ${SECRET}"
echo "${JAVA} ${JAVA_ARGS}"
${JAVA} ${JAVA_ARGS}
