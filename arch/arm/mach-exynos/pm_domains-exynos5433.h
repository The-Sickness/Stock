/*
 * Exynos Generic power domain support.
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Implementation of Exynos specific power domain control which is used in
 * conjunction with runtime-pm. Support for both device-tree and non-device-tree
 * based power domain support is included.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <plat/pm.h>
#include <mach/pm_domains.h>
#include <mach/devfreq.h>
#include "pm_domains-exynos5433-cal.h"

void __iomem *sysreg_disp;

bool is_mfc_clk_en = false;
bool is_hevc_clk_en = false;
bool is_gscl_clk_en = false;
bool is_mscl_clk_en = false;
bool is_g2d_clk_en = false;
bool is_g3d_clk_en = false;
bool is_isp_clk_en = false;
bool is_cam0_clk_en = false;
bool is_cam1_clk_en = false;
bool is_disp_clk_en = false;

struct exynos5433_pd_clk {
	const char *name;
	unsigned int num_gateclks;
	struct exynos5430_pd_state *gateclks;
};

static struct exynos5433_pd_clk pd_clk_list[] = {
	{
		.name = "pd-maudio",
	}, {
		.name = "pd-mfc",
		.num_gateclks = ARRAY_SIZE(gateclks_mfc),
		.gateclks = gateclks_mfc,
	}, {
		.name = "pd-hevc",
		.num_gateclks = ARRAY_SIZE(gateclks_hevc),
		.gateclks = gateclks_hevc,
	}, {
		.name = "pd-gscl",
		.num_gateclks = ARRAY_SIZE(gateclks_gscl),
		.gateclks = gateclks_gscl,
	}, {
		.name = "pd-mscl",
		.num_gateclks = ARRAY_SIZE(gateclks_mscl),
		.gateclks = gateclks_mscl,
	}, {
		.name = "pd-g2d",
		.num_gateclks = ARRAY_SIZE(gateclks_g2d),
		.gateclks = gateclks_g2d,
	}, {
		.name = "pd-isp",
		.num_gateclks = ARRAY_SIZE(gateclks_isp),
		.gateclks = gateclks_isp,
	}, {
		.name = "pd-cam0",
		.num_gateclks = ARRAY_SIZE(gateclks_cam0),
		.gateclks = gateclks_cam0,
	}, {
		.name = "pd-cam1",
		.num_gateclks = ARRAY_SIZE(gateclks_cam1),
		.gateclks = gateclks_cam1,
	}, {
		.name = "pd-g3d",
		.num_gateclks = ARRAY_SIZE(gateclks_g3d),
		.gateclks = gateclks_g3d,
	}, {
		.name = "pd-disp",
		.num_gateclks = ARRAY_SIZE(gateclks_disp),
		.gateclks = gateclks_disp,
	},
};

/* To prevent possible timing violation by high frequency clock,
 * follow the save/restore sequence below:
 * DIV -> (PLL) -> MUX -> GATE
 */
static struct sleep_save exynos_pd_maudio_clk_save[] = {
	SAVE_ITEM(EXYNOS5430_DIV_AUD0),
	SAVE_ITEM(EXYNOS5430_DIV_AUD1),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_AUD0),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_AUD1),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_AUD0),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_AUD1),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_AUD),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_AUD),
	SAVE_ITEM(EXYNOS5430_ENABLE_SCLK_AUD0),
	SAVE_ITEM(EXYNOS5430_ENABLE_SCLK_AUD1),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_AUD0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_AUD1),
};

static struct sleep_save exynos_pd_g3d_clk_save[] = {
	SAVE_ITEM(EXYNOS5430_DIV_G3D),
	SAVE_ITEM(EXYNOS5430_DIV_G3D_PLL_FREQ_DET),
	SAVE_ITEM(EXYNOS5430_G3D_PLL_LOCK),	/* G3D_PLL is in the BLK_G3D */
	SAVE_ITEM(EXYNOS5430_G3D_PLL_CON0),
	SAVE_ITEM(EXYNOS5430_G3D_PLL_CON1),
	SAVE_ITEM(EXYNOS5430_G3D_PLL_FREQ_DET),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_G3D),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_G3D),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_G3D),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_G3D),
	SAVE_ITEM(EXYNOS5430_ENABLE_SCLK_G3D),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_G3D0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_G3D1),
};

static struct sleep_save exynos_pd_mfc_clk_save[] = {
	SAVE_ITEM(EXYNOS5430_DIV_MFC0),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_MFC0),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_MFC0),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_MFC0),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_MFC0_SECURE_SMMU_MFC),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_MFC0),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_MFC0_SECURE_SMMU_MFC),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_MFC00),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_MFC01),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_MFC0_SECURE_SMMU_MFC),
};

static struct sleep_save exynos_pd_hevc_clk_save[] = {
	SAVE_ITEM(EXYNOS5430_DIV_HEVC),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_HEVC),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_HEVC),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_HEVC),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_HEVC_SECURE_SMMU_HEVC),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_HEVC),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_HEVC_SECURE_SMMU_HEVC),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_HEVC0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_HEVC1),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_HEVC_SECURE_SMMU_HEVC),
};

static struct sleep_save exynos_pd_gscl_clk_save[] = {
	SAVE_ITEM(EXYNOS5430_SRC_SEL_GSCL),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_GSCL),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_GSCL),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_GSCL_SECURE_SMMU_GSCL0),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_GSCL_SECURE_SMMU_GSCL1),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_GSCL_SECURE_SMMU_GSCL2),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_GSCL),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_GSCL_SECURE_SMMU_GSCL0),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_GSCL_SECURE_SMMU_GSCL1),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_GSCL_SECURE_SMMU_GSCL2),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_GSCL0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_GSCL1),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_GSCL_SECURE_SMMU_GSCL0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_GSCL_SECURE_SMMU_GSCL1),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_GSCL_SECURE_SMMU_GSCL2),
};

static struct sleep_save exynos_pd_disp_clk_save[] = {
	/* save/restore MIF muxes due to osc change at off_post */
	SAVE_ITEM(EXYNOS5430_SRC_SEL_MIF3),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_MIF4),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_MIF5),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_MIF6),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_MIF7),
	/* BLK_DISP CMU SFRs */
	SAVE_ITEM(EXYNOS5430_DIV_DISP),
	SAVE_ITEM(EXYNOS5430_DIV_DISP_PLL_FREQ_DET),
	SAVE_ITEM(EXYNOS5430_DISP_PLL_LOCK),	/* DISP_PLL is in the BLK_DISP */
	SAVE_ITEM(EXYNOS5430_DISP_PLL_CON0),
	SAVE_ITEM(EXYNOS5430_DISP_PLL_CON1),
	SAVE_ITEM(EXYNOS5430_DISP_PLL_FREQ_DET),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_DISP0),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_DISP1),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_DISP2),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_DISP3),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_DISP4),
	SAVE_ITEM(EXYNOS5430_SRC_IGNORE_DISP2),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_DISP0),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_DISP1),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_DISP2),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_DISP3),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_DISP4),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_DISP0),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_DISP1),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_DISP),
	SAVE_ITEM(EXYNOS5430_ENABLE_SCLK_DISP),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_DISP0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_DISP1),
};

static struct sleep_save exynos_pd_mscl_clk_save[] = {
	/* save/restore TOP muxes due to osc change at off_post */
	SAVE_ITEM(EXYNOS5430_SRC_SEL_TOP_MSCL),
	/* BLK_MSCL CMU SFRs */
	SAVE_ITEM(EXYNOS5430_DIV_MSCL),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_MSCL0),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_MSCL1),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_MSCL0),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_MSCL1),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_MSCL),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_MSCL_SECURE_SMMU_M2MSCALER0),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_MSCL_SECURE_SMMU_M2MSCALER1),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_MSCL_SECURE_SMMU_JPEG),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_MSCL),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_MSCL_SECURE_SMMU_M2MSCALER0),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_MSCL_SECURE_SMMU_M2MSCALER1),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_MSCL_SECURE_SMMU_JPEG),
	SAVE_ITEM(EXYNOS5430_ENABLE_SCLK_MSCL),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_MSCL0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_MSCL1),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_MSCL_SECURE_SMMU_M2MSCALER0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_MSCL_SECURE_SMMU_M2MSCALER1),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_MSCL_SECURE_SMMU_JPEG),
};

static struct sleep_save exynos_pd_g2d_clk_save[] = {
	SAVE_ITEM(EXYNOS5430_DIV_G2D),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_G2D),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_G2D),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_G2D),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_G2D_SECURE_SMMU_G2D),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_G2D),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_G2D_SECURE_SMMU_G2D),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_G2D0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_G2D1),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_G2D_SECURE_SMMU_G2D),
};

static struct sleep_save exynos_pd_isp_clk_save[] = {
	SAVE_ITEM(EXYNOS5430_DIV_ISP),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_ISP),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_ISP),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_ISP0),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_ISP1),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_ISP2),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_ISP),
	SAVE_ITEM(EXYNOS5430_ENABLE_SCLK_ISP),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_ISP0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_ISP1),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_ISP2),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_ISP3),
	/* CMU_ISP_LOCAL SFRs */
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_ISP_LOCAL),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_ISP_LOCAL),
	SAVE_ITEM(EXYNOS5430_ENABLE_SCLK_ISP_LOCAL),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_ISP_LOCAL0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_ISP_LOCAL1),
};

static struct sleep_save exynos_pd_cam0_clk_save[] = {
	SAVE_ITEM(EXYNOS5430_DIV_CAM00),
	SAVE_ITEM(EXYNOS5430_DIV_CAM01),
	SAVE_ITEM(EXYNOS5430_DIV_CAM02),
	SAVE_ITEM(EXYNOS5430_DIV_CAM03),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_CAM00),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_CAM01),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_CAM02),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_CAM03),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_CAM04),
	SAVE_ITEM(EXYNOS5430_SRC_IGNORE_CAM01),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_CAM00),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_CAM01),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_CAM02),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_CAM03),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_CAM04),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_CAM00),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_CAM01),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_CAM02),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_CAM0),
	SAVE_ITEM(EXYNOS5430_ENABLE_SCLK_CAM0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_CAM00),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_CAM01),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_CAM02),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_CAM03),
	/* CMU_CAM0_LOCAL SFRs */
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_CAM0_LOCAL),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_CAM0_LOCAL),
	SAVE_ITEM(EXYNOS5430_ENABLE_SCLK_CAM0_LOCAL),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_CAM0_LOCAL0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_CAM0_LOCAL1),
};

static struct sleep_save exynos_pd_cam1_clk_save[] = {
	/* save/restore TOP muxes due to osc change at off_post */
	SAVE_ITEM(EXYNOS5430_SRC_SEL_TOP_CAM1),
	/* BLK_CAM1 CMU SFRs */
	SAVE_ITEM(EXYNOS5430_DIV_CAM10),
	SAVE_ITEM(EXYNOS5430_DIV_CAM11),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_CAM10),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_CAM11),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_CAM12),
	SAVE_ITEM(EXYNOS5430_SRC_IGNORE_CAM11),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_CAM10),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_CAM11),
	SAVE_ITEM(EXYNOS5430_SRC_ENABLE_CAM12),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_CAM10),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_CAM11),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_CAM12),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_CAM1),
	SAVE_ITEM(EXYNOS5430_ENABLE_SCLK_CAM1),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_CAM10),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_CAM11),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_CAM12),
	/* CMU_CAM1_LOCAL SFRs */
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_CAM1_LOCAL),
	SAVE_ITEM(EXYNOS5430_ENABLE_PCLK_CAM1_LOCAL),
	SAVE_ITEM(EXYNOS5430_ENABLE_SCLK_CAM1_LOCAL),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_CAM1_LOCAL0),
	SAVE_ITEM(EXYNOS5430_ENABLE_IP_CAM1_LOCAL1),
};

static struct sleep_save exynos_pd_fsys_clk_save[] = {
	SAVE_ITEM(EXYNOS5430_SRC_SEL_TOP_FSYS0),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_TOP_FSYS1),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_FSYS0),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_FSYS1),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_FSYS2),
	SAVE_ITEM(EXYNOS5430_SRC_SEL_FSYS3),
	SAVE_ITEM(EXYNOS5430_ENABLE_ACLK_TOP),
	SAVE_ITEM(EXYNOS5430_ENABLE_SCLK_TOP_FSYS),
};

