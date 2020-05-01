//
//  sdl_op.c
//  FFmpegLearn
//
//  Created by pinky on 2020/4/20.
//  Copyright © 2020 pinky. All rights reserved.
//

#include "sdl_op.h"

#include "SDL.h"

#define BLOCK_SIZE 4096000
static Uint8* audio_buf = NULL;
static Uint8* audio_pos = NULL;
static size_t buffer_len = 0;

void MyAudioCallback( void * userdata,
                     Uint8* stream, int len )
{
    if( buffer_len == 0 )
    {
        return;
    }
    
    SDL_memset( stream , 0,  len );
    
    int get = len ;
    if( get  > buffer_len )
    {
        get = buffer_len;
    }
    
    SDL_MixAudio( stream ,  audio_pos,  get , SDL_MIX_MAXVOLUME );
    audio_pos += len;
    buffer_len -= len;
    
}

int play_audio_pcm( char* filename, int samplerate, int channel, SDL_AudioFormat sample_fmt  )
{
    int ret = 0;
    
    FILE* fd = fopen( filename, "rb");
    if( !fd )
    {
        printf("open pcm failed\n");
        return -1;
    }
    
    audio_buf = (Uint8*) malloc( BLOCK_SIZE);
    if( !audio_buf )
    {
        printf("Failed to open pcm file\n");
        
    }
    ret = SDL_Init( SDL_INIT_AUDIO);
    if( ret != 0  )
    {
        printf("Init Error\n");
        return -1;
    }
    
    SDL_AudioSpec spec;
    spec.freq = samplerate;
    spec.format = sample_fmt;
    spec.channels= channel;
    spec.samples = 4096 ;
    spec.callback = MyAudioCallback;
    spec.userdata = NULL;
    
    ret = SDL_OpenAudio( &spec, NULL );
    if( ret !=  0)
    {
        printf("Open Audio failed\n");
        return -1;
    }
    
    //start play
    SDL_PauseAudio( 0 );
    
    do{
        buffer_len = fread( audio_buf, 1, BLOCK_SIZE, fd );
        printf( "block size is %zu\n", buffer_len );
        audio_pos = audio_buf;
        Uint8* audio_end = audio_buf + buffer_len;
        while( audio_pos <  audio_end )
        {
            SDL_Delay( 1 );
        }
        
    }while( buffer_len != 0 );
    
    SDL_CloseAudio();
    
    if( audio_buf )
    {
        free( audio_buf );
    }
    SDL_Quit();
    return 0;
}


#define BLOCK_SIZE 4096000
#define QUIT_EVENT (SDL_USEREVENT+2)
#define REFRESH_EVENT  (SDL_USEREVENT + 1 )
int thread_exit = 0;
int refresh_video_timer( void* udata )
{
    thread_exit = 0;
    while(!thread_exit)
    {
        SDL_Event evt;
        evt.type = REFRESH_EVENT;
        SDL_PushEvent( &evt );
        SDL_Delay( 40 );
    }
    
    thread_exit = 0;
    SDL_Event event;
    event.type = QUIT_EVENT;
    SDL_PushEvent( &event );
    
    return 0;
}

int play_video_yuv( char* filename, int vwidth , int vheight )
{
    //打开窗口
    SDL_Event evt;
    SDL_Rect rect;
    
    Uint32 pixformat = 0;
    
    SDL_Window* win = NULL;
    SDL_Renderer* render = NULL;
    SDL_Texture* texture = NULL;
    
    SDL_Thread* timer_thread = NULL;
    
    int w_width = 640, w_height = 480;
    
    FILE* video_fd = NULL;
    Uint8* video_pos = NULL;
    Uint8* video_end = NULL;
    
    unsigned int video_buff_len = 0;
    uint8_t video_buf[ BLOCK_SIZE ];
    unsigned int remain_len = 0;
    unsigned int blank_space_len = 0;
    
    if( SDL_Init( SDL_INIT_VIDEO ))
    {
        fprintf(stderr, "Could not init SDL:%s\n", SDL_GetError() );
        return -1;
    }
    
    const unsigned int yuv_frame_len = vwidth * vheight * 12 / 8;
    
    win = SDL_CreateWindow("YUV player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w_width, w_height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if(!win)
    {
        fprintf(stderr, "Failed to create window, %s\n", SDL_GetError());
        return -1;
    }
    
    render = SDL_CreateRenderer( &win,  -1 ,  0 );
    
    //IYUV: Y + U + V  (3 planes)
    //YV12: Y + V + U  (3 planes)
    pixformat= SDL_PIXELFORMAT_IYUV;
    
    
    //创建纹理
    texture = SDL_CreateTexture( render, pixformat, SDL_TEXTUREACCESS_STREAMING, vwidth, vheight);
    
    video_fd = fopen( filename, "r");
    if( !video_fd)
    {
        fprintf(stderr, "Failed to open yuv file\n");
        return -1;
    }
    
    if( video_buff_len = fread( video_fd, 1 , BLOCK_SIZE,  video_fd) <= 0 )
    {
        fprintf(stderr, "Failed to read data from yuv file!\n");
        return -1;
    }
    
    video_pos = video_buf;
    video_end = video_buf + video_buff_len;
    
    
    timer_thread = SDL_CreateThread( refresh_video_timer, NULL, NULL );
    
    //wait事件
    do{
        SDL_WaitEvent( &evt );
        if( evt.type == REFRESH_EVENT )
        {
            if( video_pos + yuv_frame_len > video_end )
            {
                //移动剩下的到一开始
                remain_len = video_end - video_pos;
                if( remain_len && !blank_space_len )
                {
                    memcpy( video_buf, video_pos, remain_len );
                    
                    blank_space_len = BLOCK_SIZE - remain_len;
                    video_pos = video_buf;
                    video_end = video_buf + remain_len ;
                }
                
                
                if( video_end == ( video_buf + BLOCK_SIZE )){
                    video_pos = video_buf;
                    video_end = video_buf;
                    blank_space_len = BLOCK_SIZE;
                }
                //再读一个buff
                if( video_buff_len = fread( video_end, 1 , blank_space_len ,video_fd )){
                    fprintf( stderr, "eof , exit thread ");
                    thread_exit = 1;
                    continue;  // to wait event for exiting
                }
                else{
                    video_end += video_buff_len;
                    blank_space_len -= video_buff_len;
                }
            }
            
            SDL_UpdateTexture( texture, NULL, video_pos, vwidth);
            
            rect.x = 0;
            rect.y = 0;
            rect.w = w_width;
            rect.h = w_height;
            
            SDL_RenderClear( render );
            SDL_RenderCopy( render, texture,  NULL,  &rect );
            SDL_RenderPresent( render );
            
        }
        else if( evt.type = SDL_QUIT)
        {
            thread_exit = 1;
        }
        else if( evt.type == QUIT_EVENT )
        {
            break;
        }
    }while(1);
    //refresh 更新纹理
    
    
    if( video_fd)
    {
        fclose( video_fd);
    }
    
    SDL_Quit();
    //quit 处理退出
    return 0;
    
}
