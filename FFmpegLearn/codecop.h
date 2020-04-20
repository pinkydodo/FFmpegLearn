//
//  codecop.h
//  extraAudio
//
//  Created by pinky on 2020/4/12.
//  Copyright © 2020 pinky. All rights reserved.
//

#ifndef codecop_h
#define codecop_h

#include <stdio.h>
#include <libavcodec/avcodec.h>

/*
 主要结构体：
 AVCodec:编码器结构体
 AVCodecContext:编码器上下文
 AVFrame：解码后的帧
 
 主要函数:
 av_frame_alloc()/av_frame_free()
 avcodec_alloc_context3()/avcodec_free_context()
 
 解码步骤：
 1.找到解码器:avcodec_find_decoder(by_id/by_name)
 2.打开解码器：avcodec_open2
 3.解码：avcodec_decode_video2
 
 编码步骤：
 1.找到编码器：avcodec_find_encoder_by_name
 2.设置编码参数，并打开编码器:avcodec_open2
 3.编码：avcodec_encode_video2
 
 
 */

/*
 AVCodec包含的主要内容：
 1.名字，ID等表示AVCodec的内容
 2.audio,video支持的参数
 */
void list_codec_sample_fmt_by_id( enum AVCodecID codec_id );
void show_audio_codec_info( AVCodec* codec );
void show_audio_codec_info_by_id( enum AVCodecID codec_id );

/*
 encode audio 步骤
 1.打开编码器
    avcodec_find_encoder
    avcodec_alloc_context3
    设置参数，并检查参数是否支持（bit_rate, sample_rate, channel_layout, channels）
    avcodec_open2
 3.循环读数据，并编码
    设置AVFrame参数( nb_samples = c->frame_size, format, channle_layout)
    创建AVFrame的Buff,av_frame_get_buffer
    填充AVFrame数据(av_frame_make_writable):数据存放格式：
    avcodec_semd_frame
    avcodec_receive_packet      //最后flush 数据
 */
int encode_audio(  const char* outfilePath);

/*
 decode audio 步骤
 1.打开input,找到stream_index
 2.打开解码器
    avcodec_find_decoder
    avcodec_alloc_context3
    avcodec_parameters_to_context   //从input拷贝参数,解码的输入是包，里面可能没有参数信息
    avcodec_open2
 3.循环读数据，并解码数据
    av_read_frame
    avcodec_send_packet
    avcodec_receive_frame       //需要用空数据flush数据
 */
int decode_audio( const  char* infile, const char* outfile);

int encode_video( const  char* infile, const char* outfile);
int decode_video( const  char* infile, const char* outfile);

#endif /* codecop_h */
