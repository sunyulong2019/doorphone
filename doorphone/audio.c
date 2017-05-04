//////////////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  File name: GPLE/audio.c
//
//  Author: Vincent Lin (林世霖)  微信公众号：秘籍酷
//
//  Date: 2017-04
//  
//  Description: 提供音频子模块功能
//
//  GitHub: github.com/vincent040   Bug Report: 2437231462@qq.com
//
//////////////////////////////////////////////////////////////////

#include "audio.h"

pcm_container *g_sound_recv = NULL;
pcm_container *g_sound_send = NULL;

int err = 0;
pthread_mutex_t pcm_lock;


// 从麦克风读取frames帧数据，返回真正读取到的帧数
snd_pcm_uframes_t read_pcm_data(pcm_container *sound, snd_pcm_uframes_t frames)
{
	snd_pcm_uframes_t exact_frames = 0;
	snd_pcm_uframes_t n = 0;

	uint8_t *p = sound->period_buf;
	int err_count = 0;
	while(frames > 0)	
	{
		if((n=snd_pcm_readi(sound->handle, p, frames)) < 0)
		{
			if(err_count++ > 100);
			{
				fprintf(stderr, "snd_pcm_readi() failed too many times: %s\n", strerror(errno));
				break;
			}
			continue;
		}

		frames -= n;
		exact_frames += n;
		p += (n * sound->bytes_per_frame);
	}

	return exact_frames;
}

ssize_t write_pcm_to_device(pcm_container *sound, size_t frames)  
{  
	uint8_t *data = sound->period_buf;  
  
	// 如果要写入的帧数不够一个周期，则用静音填满一个周期
	if(frames < sound->frames_per_period)
	{  
		snd_pcm_format_set_silence(sound->format,   
					data + frames * sound->bytes_per_frame,   
					(sound->frames_per_period - frames) * sound->channels);  

		frames = sound->frames_per_period;  
	}  

	ssize_t total_frames = 0;  
	while(frames > 0)
	{
		snd_pcm_uframes_t n = snd_pcm_writei(sound->handle, data, frames);  

		if(n == -EAGAIN || (n >= 0 && (size_t)n < frames))
		{  
			snd_pcm_wait(sound->handle, 1000); 
		}
		else if(n == -EPIPE)
		{  
			snd_pcm_prepare(sound->handle);  
		}
		else if(n < 0)
		{  
			fprintf(stderr, "Error snd_pcm_writei: [%s]\n", snd_strerror(n));  
			exit(-1);
		}  
		if(n > 0)
		{  
			total_frames += n;  
			frames -= n;  
			data += n * sound->bytes_per_frame;  
		}  
	}  
	return total_frames;  
}  


void set_pcm_param(pcm_container *pcm)  
{  
	snd_pcm_hw_params_t *hwparams;  
  
	// A) 分配参数空间
	//	以PCM设备能支持的所有配置范围初始化该参数空间
	snd_pcm_hw_params_alloca(&hwparams);  

	int err;
	if((err=snd_pcm_hw_params_any(pcm->handle, hwparams)) < 0)
	{
		printf("[%d] error: %s\n", __LINE__, snd_strerror(err));
		exit(0);
	}
  
	// B) 设置访问方式为“帧连续交错方式”
	snd_pcm_hw_params_set_access(pcm->handle, hwparams,
									SND_PCM_ACCESS_RW_INTERLEAVED);
  
	// C) 设置量化参数为（16位）
	snd_pcm_hw_params_set_format(pcm->handle, hwparams, pcm->format);
  
	// D) 设置声道数为（2声道）
	snd_pcm_hw_params_set_channels(pcm->handle, hwparams, LE_SHORT(2));
  
	// E) 设置采样频率为（44100赫兹）
	//	如果声卡不支持该采样频率，则选择一个最接近的频率
	snd_pcm_hw_params_set_rate_near(pcm->handle, hwparams, &pcm->sample_rate, 0);
  
	// F) 设置buffer大小为声卡支持的最大值
	//	并将处理周期设置为buffer的1/4的大小
	snd_pcm_hw_params_get_buffer_size_max(hwparams,
									&pcm->frames_per_buffer);
	snd_pcm_hw_params_set_buffer_size_near(pcm->handle,
									hwparams, &pcm->frames_per_buffer);

	pcm->frames_per_period = pcm->frames_per_buffer / 4;
	snd_pcm_hw_params_set_period_size(pcm->handle,
									hwparams, pcm->frames_per_period, 0);
	snd_pcm_hw_params_get_period_size(hwparams,
									&pcm->frames_per_period, 0);

	// G) 将所设置的参数安装到PCM设备中
	snd_pcm_hw_params(pcm->handle, hwparams);
  
	// H) 由所设置的buffer时间和周期，分配相应的大小缓冲区
	pcm->bits_per_sample = snd_pcm_format_physical_width(pcm->format);
	pcm->bytes_per_frame = pcm->bits_per_sample/8 * LE_SHORT(pcm->channels);
	pcm->period_buf = (uint8_t *)malloc(pcm->frames_per_period *
							pcm->bytes_per_frame);
} 

void prepare_pcm_param(pcm_container *pcm)
{
	pcm->format = SND_PCM_FORMAT_S16_LE; // 量化位数
	pcm->channels = LE_SHORT(2);		 // 声道数目
	pcm->sample_rate = LE_INT(44100);	 // 采样频率
}
