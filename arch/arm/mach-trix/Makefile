#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

obj-y += lowlevel_init.o timer.o reset.o cache.o otp.o smc-stub.o

obj-$(CONFIG_TEST_S2RAM) += s2ramtest.o proc-v7.o sleep.o suspend.o
obj-$(CONFIG_S2RAM_CHECKSUM) += crc32.o s2ramcrc.o

obj-$(CONFIG_DTV_SMC) += smc-common.o

obj-$(CONFIG_BOOTPARAM) += boot-common.o
