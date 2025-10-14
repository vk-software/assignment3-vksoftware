#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    echo "Running the mrproper target"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    echo "Running the defconfig target"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    echo "Running the all target"
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
    echo "Running the modules target"
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
    echo "Running the dtbs target"
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

mkdir -p ${OUTDIR}/rootfs
cd "${OUTDIR}/rootfs"

mkdir -p bin dev etc home lib lib64 proc sbin  sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
	make defconfig
else
    cd busybox
fi

# Check if busybox is already built
if [ ! -f ${OUTDIR}/rootfs/usr/sbin/telnetd ]; then
    # TODO: Make and install busybox
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
    make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
else
    echo "Busybox is already built and installed!"
fi

cd ${OUTDIR}/rootfs
echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
TOOLCHAIN_SYSROOT_DIR=$(${CROSS_COMPILE}gcc -print-sysroot)
echo "Copying library dependencies from ${TOOLCHAIN_SYSROOT_DIR}"
cp ${TOOLCHAIN_SYSROOT_DIR}/lib/ld-linux-aarch64.so.1 lib/
cp ${TOOLCHAIN_SYSROOT_DIR}/lib64/libm.so.6 lib64/ 
cp ${TOOLCHAIN_SYSROOT_DIR}/lib64/libresolv.so.2 lib64/
cp ${TOOLCHAIN_SYSROOT_DIR}/lib64/libc.so.6 lib64/

# TODO: Make device nodes

if [ ! -f ${OUTDIR}/rootfs/dev/null ]; then
    sudo mknod -m 666 dev/null c 1 3
else
    echo "dev/null already exists"
fi

if [ ! -f ${OUTDIR}/rootfs/dev/console ]; then
    sudo mknod -m 666 dev/console c 5 1
else
    echo "dev/console already exists"
fi

# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean # clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs

cp writer ${OUTDIR}/rootfs/home
cp finder.sh ${OUTDIR}/rootfs/home/
cp finder-test.sh ${OUTDIR}/rootfs/home/

mkdir -p ${OUTDIR}/rootfs/home/conf
cp ../conf/username.txt ${OUTDIR}/rootfs/home/conf
cp ../conf/assignment.txt ${OUTDIR}/rootfs/home/conf

cp autorun-qemu.sh ${OUTDIR}/rootfs/home/

# TODO: Chown the root directory

sudo chown -R root:root ${OUTDIR}/rootfs

# TODO: Create initramfs.cpio.gz

cd "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip -f initramfs.cpio
