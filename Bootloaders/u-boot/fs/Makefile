#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# SPDX-License-Identifier:	GPL-2.0+
#

ifdef CONFIG_SPL_BUILD
obj-$(CONFIG_SPL_FAT_SUPPORT) += fat/
else
obj-y				+= fs.o

obj-$(CONFIG_FS_EXT4) += ext4/
obj-y += fat/
endif
