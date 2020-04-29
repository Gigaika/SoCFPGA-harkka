SUMMARY = "Userspace driver for the image classifier hardware accelerator"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://classifier.c \
           file://classifier.h \
           file://dma-proxy.h \
           file://devices.h \
           file://devices.c \
           file://interface.h \
           file://interface.c \
	   file://Makefile \
		  "

S = "${WORKDIR}"

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 classifier ${D}${bindir}
}
