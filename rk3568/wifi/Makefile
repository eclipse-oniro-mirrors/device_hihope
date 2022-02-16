# SPDX-License-Identifier: GPL-2.0
#rkwifi packed Makefile
# (gwl)

#$(info -----------------AP6275S WIFI MAKEFILE -------------------)
$(info rkwifi hdf makefile: srctree=$(srctree)  src=$(src))

obj-$(CONFIG_AP6XXX_WIFI6_HDF) += bcmdhd_wifi6/
#obj-y += bcmdhd_wifi6/

.PHONY: clean

clean:
	find . -name '*.o*' -exec rm -f {} \; 
