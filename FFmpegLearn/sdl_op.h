//
//  sdl_op.h
//  FFmpegLearn
//
//  Created by pinky on 2020/4/20.
//  Copyright © 2020 pinky. All rights reserved.
//

#ifndef sdl_op_h
#define sdl_op_h

#include <stdio.h>
#include "SDL.h"

/*
 步骤：
 1.SDL_INIT(SDL_QUIT)
 2.SDL_OpenAudio( 设置AudioSpec的各种参数,包括callback)/SDL_CloseAudio
 3.SDL_PauseAudio( 开始播放)
 4.callback里处理数据读入
 */
int play_audio_pcm( char* filename, int samplerate, int channel, SDL_AudioFormat sample_fmt  );


int play_video_yuv( char* filename, int vwidth , int vheight );


#endif /* sdl_op_h */
