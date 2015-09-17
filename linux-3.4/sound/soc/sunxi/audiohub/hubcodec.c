/*
 * sound\soc\sunxi\audiohub\hubcodec.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/io.h>
#include <mach/sys_config.h>
#include "./../daudio0/sunxi-daudio0.h"
struct snd_hubcodec_priv {
	int sysclk;
	int dai_fmt;

	struct snd_pcm_substream *master_substream;
	struct snd_pcm_substream *slave_substream;
};

static int hub_used = 0;
#define snd_hubcodec_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define snd_hubcodec_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static int snd_hubcodec_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int snd_hubcodec_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0;
}

static void snd_hubcodec_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{

}

static int snd_hubcodec_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	return 0;
}

static int snd_hubcodec_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int snd_hubcodec_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{
	return 0;
}

static int snd_hubcodec_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

static struct snd_soc_dai_ops snd_hubcodec_dai_ops = {
	.startup = snd_hubcodec_startup,
	.shutdown = snd_hubcodec_shutdown,
	.hw_params = snd_hubcodec_hw_params,
	.digital_mute = snd_hubcodec_mute,
	.set_sysclk = snd_hubcodec_set_dai_sysclk,
	.set_clkdiv = snd_hubcodec_set_dai_clkdiv,
	.set_fmt = snd_hubcodec_set_dai_fmt,
};

static struct snd_soc_dai_driver snd_hubcodec_dai = {
	.name = "snd-hubcodec-dai",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = snd_hubcodec_RATES,
		.formats = snd_hubcodec_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = snd_hubcodec_RATES,
		.formats = snd_hubcodec_FORMATS,
	},
	/* pcm operations */
	.ops = &snd_hubcodec_dai_ops,	
};
//EXPORT_SYMBOL(snd_hubcodec_dai);
	
static int snd_hubcodec_soc_probe(struct snd_soc_codec *codec)
{
	struct snd_hubcodec_priv *snd_hubcodec;

	snd_hubcodec = kzalloc(sizeof(struct snd_hubcodec_priv), GFP_KERNEL);
	if(snd_hubcodec == NULL){		
		return -ENOMEM;
	}		
	snd_soc_codec_set_drvdata(codec, snd_hubcodec);

	return 0;
}

/* power down chip */
static int snd_hubcodec_soc_remove(struct snd_soc_codec *codec)
{
	struct snd_hubcodec_priv *snd_hubcodec = snd_soc_codec_get_drvdata(codec);

	kfree(snd_hubcodec);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_snd_hubcodec = {
	.probe 	=	snd_hubcodec_soc_probe,
	.remove =   snd_hubcodec_soc_remove,
};

static int __init snd_hubcodec_codec_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev, &soc_codec_dev_snd_hubcodec, &snd_hubcodec_dai, 1);	
}

static int __exit snd_hubcodec_codec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

/*data relating*/
static struct platform_device snd_hubcodec_codec_device = {
	.name = "sunxi-hubcodec-codec",
};

/*method relating*/
static struct platform_driver snd_hubcodec_codec_driver = {
	.driver = {
		.name = "sunxi-hubcodec-codec",
		.owner = THIS_MODULE,
	},
	.probe = snd_hubcodec_codec_probe,
	.remove = __exit_p(snd_hubcodec_codec_remove),
};

static int __init snd_hubcodec_codec_init(void)
{	
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audiohub", "hub_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[audiohub]:%s,line:%d type err!\n", __func__, __LINE__);
    }

	hub_used = val.val;

	if (hub_used) {
		if((err = platform_device_register(&snd_hubcodec_codec_device)) < 0)
			return err;
	
		if ((err = platform_driver_register(&snd_hubcodec_codec_driver)) < 0)
			return err;
	} else {
       pr_err("[audiohub]snd_hubcodec cannot find any using configuration for controllers, return directly!\n");
       return 0;
    }
	
	return 0;
}
module_init(snd_hubcodec_codec_init);

static void __exit snd_hubcodec_codec_exit(void)
{
	if (hub_used) {
		hub_used = 0;
		platform_driver_unregister(&snd_hubcodec_codec_driver);
	}
}
module_exit(snd_hubcodec_codec_exit);

MODULE_DESCRIPTION("AUDIO-HUB ALSA soc codec driver");
MODULE_AUTHOR("huangxin");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-daudio-codec");
