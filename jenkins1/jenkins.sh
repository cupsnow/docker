#!/bin/bash

[ -n "${JENKINS_HOME}" ] || JENKINS_HOME="${HOME}"
[ -n "${JENKINS_SLAVE_AGENT_PORT}" ] || JENKINS_SLAVE_AGENT_PORT=50000
[ -n "${JENKINS_HTTP_PORT}" ] || JENKINS_HTTP_PORT=8080
[ -n "${JENKINS_WAR}" ] || JENKINS_WAR="/usr/lib/jenkins/jenkins.war"

java -Djenkins.model.Jenkins.slaveAgentPort=$JENKINS_SLAVE_AGENT_PORT \
  -Duser.home=$JENKINS_DATA -jar $JENKINS_WAR --httpPort=$JENKINS_HTTP_PORT

