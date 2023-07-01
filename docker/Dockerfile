FROM artifactory.ad.sklk.us/gitlabdocker/skylark-wireless/software/snapcraft/focal:latest

RUN apt update
RUN apt install -y -qq curl
RUN curl http://10.204.0.164/postkickstart/certs/skylark-wireless.pem > /usr/local/share/ca-certificates/skylark-wireless.crt
RUN update-ca-certificates
RUN apt install -y gnupg2
RUN curl -fsSL https://releases.jfrog.io/artifactory/api/gpg/key/public | apt-key add -
RUN echo "deb https://releases.jfrog.io/artifactory/jfrog-debs xenial contrib" | tee -a /etc/apt/sources.list
RUN echo "deb https://fabricio.ad.sklk.us/artifactory/sklk-debian focal main" | tee -a /etc/apt/sources.list
RUN curl https://fabricio.ad.sklk.us:443/artifactory/api/security/keypair/sklk-debian-keys/public | sudo apt-key add -
RUN sudo apt update
RUN apt install -y -qq cmake g++
RUN apt install -y -qq autoconf automake
RUN apt install -y -qq nlohmann-json3-dev libyaml-cpp-dev libzmq3-dev libarmadillo-dev libhdf5-dev
RUN apt install -y -qq libboost-program-options-dev libboost-filesystem-dev
RUN apt install -y -qq python3 libpython3-dev python3-distutils python3-pip git
RUN apt install -y -qq swig
RUN apt install -y -qq lsb-release fakeroot
RUN apt install -y -qq jfrog-cli-v2
RUN apt install -y -qq doxygen
RUN apt install -y -qq graphviz
RUN apt install -y -qq texlive-latex-extra texlive-font-utils

RUN pip3 install numpy tabulate
RUN pip install twine
