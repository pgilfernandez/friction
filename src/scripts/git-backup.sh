#!/bin/bash
set -e -x

CWD=`pwd`
DIR=${CWD}/backup
DATE=`date +%s`
URL=https://github.com/friction2d
BACKUP_DIR=${DIR}/friction-git-backup-${DATE}
REPOS="
friction
friction2d.github.io
skia
gperftools
sfntly
friction-shader-plugins
mxe
"

mkdir -p ${BACKUP_DIR}
cd ${BACKUP_DIR}
for repo in ${REPOS}; do
    git clone --mirror ${URL}/${repo}.git
done