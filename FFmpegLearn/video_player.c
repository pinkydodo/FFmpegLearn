//
//  video_player.c
//  FFmpegLearn
//
//  Created by pinky on 2020/5/2.
//  Copyright © 2020 pinky. All rights reserved.
//

#include "video_player.h"
#include "SDL.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include <libswscale/swscale.h>

struct CH264Player
{
    //sdl
    SDL_Window* win;
    SDL_Renderer* render;
    SDL_Texture* texture;
    Uint32 pixformat;
    
    //ffmpeg
    AVFormatContext* m_fctx;
    AVCodec* codec;
    AVCodecContext* m_cctx;
    struct SwsContext *sws_ctx;
    int video_stream_index;
    
};

struct CH264Player g_h264Player;
void initPlayer();
void setInFile( char* file );
void startPlay();
void releasePlayer();
void playVideoH264( char* fileName )
{
    initPlayer();
    setInFile( fileName );
    startPlay();
    releasePlayer();
}

void initPlayer()
{
    int w_width = 640;
    int w_height = 480;
    int ret = 0;
    //Init SDL
    SDL_Init( SDL_INIT_VIDEO );
    g_h264Player.pixformat = SDL_PIXELFORMAT_IYUV;
    
    g_h264Player.win = SDL_CreateWindow("h264 player ", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                           w_width, w_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    
    if( !g_h264Player.win )
    {
        SDL_LogError( SDL_LOG_CATEGORY_APPLICATION, "Failed to create window by SDL ");
        return ;
    }
    
    g_h264Player.render = SDL_CreateRenderer( g_h264Player.win, -1 , 0);
    if(!g_h264Player.render){
       SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Renderer by SDL");
        return ;
    }
    

    
    
    //Init ffmpeg
//    需要知道文件了才能初始化，那就算了
    
}
void setInFile( char* file )
{
    int ret = 0;
    //open file
    if( avformat_open_input(&g_h264Player.m_fctx,  file , NULL, NULL ) != 0 )
    {
        printf("Failed to open input\n");
        return ;
    }
    
    if( avformat_find_stream_info( g_h264Player.m_fctx,  NULL ) <  0)
    {
        printf("Failed to find stream info\n");
        return ;
    }
    
    av_dump_format( g_h264Player.m_fctx,  0 ,  file ,  0 );
    
    g_h264Player.video_stream_index = av_find_best_stream( g_h264Player.m_fctx, AVMEDIA_TYPE_VIDEO, 0, 0, &g_h264Player.codec, 0 );
    
    if( !g_h264Player.codec )
    {
        printf("Could not find decoder \n");
        return;
    }
    
    AVStream* stream = g_h264Player.m_fctx->streams[ g_h264Player.video_stream_index ];
    g_h264Player.m_cctx = avcodec_alloc_context3( g_h264Player.codec);
    //copy context param
    avcodec_parameters_to_context(g_h264Player.m_cctx, stream->codecpar );
    
    //open codec
    if( avcodec_open2( g_h264Player.m_cctx, g_h264Player.codec,  NULL))
    {
        printf("Failed to open decoder\n ");
        return ;
    }
    
    g_h264Player.sws_ctx = sws_getContext( g_h264Player.m_cctx->width, g_h264Player.m_cctx->height, g_h264Player.m_cctx->pix_fmt, g_h264Player.m_cctx->width, g_h264Player.m_cctx->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR,  NULL, NULL, NULL);
    
    
    g_h264Player.texture = SDL_CreateTexture( g_h264Player.render,  g_h264Player.pixformat, SDL_TEXTUREACCESS_STREAMING,
                                             g_h264Player.m_cctx->width, g_h264Player.m_cctx->height );
}
void startPlay()
{
    int ret = 0;
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();
    AVPicture* pic = (AVPicture*)malloc( sizeof(AVPicture));
    avpicture_alloc( pic ,  AV_PIX_FMT_YUV420P,  g_h264Player.m_cctx->width, g_h264Player.m_cctx->height );
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = g_h264Player.m_cctx->width  ;
    rect.h = g_h264Player.m_cctx->height  ;
    
    while( av_read_frame( g_h264Player.m_fctx,  packet) >= 0 )
    {
        if( packet->stream_index != g_h264Player.video_stream_index )
        {
            continue;
        }
        
        if( avcodec_send_packet( g_h264Player.m_cctx,  packet ) < 0 )
        {
            printf("Failed send packet\n");
            break;
        }
        
        if( avcodec_receive_frame( g_h264Player.m_cctx,  frame ) == 0 )
        {
            sws_scale( g_h264Player.sws_ctx, (const uint8_t *const *) frame->data,
                      frame->linesize, 0, g_h264Player.m_cctx->height, pic->data , pic->linesize );
            
            SDL_SetRenderTarget(g_h264Player.render, g_h264Player.texture   );
            SDL_RenderClear( g_h264Player.render );
            SDL_UpdateYUVTexture(g_h264Player.texture,NULL , pic->data[0], pic->linesize[0],
                                 pic->data[1], pic->linesize[1],
                                 pic->data[2], pic->linesize[2]);
            
//            SDL_UpdateYUVTexture(g_h264Player.texture, NULL , frame->data[0], frame->linesize[0],
//                                 frame->data[1], frame->linesize[1],
//                                 frame->data[2], frame->linesize[2]);
            
//            SDL_UpdateTexture( g_h264Player.texture, NULL, pic->data,  pic->linesize[0] );
            
            SDL_SetRenderTarget( g_h264Player.render,  NULL );
            SDL_RenderClear( g_h264Player.render );
            SDL_RenderCopy( g_h264Player.render, g_h264Player.texture,  NULL, NULL );
            SDL_RenderPresent( g_h264Player.render );
        }
        
        av_packet_unref( packet );
        
//        SDL_Event evt;
//        int ret = SDL_WaitEventTimeout( &evt.quit ,  20 );
//        if( ret == 0 )
//        {
//            continue;
//        }
//        else{
//            break;
//        }
        SDL_Delay( 100 );
        SDL_Event evt;
        SDL_PollEvent( &evt);
        switch ( evt.type) {
            case SDL_QUIT:
                goto __QUIT;
                break;

            default:
                break;
        }
    }
    
    av_frame_free( &frame );
    avpicture_free( pic );
    free( pic );

    
    
__QUIT:
    return ;
}

void releasePlayer()
{
    if( g_h264Player.m_cctx )
    {
        avcodec_close( g_h264Player.m_cctx );
    }
    
    if( g_h264Player.m_fctx )
    {
        avformat_close_input( &g_h264Player.m_fctx );
    }
    
    if( g_h264Player.sws_ctx )
    {
        sws_freeContext( g_h264Player.sws_ctx );
    }
    
    if( g_h264Player.win )
    {
        SDL_DestroyWindow( g_h264Player.win);
    }
    
    if( g_h264Player.render )
    {
        SDL_DestroyRenderer( g_h264Player.render );
    }
    
    if( g_h264Player.texture )
    {
        SDL_DestroyTexture( g_h264Player.texture );
    }
    
    SDL_Quit();
    
}
