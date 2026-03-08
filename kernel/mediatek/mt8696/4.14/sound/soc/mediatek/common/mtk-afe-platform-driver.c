/*
 * mtk-afe-platform-driver.c  --  Mediatek afe platform driver
 *
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Garlic Tseng <garlic.tseng@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <sound/soc.h>

#include "mtk-afe-platform-driver.h"
#include "mtk-base-afe.h"

int mtk_afe_combine_sub_dai(struct mtk_base_afe *afe)
{
	struct mtk_base_afe_dai *dai;
	size_t num_dai_drivers = 0, dai_idx = 0;

	/* calcualte total dai driver size */
	list_for_each_entry(dai, &afe->sub_dais, list) {
		num_dai_drivers += dai->num_dai_drivers;
	}

	dev_info(afe->dev, "%s(), num of dai %zd\n", __func__, num_dai_drivers);

	/* combine sub_dais */
	afe->num_dai_drivers = num_dai_drivers;
	afe->dai_drivers = devm_kcalloc(afe->dev,
					num_dai_drivers,
					sizeof(struct snd_soc_dai_driver),
					GFP_KERNEL);
	if (!afe->dai_drivers)
		return -ENOMEM;

	list_for_each_entry(dai, &afe->sub_dais, list) {
		/* dai driver */
		memcpy(&afe->dai_drivers[dai_idx],
		       dai->dai_drivers,
		       dai->num_dai_drivers *
		       sizeof(struct snd_soc_dai_driver));
		dai_idx += dai->num_dai_drivers;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(mtk_afe_combine_sub_dai);

int mtk_afe_add_sub_dai_control(struct snd_soc_platform *platform)
{
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(platform);
	struct mtk_base_afe_dai *dai;

	list_for_each_entry(dai, &afe->sub_dais, list) {
		if (dai->controls)
			snd_soc_add_platform_controls(platform,
						      dai->controls,
						      dai->num_controls);

		if (dai->dapm_widgets)
			snd_soc_dapm_new_controls(&platform->component.dapm,
						  dai->dapm_widgets,
						  dai->num_dapm_widgets);
	}
	/* add routes after all widgets are added */
	list_for_each_entry(dai, &afe->sub_dais, list) {
		if (dai->dapm_routes)
			snd_soc_dapm_add_routes(&platform->component.dapm,
						dai->dapm_routes,
						dai->num_dapm_routes);
	}

	snd_soc_dapm_new_widgets(platform->component.dapm.card);

	return 0;

}
EXPORT_SYMBOL_GPL(mtk_afe_add_sub_dai_control);

static snd_pcm_uframes_t mtk_afe_pcm_pointer
			 (struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mtk_base_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];
	const struct mtk_base_memif_data *memif_data = memif->data;
	struct regmap *regmap = afe->regmap;
	struct device *dev = afe->dev;
	int reg_ofs_base = memif_data->reg_ofs_base;
	int reg_ofs_cur = memif_data->reg_ofs_cur;
	unsigned int hw_ptr = 0, hw_base = 0;
	int ret, pcm_ptr_bytes;
#if defined(CONFIG_SND_SOC_MT8696)
	unsigned long flags;
	unsigned long buf_cmp;
	unsigned long long temp_read;
	unsigned int hw_bef_read_index, buf_addition;

	spin_lock_irqsave(&memif->buf_info_lock, flags);
#endif

	ret = regmap_read(regmap, reg_ofs_cur, &hw_ptr);
	if (ret || hw_ptr == 0) {
		dev_err(dev, "%s hw_ptr err\n", __func__);
		pcm_ptr_bytes = 0;
		goto POINTER_RETURN_FRAMES;
	}

	ret = regmap_read(regmap, reg_ofs_base, &hw_base);
	if (ret || hw_base == 0) {
		dev_err(dev, "%s hw_ptr err\n", __func__);
		pcm_ptr_bytes = 0;
		goto POINTER_RETURN_FRAMES;
	}

	pcm_ptr_bytes = hw_ptr - hw_base;
#if defined(CONFIG_SND_SOC_MT8696)
	if (memif->avsync_mode) {
		temp_read = memif->buf_read;
		hw_bef_read_index = do_div(temp_read, memif->buffer_size);
		if (pcm_ptr_bytes >= hw_bef_read_index)
			buf_addition = pcm_ptr_bytes - hw_bef_read_index;
		else
			buf_addition = memif->buffer_size + pcm_ptr_bytes
					- hw_bef_read_index;

		buf_cmp = memif->buf_read + buf_addition;
		if (memif->buf_read >= memif->buf_write) {
			dev_info(afe->dev, "%s underrun happened!\n", __func__);
			dev_info(afe->dev, "%s buf_read 0x%llx, buf_write 0x%llx!\n",
				__func__, memif->buf_read, memif->buf_write);
			memif->buf_read = buf_addition;
			memif->buf_reset = true;
		} else {
			memif->buf_read += buf_addition;
		}
	}
	spin_unlock_irqrestore(&memif->buf_info_lock, flags);
#endif

POINTER_RETURN_FRAMES:
	return bytes_to_frames(substream->runtime, pcm_ptr_bytes);
}

#if defined(CONFIG_SND_SOC_MT8696)
int mtk_afe_pcm_copy_user(struct snd_pcm_substream *substream,
			int channel, unsigned long pos,
			void __user *buf, unsigned long bytes)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);
	struct mtk_base_afe_memif *memif = &afe->memif[rtd->cpu_dai->id];
	char *hwbuf = runtime->dma_area + pos +
		channel * (runtime->dma_bytes / runtime->channels);
	unsigned long flags;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (copy_from_user(hwbuf, buf, bytes))
			return -EFAULT;
	} else {
		if (copy_to_user(buf, hwbuf, bytes))
			return -EFAULT;
	}
	if (memif->avsync_mode) {
		spin_lock_irqsave(&memif->buf_info_lock, flags);
		memif->buf_write += bytes;
		spin_unlock_irqrestore(&memif->buf_info_lock, flags);
	}
	/*ATRACE_COUNTER("afe_pcm_copy", count);*/
	return 0;
}
#endif

const struct snd_pcm_ops mtk_afe_pcm_ops = {
	.ioctl = snd_pcm_lib_ioctl,
	.pointer = mtk_afe_pcm_pointer,
#if defined(CONFIG_SND_SOC_MT8696)
	.copy_user = mtk_afe_pcm_copy_user,
#endif
};
EXPORT_SYMBOL_GPL(mtk_afe_pcm_ops);

int mtk_afe_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	size_t size;
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	struct mtk_base_afe *afe = snd_soc_platform_get_drvdata(rtd->platform);

	size = afe->mtk_afe_hardware->buffer_bytes_max;
	return snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
						     card->dev, size, size);
}
EXPORT_SYMBOL_GPL(mtk_afe_pcm_new);

void mtk_afe_pcm_free(struct snd_pcm *pcm)
{
	snd_pcm_lib_preallocate_free_for_all(pcm);
}
EXPORT_SYMBOL_GPL(mtk_afe_pcm_free);

const struct snd_soc_platform_driver mtk_afe_pcm_platform = {
	.ops = &mtk_afe_pcm_ops,
	.pcm_new = mtk_afe_pcm_new,
	.pcm_free = mtk_afe_pcm_free,
};
EXPORT_SYMBOL_GPL(mtk_afe_pcm_platform);

MODULE_DESCRIPTION("Mediatek simple platform driver");
MODULE_AUTHOR("Garlic Tseng <garlic.tseng@mediatek.com>");
MODULE_LICENSE("GPL v2");

