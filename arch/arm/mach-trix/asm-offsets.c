/*
 * Generate definitions needed by assembly language modules.
 * This code generates raw asm output which is post-processed to extract
 * and format the required data.
 *
 * Author: Tony He <tony_he@sigmadesigns.com>
 * Date:   05/16/2016
 */
#if defined(__PREBOOT__)
#include <config.h>
#include <umac.h>
#include <list.h>
#include <kbuild.h>
#elif defined(__UBOOT__)
#include <common.h>
#include <linux/kbuild.h>
#include <asm/arch/umac.h>
#include "s2ramctrl.h"
#elif defined(__LINUX__)
#include <linux/kernel.h>
#include <linux/kbuild.h>
#include "umac.h"
#include "s2ramctrl.h"
#endif

int main(void)
{
#if defined(__UBOOT__)
	DEFINE(S2RAM_ENTRY_OFS, offsetof(struct s2ram_resume_frame, S2RAM_ENTRY));
	DEFINE(S2RAM_START0_OFS, offsetof(struct s2ram_resume_frame, S2RAM_START0));
	DEFINE(S2RAM_LEN0_OFS, offsetof(struct s2ram_resume_frame, S2RAM_LEN0));
	DEFINE(S2RAM_CRC0_OFS, offsetof(struct s2ram_resume_frame, S2RAM_CRC0));
	DEFINE(S2RAM_START1_OFS, offsetof(struct s2ram_resume_frame, S2RAM_START1));
	DEFINE(S2RAM_LEN1_OFS, offsetof(struct s2ram_resume_frame, S2RAM_LEN1));
	DEFINE(S2RAM_CRC1_OFS, offsetof(struct s2ram_resume_frame, S2RAM_CRC1));
	DEFINE(S2RAM_CRC_OFS, offsetof(struct s2ram_resume_frame, S2RAM_CRC));
	BLANK();
#endif
#if defined(__LINUX__)
	DEFINE(S2RAM_ENTRY_OFS, offsetof(struct s2ram_resume_frame, S2RAM_ENTRY));
	BLANK();
#endif
	DEFINE(MCTL_OFS_IF_ENABLE, offsetof(struct umac_mac, mctl.if_enable));
	BLANK();
	DEFINE(PCTL_OFS_SCTL, offsetof(struct umac_phy, pctl.sctl));
	DEFINE(PCTL_OFS_STAT, offsetof(struct umac_phy, pctl.stat));
	DEFINE(PCTL_OFS_DQSECFG, offsetof(struct umac_phy, pctl.dqsecfg));
	DEFINE(PCTL_OFS_TREFI, offsetof(struct umac_phy, pctl.trefi));
	DEFINE(PCTL_OFS_PHYPVTUPDI, offsetof(struct umac_phy, pctl.phypvtupdi));
	DEFINE(PCTL_OFS_PVTUPDI, offsetof(struct umac_phy, pctl.pvtupdi));
	BLANK();
	DEFINE(PUB_OFS_PHY_PIR, offsetof(struct umac_phy, pub.phy_pir));
	DEFINE(PUB_OFS_PHY_ACIOCR, offsetof(struct umac_phy, pub.phy_aciocr));
	DEFINE(PUB_OFS_PHY_DXCCR, offsetof(struct umac_phy, pub.phy_dxccr));
	DEFINE(PUB_OFS_TR_ADDR0, offsetof(struct umac_phy, pub.pub_tr_addr0));
	DEFINE(PUB_OFS_TR_ADDR1, offsetof(struct umac_phy, pub.pub_tr_addr1));
	DEFINE(PUB_OFS_TR_ADDR2, offsetof(struct umac_phy, pub.pub_tr_addr2));
	DEFINE(PUB_OFS_TR_ADDR3, offsetof(struct umac_phy, pub.pub_tr_addr3));
	DEFINE(PUB_OFS_TR_RD_LCDL, offsetof(struct umac_phy, pub.pub_tr_rd_lcdl));
	DEFINE(PUB_OFS_TR_WR_LCDL, offsetof(struct umac_phy, pub.pub_tr_wr_lcdl));
	DEFINE(PUB_OFS_PHY_DX0LCDLR1, offsetof(struct umac_phy, pub.phy_dx0lcdlr1));
	DEFINE(PUB_OFS_PHY_DX1LCDLR1, offsetof(struct umac_phy, pub.phy_dx1lcdlr1));
	DEFINE(PUB_OFS_PHY_DX2LCDLR1, offsetof(struct umac_phy, pub.phy_dx2lcdlr1));
	DEFINE(PUB_OFS_PHY_DX3LCDLR1, offsetof(struct umac_phy, pub.phy_dx3lcdlr1));
#if !defined(CONFIG_SIGMA_SOC_UXLB)
	DEFINE(PUB_OFS_TR_RW_OFS, offsetof(struct umac_phy, pub.pub_tr_rw_ofs));
	DEFINE(PUB_OFS_TR_RW_OFS_SIGN, offsetof(struct umac_phy, pub.pub_tr_rw_ofs_sign));
#else
	BLANK();
	DEFINE(ABT_OFS_TR_RW_OFS, offsetof(struct umac_arbt, pub_tr_rw_ofs));
	DEFINE(ABT_OFS_TR_RW_OFS_SIGN, offsetof(struct umac_arbt, pub_tr_rw_ofs_sign));
#endif
	BLANK();
	DEFINE(SIZEOF_UMAC_DEV, sizeof(struct umac_device));
	DEFINE(UDEV_OFS_MAC, offsetof(struct umac_device, mac));
	DEFINE(UDEV_OFS_PHY, offsetof(struct umac_device, phy));
	DEFINE(UDEV_OFS_ABT, offsetof(struct umac_device, abt));
	DEFINE(UDEV_OFS_WIDTH, offsetof(struct umac_device, width));
	DEFINE(UDEV_OFS_QUIRKS, offsetof(struct umac_device, quirks));
	DEFINE(UDEV_OFS_ADDR, offsetof(struct umac_device, addr));
	BLANK();
	return 0;
}
