//
//  formatop.c
//  extraAudio
//
//  Created by pinky on 2020/4/11.
//  Copyright © 2020 pinky. All rights reserved.
//

#include "formatop.h"
#include "libavformat/avformat.h"
#include "libavutil/timestamp.h"

int extra_audio( char* fileName )
{
    return 0;
}
int extra_video( char* fileName )
{
    return 0;
}


void log_packet( const AVFormatContext* fmt_ctx, const AVPacket* pkt, const char* tag )
{
    AVRational* time_base = &fmt_ctx->streams[ pkt->stream_index ]->time_base;
    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
    tag,
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

//核心点：时间戳转换
/*
 1.output的time_base可以设置吗，直接修改会在write_header的时候被改掉
 2.avio_open是必须，不然写文件会报错
 */

int remux( char* infile, char* outfile )
{
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    int ret = 0;
    
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;
    
    //open input
    if ((ret = avformat_open_input(&ifmt_ctx, infile, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", infile);
        goto end;
    }
    
    //貌似是有些格式没有header，需要find_stream_info后才能知道stream信息
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }
    
    av_dump_format( ifmt_ctx, 0, outfile, 0);
    
    //open output
    avformat_alloc_output_context2( &ofmt_ctx, NULL, NULL, outfile );
    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    
    //add stream
    int stream_count = ifmt_ctx->nb_streams;
    
    stream_mapping_size = stream_count;
    stream_mapping = av_mallocz_array( stream_mapping_size, sizeof(*stream_mapping) );
    if(!stream_mapping ){
        ret = AVERROR(ENOMEM);
        goto end;
    }
    
    int stream_index = 0;
    //这里的stream_index和在数组中的顺序，也许不能保证？看来应该是保序的
    for( int i = 0 ; i < stream_count; i++ )
    {
        AVStream* out_stream;
        AVStream* in_stream = ifmt_ctx->streams[i];
        
        AVCodecParameters* in_codecpar = in_stream->codecpar;
        
        out_stream = avformat_new_stream( ofmt_ctx,  NULL );
        if( !out_stream)
        {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        
        stream_mapping[i] = stream_index++;
        
        ret = avcodec_parameters_copy( out_stream->codecpar, in_codecpar );
        if( ret < 0 )
        {
            fprintf( stderr, "Failed to copy codec parameters\n");
            goto end;
        }
        
        //todo,这句不要会有什么影响？
        out_stream->codecpar->codec_tag = 0 ;
        
    }
    
    av_dump_format( ofmt_ctx, 0, outfile, 1 );
    
    //read and write packet
    //创建的时候可能没给文件名，所以这个需要创建文件
    if( !(ofmt_ctx->oformat->flags & AVFMT_NOFILE ) )
    {
        ret = avio_open( &ofmt_ctx->pb, outfile,  AVIO_FLAG_WRITE );
        if (ret < 0) {
              fprintf(stderr, "Could not open output file '%s'",  outfile);
              goto end;
        }
    }
    
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }
    
    //这个init不需要了吗？
    //av_init_packet(<#AVPacket *pkt#>)
    
    while(1)
    {
        AVStream* in_stream, *out_stream;
        
        ret = av_read_frame( ifmt_ctx, &pkt );
        if( ret < 0 )
            break;
        
        
        log_packet( ifmt_ctx,  &pkt, "in");
        in_stream = ifmt_ctx->streams[ pkt.stream_index ];
        pkt.stream_index = stream_mapping[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index ];
        
        //copy packet
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        
        log_packet( ofmt_ctx,  &pkt, "out");
        
        ret = av_interleaved_write_frame( ofmt_ctx,  &pkt );
        if( ret < 0 )
        {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }
        
        av_packet_unref( &pkt );
    }
    
    av_write_trailer( ofmt_ctx );
    
end:
    //close input and output
    avformat_close_input(& ifmt_ctx );
    
    if( ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE ) )
        avio_closep( &ofmt_ctx->pb );
    
    avformat_free_context( ofmt_ctx);
    
    av_freep( &stream_mapping );
    
    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }
    
    return 0;
}


/*
 1.与remux比，增加av_seek_frame，及判断到了结束时间的提前退出。
    a.是否指定具体的流，时间参数的单位不一样
    b.如果指定_Any，则如果没找到关键帧，会有一段时间看不到画面。
 2.由于从中间截取，所以pts,dts应该对应的减去开始时间，这个开始时间，对于pts和dts应该是一致的。
 */
int crop( char* infile, int startTime, int duration, char* outfile )
{
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    int ret = 0;
    
    int video_stream_index = -1;
    
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;
    
    //open input
    if ((ret = avformat_open_input(&ifmt_ctx, infile, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", infile);
        goto end;
    }
    
    //貌似是有些格式没有header，需要find_stream_info后才能知道stream信息
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }
    
    av_dump_format( ifmt_ctx, 0, outfile, 0);
    
    //open output
    avformat_alloc_output_context2( &ofmt_ctx, NULL, NULL, outfile );
    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    
    //add stream
    int stream_count = ifmt_ctx->nb_streams;
    
    stream_mapping_size = stream_count;
    stream_mapping = av_mallocz_array( stream_mapping_size, sizeof(*stream_mapping) );
    if(!stream_mapping ){
        ret = AVERROR(ENOMEM);
        goto end;
    }
    
    int stream_index = 0;
    //这里的stream_index和在数组中的顺序，也许不能保证？看来应该是保序的
    for( int i = 0 ; i < stream_count; i++ )
    {
        AVStream* out_stream;
        AVStream* in_stream = ifmt_ctx->streams[i];
        
        if( in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
        {
            video_stream_index = i;
        }
        AVCodecParameters* in_codecpar = in_stream->codecpar;
        
        out_stream = avformat_new_stream( ofmt_ctx,  NULL );
        if( !out_stream)
        {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        
        stream_mapping[i] = stream_index++;
        
        ret = avcodec_parameters_copy( out_stream->codecpar, in_codecpar );
        if( ret < 0 )
        {
            fprintf( stderr, "Failed to copy codec parameters\n");
            goto end;
        }
        
        //todo,这句不要会有什么影响？
        out_stream->codecpar->codec_tag = 0 ;
        
    }
    
    av_dump_format( ofmt_ctx, 0, outfile, 1 );
    
    //read and write packet
    //创建的时候可能没给文件名，所以这个需要创建文件
    if( !(ofmt_ctx->oformat->flags & AVFMT_NOFILE ) )
    {
        ret = avio_open( &ofmt_ctx->pb, outfile,  AVIO_FLAG_WRITE );
        if (ret < 0) {
              fprintf(stderr, "Could not open output file '%s'",  outfile);
              goto end;
        }
    }
        
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }
    
    //这个init不需要了吗？
    //av_init_packet(<#AVPacket *pkt#>)
    
    //seek,可以seek指定流，也可以seek不指定流
//    AV_TIME_BASE
//    if( video_stream_index != -1 )
//    {
//        AVStream* video_stream = ifmt_ctx->streams[video_stream_index];
//        int start_ts = startTime * video_stream->time_base.den / video_stream->time_base.num ;
//        ret = av_seek_frame( ifmt_ctx, video_stream_index,  start_ts, AVSEEK_FLAG_BACKWARD );
//    }

    //AVSEEK_FLAG_ANY搜到的不是关键帧，有可能会有一段时间没有视频图像
    //AVSEEK_FLAG_BACKWARD找关键帧，可能事件与预期的时间差距比较大
    ret = av_seek_frame( ifmt_ctx,  -1 ,  startTime * AV_TIME_BASE  ,  AVSEEK_FLAG_BACKWARD );
//
    int endTime = startTime + duration;

    //记录开始时间，要修正流的pts和dts
    //pinky:不应该分别记录dts和pts的开始值，因为pts,dts在同一个时间轴上，如果记录两个时间，会错误记录pts和dts的先后，从而引起问题
//    int64_t *dts_start_from = malloc( sizeof(int64_t) * ifmt_ctx->nb_streams );
//    memset( dts_start_from, 0 ,sizeof(int64_t)* ifmt_ctx->nb_streams);
    int64_t *pts_start_from = malloc(sizeof(int64_t) * ifmt_ctx->nb_streams);
    memset(pts_start_from, 0, sizeof(int64_t) * ifmt_ctx->nb_streams);
    
    
    
    while(1)
    {
        AVStream* in_stream, *out_stream;
        
        ret = av_read_frame( ifmt_ctx, &pkt );
        if( ret < 0 )
            break;
        
            
        log_packet( ifmt_ctx,  &pkt, "in");
        in_stream = ifmt_ctx->streams[ pkt.stream_index ];
        pkt.stream_index = stream_mapping[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index ];
        
        if(  av_q2d( in_stream->time_base ) * pkt.pts > endTime )
        {
            break;
        }
        
//        if (dts_start_from[pkt.stream_index] == 0) {
//            dts_start_from[pkt.stream_index] = pkt.dts;
//            printf("dts_start_from: %s\n", av_ts2str(dts_start_from[pkt.stream_index]));
//        }
        if (pts_start_from[pkt.stream_index] == 0) {
            pts_start_from[pkt.stream_index] = pkt.pts;
            printf("pts_start_from: %s\n", av_ts2str(pts_start_from[pkt.stream_index]));
        }
        
        
        //copy packet
        //pinky:AV_ROUND_PASS_MINMAX表示如果遇到最大最小值，直接返回
        pkt.pts = av_rescale_q_rnd( pkt.pts-pts_start_from[pkt.stream_index] , in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd( pkt.dts-pts_start_from[pkt.stream_index] , in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        
        if( pkt.pts < 0 )
            pkt.pts = 0;
        if( pkt.dts < 0 )
            pkt.dts = 0;
        
        log_packet( ofmt_ctx,  &pkt, "out");
        
        ret = av_interleaved_write_frame( ofmt_ctx,  &pkt );
        if( ret < 0 )
        {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }
        
        av_packet_unref( &pkt );
    }
    
    av_write_trailer( ofmt_ctx );
    
    free( pts_start_from );
        
end:
    //close input and output
    avformat_close_input(& ifmt_ctx );
    
    if( ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE ) )
        avio_closep( &ofmt_ctx->pb );
    
    avformat_free_context( ofmt_ctx);
    
    av_freep( &stream_mapping );
    
    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }
    
    return 0;
}
int concat( char* infile, char* infile2, char* outfile )
{
    return 0;
}
