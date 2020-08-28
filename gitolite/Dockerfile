#
# Build: docker build -m 200m -t gito1 .
#   --build-arg=APT_CACHER=http://172.17.0.2:3142
#   --build-arg=APT_RETAIN_CACHE=1
# docker run -m 200m --cpus=1 -it --rm --name gito gito1
#   -v `pwd`/issuer:/srv/trac/issuer
#   -p 8762:22 -p 8760:8000
#
FROM ubuntu

ARG APT_CACHER
RUN if [ "x${APT_CACHER}" != "x"  ]; then \
    echo "Using apt-cacher ${APT_CACHER}" && \
    (echo 'Acquire::http::Proxy "'${APT_CACHER}'";' >> \
      /etc/apt/apt.conf.d/01proxy); \
    # echo -n "Using apt-cacher: " && cat /etc/apt/apt.conf.d/01proxy; \
  fi

# set localtime to silence install tzdata (mono-runtime)
# add arch i386 before apt update
RUN ln -fs /usr/share/zoneinfo/Asia/Taipei /etc/localtime && \
  dpkg --add-architecture i386 && \
  apt update && apt upgrade -y && \
  apt install -y --no-install-recommends \
    sudo openssh-server vim tmux sqlite3 libsqlite3-dev curl \
    p7zip-full p7zip-rar graphviz \
    build-essential make libc6:i386 zlib1g:i386 \
    subversion git git-lfs \
    mono-runtime \
    openjdk-14-jdk openjdk-8-jdk \
    python python3 virtualenv python-dev python3-dev

ARG APT_RETAIN_CACHE
RUN if [ "x${APT_RETAIN_CACHE}" = "x"  ]; then \
    apt clean && \
    rm -rf /var/lib/apt/lists/*; \
  fi

# setup user
RUN for i in joelai; do \
    useradd -G sudo $i && \
    (echo "$i:$i" | chpasswd); \
  done

# setup ssh
RUN mkdir -p /run/sshd

# setup trac
ARG TRAC_DESTDIR=/srv/trac
ARG TRAC_HTTP_PORT=8000
ENV TRAC_HTTP_PORT=${TRAC_HTTP_PORT}
EXPOSE ${TRAC_HTTP_PORT}

# TracAccountManager/trunc for trac 1.4
RUN virtualenv -p python2 ${TRAC_DESTDIR} && \
  . ${TRAC_DESTDIR}/bin/activate && \
  pip install setuptools jinja2 genshi \
    Trac pysqlite TracGraphviz TracWikiExtras && \
  pip install svn+https://trac-hacks.org/svn/accountmanagerplugin/trunk && \
  easy_install https://trac-hacks.org/svn/tracdragdropplugin/0.12

ADD tmux.conf /root/.tmux.conf
ADD vimrc /root/.vimrc
ADD startup.sh /root/startup.sh

CMD /root/startup.sh
