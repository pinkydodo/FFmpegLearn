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
