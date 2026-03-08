subdir-ccflags-y += -Werror

mediatek-drm-y := mtk_drm_drv.o \
		  mtk_disp_tdshp.o \
		  mtk_disp_color.o \
		  mtk_disp_ccorr.o \
		  mtk_disp_c3d.o   \
		  mtk_disp_gamma.o \
		  mtk_disp_aal.o \
		  mtk_disp_dither.o \
		  mtk_disp_chist.o \
		  mtk_disp_ovl.o \
		  mtk_disp_rdma.o \
		  mtk_disp_rsz.o \
		  mtk_disp_wdma.o \
		  mtk_drm_crtc.o \
		  mtk_drm_ddp.o \
		  mtk_drm_ddp_addon.o \
		  mtk_drm_ddp_comp.o \
		  mtk_drm_fb.o \
		  mtk_drm_gem.o \
		  mtk_drm_plane.o \
		  mtk_dsi.o \
		  mtk_mipi_phy_60384.o \
		  mtk_mipi_tx.o \
		  mtk_writeback.o \
		  mtk_fence.o \
		  mtk_drm_session.o \
		  mtk_dump.o \
		  mtk_debug.o \
		  mtk_layering_rule.o \
		  mtk_layering_rule_base.o \
		  mtk_rect.o \
		  mtk_drm_mmp.o \
		  mtk_drm_trace.o \
		  mtk_panel_ext.o \
		  mtk_drm_helper.o \
		  mtk_drm_lowpower.o \
		  mtk_disp_postmask.o \
		  mtk_drm_assert.o \
		  mtk_drm_fbconsole.o \
		  mtk_disp_recovery.o \
		  mtk_disp_pmqos.o \
		  mtk_disp_dsc.o \
		  mtk_disp_cm.o \
		  mtk_disp_spr.o \
		  mtk_drm_arr.o \
		  mtk_disp_merge.o \
		  mtk_dmdp_aal.o \
		  mtk_fbconfig_kdebug.o \
		  mtk_ethdr.o \
		  mtk_disp_powerdev.o \
		  mtk_cec.o \
		  mtk_mdp_rdma.o        \
		  mtk_disp_pseudo_ovl.o \
		  vdo1/8188/mtk_vdo1_comp.o \
		  mtk_drm_dp_intf.o

#ccflags-y += -DDRM_CMDQ_DISABLE
#mediatek-drm-y += mtk_cmdq_dummy.o

ifeq ($(CONFIG_DRM_MEDIATEK_DEBUG_FS),y)
mediatek-drm-y += mtk_drm_debugfs.o
endif

ifeq ($(CONFIG_MTK_EDPTX_SUPPORT), y)
obj-y += mtk_drm_edp.o
endif

ifeq ($(CONFIG_MTK_DPTX_SUPPORT), y)
obj-y += mtk_drm_dp.o

ifeq (yy, $(CONFIG_MTK_TEE_GP_SUPPORT)$(CONFIG_OPTEE))
ifeq ($(CONFIG_DPTX_HDCP_ENABLE), y)
obj-y += ca/
obj-y += mtk_dp_hdcp1x.o \
	mtk_dp_hdcp2.o
endif
endif

endif

mediatek-drm-$(CONFIG_DRM_FBDEV_EMULATION) += mtk_drm_fbdev.o

obj-$(CONFIG_DRM_MEDIATEK) += mediatek-drm.o
obj-$(CONFIG_DRM_MEDIATEK) += mtk_sync.o

obj-$(CONFIG_DRM_MEDIATEK) += mtk_panel_ext.o
obj-$(CONFIG_DRM_MEDIATEK) += mtk_disp_notify.o


ccflags-y += -I$(srctree)/include/uapi/drm \
			 -I$(srctree)/drivers/gpu/drm/mediatek/mediatek_v2/vdo1/8188 \


ifeq ($(CONFIG_DRM_MEDIATEK_HDMI), y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/trustzone \

mediatek-drm-hdmi-objs := mtk_hdmi.o \
			  mtk_hdmi_ddc.o \
			  mtk_mt8195_hdmi_phy.o \
			  mtk_hdmi_ca.o \
			  mtk_hdmi_hdr.o \
			  mtk_hdmi_edid.o \
			  mtk_hdmi_debug.o \

ifeq ($(CONFIG_DRM_MEDIATEK_HDMI_HDCP),y)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hdmirx/
mediatek-drm-hdmi-objs += mtk_hdmi_hdcp.o
endif

endif

obj-$(CONFIG_DRM_MEDIATEK_HDMI) += mediatek-drm-hdmi.o \
				  mtk_dpi.o \
				  mtk_lvds.o \
				  mtk_lvds_phy.o

ccflags-y += -I$(srctree)/drivers/misc/mediatek/dramc/$(MTK_PLATFORM)	\
             -I$(srctree)/drivers/misc/mediatek/smi/			\
             -I$(srctree)/drivers/misc/mediatek/mmp/			\
             -I$(srctree)/drivers/misc/mediatek/mmdvfs/			\
             -I$(srctree)/drivers/misc/mediatek/pmic/			\
             -I$(srctree)/drivers/iommu/				\
             -I$(srctree)/drivers/devfreq/				\
             -I$(srctree)/drivers/gpu/drm/				\
             -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/mach/				\
             -I$(srctree)/drivers/misc/mediatek/include/mt-plat/ \
             -I$(srctree)/drivers/misc/mediatek/leds/ \
             -I$(srctree)/drivers/misc/mediatek/leds/$(MTK_PLATFORM) \
             -I$(srctree)/drivers/misc/mediatek/cmdq/mailbox/        \
             -I$(srctree)/drivers/misc/mediatek/iommu/		\
             -I$(srctree)/drivers/misc/mediatek/mtk-interconnect/        \
             -I$(srctree)/drivers/gpu/drm/mediatek \

ccflags-$(CONFIG_MTK_CMDQ_MBOX_EXT) += -I$(srctree)/drivers/misc/mediatek/cmdq/mailbox

#ccflags-y += -DDRM_MMPATH
ccflags-y += -DNOT_SUPPORT_CABC_HW
ccflags-y += -DDISP_MDP_COLOR_ON
ccflags-y += -DCCORR_SUPPORT

subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
