//
//  main.cpp
//  extraAudio
//
//  Created by pinky on 2020/4/11.
//  Copyright © 2020 pinky. All rights reserved.
//

//#include <iostream>
#include <stdio.h>
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"

#include "dirop.h"
#include "formatop.h"

#include "sdl_op.h"





int main(int argc, const char * argv[]) {
    // insert code here...
//    std::cout << "Hello, World!\n";
    
//    list_dir("/Users/pinky/avlearn/ffmpegCmd");
//    move_dir( "/Users/pinky/avlearn/ffmpegCmd/image-016.jpeg" , "/Users/pinky/avlearn/ffmpegCmd/image-200.jpeg");
//    del_dir( "/Users/pinky/avlearn/ffmpegCmd/image-200.jpeg" );
//    getInfo( "/Users/pinky/avlearn/avfiles/beauty.mp4");
    
    
//    remux( "/Users/pinky/avlearn/avfiles/beauty.mp4", "/Users/pinky/avlearn/avfiles/beauty.flv");
//    getInfo("/Users/pinky/avlearn/avfiles/beauty.flv");
    
//    crop( "/Users/pinky/avlearn/avfiles/beauty.mp4", 5, 5 , "/Users/pinky/avlearn/avfiles/crop.flv");
    
    //AV_CODEC_ID_AAC
//    show_audio_codec_info_by_id( AV_CODEC_ID_MP2 );
    
//    decode_audio( "/Users/pinky/avlearn/avfiles/beauty.mp4", "/Users/pinky/avlearn/avfiles/beauty.pcm");
    
//    encode_audio("/Users/pinky/avlearn/avfiles/test.mp2");
    
//    play_audio_pcm( "/Users/pinky/avlearn/avfiles/beauty.pcm",  48000, 2, AUDIO_F32LSB  );
    
    play_video_yuv( "/Users/pinky/avlearn/avfiles/beauty.yuv", 640 , 336 );
    return 0;
}

int getInfo( char* fileName )
{
    //regist
    //av_register_all();
    AVFormatContext *ifmt_ctx = NULL;
    
    //open input
    int ret = 0;
    if( (ret = avformat_open_input(&ifmt_ctx, fileName, NULL, NULL) )<0)
    {
        fprintf( stderr, "Could not open input file '%s'", fileName );
        goto end;
    }
    //todo:这个有什么意义呢？
    if( (ret = avformat_find_stream_info( ifmt_ctx, NULL ) ) < 0 )
    {
        fprintf( stderr, "Failed to retrieve input stream information ");
        goto end;
    }
    //dump input
    //index参数有什么用,没看出有啥用
    av_dump_format( ifmt_ctx, 0, fileName, 0 );
    
    //close input
    avformat_close_input( &ifmt_ctx );
end:
    return 0;
}
