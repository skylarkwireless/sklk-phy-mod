#!/bin/sh

# You may need to log in if the docker push fails.
# docker login -u <user> artifactory.ad.sklk.us/gitlabdocker/skylark-wireless

progname=$0
progdir=$(dirname "${progname}")

docker build -t "artifactory.ad.sklk.us/gitlabdocker/skylark-wireless/software/sklk-phy-mod:latest" ${progdir} || exit 1
docker push "artifactory.ad.sklk.us/gitlabdocker/skylark-wireless/software/sklk-phy-mod:latest"
