#!/bin/bash
#
#
NAME=tcp_rmtlog_be
PROJECT_DESC="TCP Remote Logging Plugin"
PACKAGER="Hien Nguyen <nguyhien@us.ibm.com>"

VERSION=1.5.1
RELEASE=1
arch=i386
BUILDRPM_START_DIR=`pwd`
SRC=""

function usage {
	echo 
	echo "Usage:"
	echo "      bld_tcp_rmtlog_be.sh [-a <architecture] [-v <version>]"
	echo
	exit 0
}

while getopts "a:v:h" OPT; do
	case $OPT in
		a) arch=$OPTARG ;;
		v) VERSION=$OPTARG ;;
		h) usage ;;
		*) usage ;;
	esac
done

echo "$VERSION  $arch"

if [ -e ${HOME}/srctmp ]
then
	rm -rf ${HOME}/srctmp
fi
if [ -e ${HOME}/rpmbuild ]
then
	rm -rf ${HOME}/rpmbuild
fi

mkdir ${HOME}/srctmp
cd ${HOME}/srctmp
mkdir -p ${NAME}-${VERSION}/kernel
mkdir -p ${NAME}-${VERSION}/user
mkdir -p ${NAME}-${VERSION}/user/cmd

if [ "$SRC" = "cvs" ]
then
#
# get the source from cvs
#
cvs export -r HEAD -d ${NAME}-${VERSION} ${NAME}
#
# get source from current project dir
#
else
#cp -rf $BUILDRPM_START_DIR ${NAME}-${VERSION}
cp -rf $BUILDRPM_START_DIR/kernel/v2.4.20 ${NAME}-${VERSION}/kernel
cp -rf $BUILDRPM_START_DIR/user/lib ${NAME}-${VERSION}/user
cp -rf $BUILDRPM_START_DIR/user/include ${NAME}-${VERSION}/user
cp -rf $BUILDRPM_START_DIR/user/cmd/evlogd ${NAME}-${VERSION}/user/cmd
fi

cp ${NAME}-${VERSION}/user/cmd/evlogd/${NAME}/${NAME}.spec ${HOME}/srctmp
#
# tar it up
#
tar cvfz ${NAME}-${VERSION}.tar.gz ${NAME}-${VERSION}

#
# create rpm build environment
#
RPM_TOP_DIR=${HOME}/rpmbuild
mkdir -p ${RPM_TOP_DIR}/BUILD
mkdir ${RPM_TOP_DIR}/SOURCES
mkdir ${RPM_TOP_DIR}/SPECS
mkdir ${RPM_TOP_DIR}/RPMS
mkdir ${RPM_TOP_DIR}/SRPMS
#
# setup RPM topdir
#
if [ -e ${HOME}/.rpmrc ]
then
	mv ${HOME}/.rpmrc ${HOME}/rpmrc.ORG
fi
echo "macrofiles: /usr/lib/rpm/macros:~/.rpmmacros" > ${HOME}/.rpmrc

if [ -e ${HOME}/.rpmmacros ]
then
	mv ${HOME}/.rpmmacros ${HOME}/rpmmacros.ORG
fi
echo "%_topdir ${RPM_TOP_DIR}" > ${HOME}/.rpmmacros
#
# copy tar.gz to RPM SOURCES
#
cp ${NAME}-${VERSION}.tar.gz ${RPM_TOP_DIR}/SOURCES

#
# Fix up the spec file
# 
cat ${HOME}/srctmp/${NAME}.spec | sed "s/\%VERSION\%/$VERSION/" | sed "s/\%NAME\%/$NAME/" | sed "s/\%PACKAGER\%/$PACKAGER/" | sed "s/\%PROJECT_DESC\%/$PROJECT_DESC/" | sed "s/\%RELEASE\%/$RELEASE/" > ${NAME}-${VERSION}.spec

#
# We have the spec file now, build the RPM.
#
rpm -ba ${NAME}-${VERSION}.spec

#
# If everything goes well - clean up.
#
if [ $? = "0" ]
then
	cp ${RPM_TOP_DIR}/SOURCES/${NAME}-${VERSION}.tar.gz $BUILDRPM_START_DIR
	rm -rf ${HOME}/srctmp
	rm  ${HOME}/.rpmrc
	if [ -e ${HOME}/rpmrc.ORG ]
	then
		mv ${HOME}/rpmrc.ORG ${HOME}/.rpmrc
	fi
	rm  ${HOME}/.rpmmacros
	if [ -e ${HOME}/rpmmacros.ORG ]
	then
		mv ${HOME}/rpmmacros.ORG ${HOME}/.rpmmacros
	fi
	mv  $RPM_TOP_DIR/SRPMS/* $BUILDRPM_START_DIR
	mv  $RPM_TOP_DIR/RPMS/$arch/* $BUILDRPM_START_DIR 
	rm -rf ${RPM_TOP_DIR}
	echo "The source rpm is created and under $BUILDRPM_START_DIR directory"
	echo "The binary rpm is created and under $BUILDRPM_START_DIR directory"
fi
