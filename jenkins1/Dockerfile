FROM ubuntu

RUN  echo 'Acquire::http::Proxy "http://172.17.0.2:3142";' >> \
  /etc/apt/apt.conf.d/01proxy
RUN apt-get update && apt-get upgrade -y && \
  apt-get install -y --no-install-recommends \
    openssh-server vim tmux openjdk-11-jdk git curl p7zip-full p7zip-rar \
    git-lfs make && \
  rm -rf /var/lib/apt/lists/*

RUN mkdir -p /run/sshd

ARG user=jenkins
ARG group=jenkins
ARG JENKINS_HOME="/jenkins"

RUN mkdir -p "`dirname ${JENKINS_HOME}`" && \
  groupadd ${group} && \
  useradd -d "${JENKINS_HOME}" \
    -g `getent group ${group} | cut -d: -f3` \
    -m -s /bin/bash ${user} && \
    chown -R `id -u ${user}`:`id -g ${group}` "${JENKINS_HOME}"

ARG JENKINS_PREFIX="/usr/lib/jenkins"
ARG JENKINS_WAR="${JENKINS_PREFIX}/jenkins.war"
ARG JENKINS_URL="http://mirrors.jenkins.io/war-stable/latest/jenkins.war"

RUN curl -L --create-dirs --output "$JENKINS_WAR" "$JENKINS_URL" && \
  chmod -R 0755 `dirname ${JENKINS_WAR}`

ADD jenkins.sh ${JENKINS_PREFIX}/
RUN chmod 0775 "${JENKINS_PREFIX}/jenkins.sh"

ARG JENKINS_HTTP_PORT=8080
ARG JENKINS_SLAVE_AGENT_PORT=50000

EXPOSE ${JENKINS_HTTP_PORT}
EXPOSE ${JENKINS_SLAVE_AGENT_PORT}
VOLUME ${JENKINS_HOME}

# USER ${user}

ENV JENKINS_HTTP_PORT=${JENKINS_HTTP_PORT}
ENV JENKINS_SLAVE_AGENT_PORT=${JENKINS_SLAVE_AGENT_PORT}
ENV JENKINS_HOME="${JENKINS_HOME}"
ENV JENKINS_WAR="${JENKINS_WAR}"

