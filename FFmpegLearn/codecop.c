//
//  codecop.c
//  extraAudio
//
//  Created by pinky on 2020/4/12.
//  Copyright © 2020 pinky. All rights reserved.
//

#include "codecop.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

int check_sample_fmt( const AVCodec* codec, enum AVSampleFormat sample_fmt )
{
    const enum AVSampleFormat* p = codec->sample_fmts;
    while( *p != AV_SAMPLE_FMT_NONE )
    {
        if( *p == sample_fmt )
            return 1;
        p++;
    }
    
    return 0;
}

void list_codec_sample_fmt( const AVCodec* codec )
{
    const enum AVSampleFormat* p = codec->sample_fmts;
    while( *p != AV_SAMPLE_FMT_NONE )
    {
        fprintf( stdout, "  %s\n", av_get_sample_fmt_name( *p ));
        p++;
    }
}

void show_audio_codec_info_by_id( enum AVCodecID codec_id )
{
    AVCodec* codec = avcodec_find_encoder( codec_id );
    if( codec )
    {
        fprintf( stdout, "Encode:\n");
        show_audio_codec_info( codec );
    }
    else{
        fprintf(stderr, "Encoder not support!\n");
    }
    
    codec = avcodec_find_decoder( codec_id );
    if( codec )
    {
        fprintf( stdout, "Decode:\n");
        show_audio_codec_info( codec );
    }
    else{
        fprintf(stderr, "Decoder not support!\n");
    }
}

void list_codec_sample_fmt_by_id( enum AVCodecID codec_id )
{
    AVCodec* codec = avcodec_find_encoder( codec_id );
    if( codec )
    {
        fprintf(stdout, "Encoder support fmt:\n");
        list_codec_sample_fmt( codec );
    }
    else{
        fprintf(stderr, "Encoder not support!\n");
    }
    
    codec = avcodec_find_decoder( codec_id );
    if( codec )
    {
        fprintf(stdout, "Decoder support fmt:\n");
        list_codec_sample_fmt( codec );
    }
    else{
        fprintf(stderr, "Decoder not support!\n");
    }
    
}


void show_audio_codec_info( AVCodec* codec )
{
    fprintf( stdout, "MediaType:%d\n", codec->type );
    fprintf( stdout, "Supported Sample rates :\n");
    int* p = codec->supported_samplerates;
    while( p && *p != 0)
    {
        fprintf( stdout,"   %d\n", *p );
        p++;
    }
    
    fprintf( stdout, "Supported Channel layouts:\n");
    uint64_t* pcl = codec->channel_layouts;
    while( pcl && *pcl != 0 )
    {
        fprintf( stdout, "  %llu\n", *pcl);
        pcl++;
    }
    
    fprintf( stdout, "Supported Sample Fmt:\n");
    const enum AVSampleFormat* psf = codec->sample_fmts;
    while( *psf != AV_SAMPLE_FMT_NONE )
    {
        fprintf( stdout, "  %s\n", av_get_sample_fmt_name( *psf ));
        psf++;
    }
}


int select_sample_rate( const AVCodec* codec )
{
    const int *p;
    int best_samplerate = 0;
    
    if( !codec->supported_samplerates )
        return 44100;
    
    //找最接近44100的。
    p = codec->supported_samplerates;
    while( *p )
    {
        if( !best_samplerate || abs( 44100 - *p ) < abs( 44100 -best_samplerate ))
            best_samplerate = *p ;
        p++;
    }
    
    return best_samplerate;
}

int select_channel_layout( const AVCodec* codec )
{
    const uint64_t* p;
    uint64_t best_ch_layout = 0;
    int best_nb_channels = 0;
    
    if( !codec->channel_layouts )
        return AV_CH_LAYOUT_STEREO;
    
    //找声道数多的
    p = codec->channel_layouts;
    while (*p) {
        int nb_channels = av_get_channel_layout_nb_channels( *p );
        
        if( nb_channels > best_nb_channels )
        {
            best_ch_layout = *p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return best_ch_layout;
}


void encode( AVCodecContext* ctx, AVFrame* frame, AVPacket* pkt, FILE* output )
{
    int ret = 0;
    ret = avcodec_send_frame( ctx, frame );
    if( ret < 0 )
    {
        printf("Error sending frame:%s\n", av_err2str( ret ));
        return ;
    }
    
    while( ret >= 0 )
    {
        ret = avcodec_receive_packet( ctx, pkt );
        if( ret == AVERROR( EAGAIN) || ret == AVERROR_EOF )
            return ;
        else if ( ret < 0 )
        {
            printf("Error encoding audio ");
            return;
        }
        
        fwrite( pkt->data, 1, pkt->size, output);
        av_packet_unref( pkt );
    }
}
int encode_audio(const char* outfilePath )
{
//    avcodec_register_all();
    AVCodec* codec;
    AVCodecContext* c = NULL;
    int ret = 0;
    AVPacket *pkt=NULL;
    AVFrame *frame = NULL;
    
    //准备
    pkt = av_packet_alloc();
    frame = av_frame_alloc();
    
    if(!pkt || !frame )
    {
        printf("Error alloc pkt and frame \n");
        return -1;
    }
    
    FILE* f = fopen( outfilePath , "wb");
    //find codec
    codec = avcodec_find_encoder( AV_CODEC_ID_MP2 );
    if(!codec )
    {
        fprintf( stderr, "Codec not found\n");
        return -1;
    }
    
    c = avcodec_alloc_context3( codec );
    if(!c)
    {
        fprintf( stderr, "Counld not allocate audio codec context\n");
        return -1;
    }
    
    //set parameters
    c->bit_rate = 64000;
    
    c->sample_fmt = AV_SAMPLE_FMT_S16 ;
    if( !check_sample_fmt( codec, c->sample_fmt)){
        fprintf( stderr, "Encoder does not support sample format %s\n", av_get_sample_fmt_name(c->sample_fmt));
        return -1;
    }
    c->sample_rate = select_sample_rate( codec);
    c->channel_layout = select_channel_layout( codec );
    c->channels = av_get_channel_layout_nb_channels( c->channel_layout);
    
    ret = avcodec_open2( c, codec, NULL );
    if( ret < 0 )
    {
        printf("Error open avcodec\n");
        return -1;
    }
    
    //构建frame,设置frame参数，创建framebuff
    frame->nb_samples = c->frame_size;
    frame->format = c->sample_fmt;
    frame->channel_layout = c->channel_layout;
    
    printf("Encode Audio: sample_rate:%d, c:%d\n", c->sample_rate,  c->channels );
    
    //alloc the data buffer,设定好参数，就知道buffer需要的大小了，所以可以创建了。
    ret = av_frame_get_buffer( frame ,  0 );
    if( ret < 0 )
    {
        printf("Could not alloc audio data buffer");
        return -1;
    }
    
    //encode a single tone sound
    float t = 0;
    float tincr = 2*M_PI * 440.0 / c->sample_rate;
    uint16_t *samples;
    for( int i = 0 ; i < 200; i++ )
    {
        //make sure the frame is writable
        //makes a copy if the encoder kept a reference internally
        ret = av_frame_make_writable( frame );
        if( ret < 0)
            return -1;
        
        samples = (uint16_t*)frame->data[0];
        
        for( int j  = 0 ; j < c->frame_size; j++ )
        {
            samples[  c->channels * j  ] = (int)(sin(t) * 10000 );
            
            for( int k = 1 ; k < c->channels ; k++ )
            {
                samples[c->channels*j + k ] = samples[c->channels*j];
            }
            
            t+= tincr;
        }
        
        encode( c, frame, pkt, f );
    }
    
    encode( c, NULL, pkt, f );
    fclose( f );
    
    av_frame_free( &frame );
    av_packet_free( &pkt );
    avcodec_free_context( &c );
    
    return 0;
}

void decode( AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* frame, FILE* outfile )
{
    int ret  = 0;
    int data_size = 0;
    
    ret = avcodec_send_packet( dec_ctx,  pkt );
    if( ret < 0 )
    {
        printf("Error send packet :%s\n", av_err2str( ret ));
        return ;
    }
    
    while( ret >= 0 )
    {
        ret = avcodec_receive_frame( dec_ctx,  frame );
        if( ret == AVERROR( EAGAIN ) || ret == AVERROR_EOF )
            return ;
        else if( ret < 0 )
        {
            printf(" Error during decoding\n");
            return ;
        }
        
        data_size = av_get_bytes_per_sample( dec_ctx->sample_fmt );
        if( data_size < 0 )
        {
            printf("Failed to calculate data size\n");
            return ;
        }
        
        for( int i = 0 ; i < frame->nb_samples; i++ )
        {
            for( int ch = 0 ; ch < dec_ctx->channels; ch++ )
            {
                fwrite( frame->data[ch] + data_size * i , 1, data_size, outfile );
            }
        }
    }
}

/*
 
 */

int decode_audio( const  char* infile, const char* outfile)
{
    //准备工作
    AVFormatContext* ifmt = NULL;
    AVCodec* codec;
    AVCodecContext* c = NULL;
    AVPacket pkt;
    AVFrame* decoded_frame = NULL;
    int ret = 0;
    int audio_stream_index = -1;
    
    av_init_packet( &pkt );
    decoded_frame = av_frame_alloc();
    if(!decoded_frame )
    {
        printf("Error alloc frame \n");
        return -1;
    }
    //打开文件，找到音频流
    ret = avformat_open_input(&ifmt, infile, NULL, NULL );
    if( ret != 0 )
    {
        printf("open input fail\n");
        return -1;
    }
    
    audio_stream_index = av_find_best_stream( ifmt, AVMEDIA_TYPE_AUDIO, -1, 0, NULL, 0);
    AVStream* stream = ifmt->streams[audio_stream_index];
    
    //准备decoder
    codec = avcodec_find_decoder( AV_CODEC_ID_AAC );
    if(!codec)
    {
        printf("find decoder err!");
        return -1;
    }
    
    FILE* out = fopen( outfile, "wb" );
    
    c = avcodec_alloc_context3( codec );
    if(!c)
    {
        printf("Could not alloc audio codec context ");
        return -1;
    }
    
    //拷贝参数
    ret = avcodec_parameters_to_context( c,  stream->codecpar );
    if( ret < 0 )
    {
        return -1;
    }
    
    ret = avcodec_open2( c ,  codec, NULL );
    if( ret < 0 )
    {
        printf("Could not open codec\n");
        return -1;
    }
    
    do{
        ret = av_read_frame( ifmt, &pkt );
        if( ret < 0 )
        {
            break;
        }
        
        if( pkt.stream_index != audio_stream_index )
        {
            av_packet_unref( &pkt );
            continue;
        }
        
        decode( c, &pkt, decoded_frame, out );
        
        av_packet_unref( &pkt );
        
    }while(1);
    
    //flush the decoder
    pkt.data = NULL;
    pkt.size = 0;
    decode( c, &pkt, decoded_frame, out );
    
    fclose( out);
    
    avcodec_free_context(&c);
    av_frame_free( &decoded_frame);
    av_packet_free(&pkt);

    return 0;
}

int encode_video( const  char* infile, const char* outfile)
{
    return 0;
}
int decode_video( const  char* infile, const char* outfile)
{
    return 0;
}
