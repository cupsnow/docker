FROM ubuntu

RUN  echo 'Acquire::http::Proxy "http://172.17.0.2:3142";' >> \
  /etc/apt/apt.conf.d/01proxy

# set localtime to silence install tzdata (mono-runtime)
# add arch i386 before apt update
RUN ln -fs /usr/share/zoneinfo/Asia/Taipei /etc/localtime && \
  dpkg --add-architecture i386 && \
  apt update && apt upgrade -y && \
  apt install -y --no-install-recommends \
    openssh-server vim tmux openjdk-11-jdk git curl p7zip-full p7zip-rar \
    git-lfs make mono-runtime python python3 libc6:i386 zlib1g:i386 && \
  rm -rf /var/lib/apt/lists/* && \
  mkdir -p /run/sshd

ARG user=jenkins
ARG group=jenkins

RUN groupadd ${group} && \
  useradd -d "/${user}" \
    -g `getent group ${group} | cut -d: -f3` \
    -m -s /bin/bash ${user} && \
  chown -R `id -u ${user}`:`id -g ${group}` "/${user}"

ARG JENKINS_URL="http://mirrors.jenkins.io/war-stable/latest/jenkins.war"
ARG JENKINS_PREFIX
ARG JENKINS_WAR

ENV JENKINS_PREFIX="${JENKINS_PREFIX:-/usr/lib/jenkins}"
ENV JENKINS_WAR="${JENKINS_WAR:-${JENKINS_PREFIX}/jenkins.war}"

RUN curl -L --create-dirs --output "$JENKINS_WAR" "$JENKINS_URL" && \
  chmod -R 0755 "`dirname ${JENKINS_WAR}`"

ADD jenkins.sh "${JENKINS_PREFIX}/"
RUN chmod 0775 "${JENKINS_PREFIX}/jenkins.sh"

ADD tmux.conf /root/.tmux.conf
ADD vimrc /root/.vimrc

ARG JENKINS_HTTP_PORT
ARG JENKINS_SLAVE_AGENT_PORT

ENV JENKINS_HTTP_PORT=${JENKINS_HTTP_PORT:-8080}
ENV JENKINS_SLAVE_AGENT_PORT=${JENKINS_SLAVE_AGENT_PORT:-50000}

EXPOSE ${JENKINS_HTTP_PORT}
EXPOSE ${JENKINS_SLAVE_AGENT_PORT}
VOLUME "/${user}"

USER "${user}"

ENV JENKINS_HOME="/${user}/jenkins-data"

RUN echo "JENKINS_PREFIX: ${JENKINS_PREFIX}"
RUN echo "JENKINS_HOME: ${JENKINS_HOME}"
RUN echo "user: ${user}"
RUN echo "JENKINS_WAR: ${JENKINS_WAR}"

CMD "${JENKINS_PREFIX}/jenkins.sh"

USER root
