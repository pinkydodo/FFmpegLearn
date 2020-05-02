//
//  video_player.h
//  FFmpegLearn
//
//  Created by pinky on 2020/5/2.
//  Copyright © 2020 pinky. All rights reserved.
//

#ifndef video_player_h
#define video_player_h

#include <stdio.h>


/*
 播放h264流程
 1.初始化SDL,创建窗口，创建渲染器
 2.打开要播放的文件，准备流信息
 3.创建，打开解码器
 4.创建，sws_context， 转换size和pix_format
 5.循环处理
    a.读取一个packet
    b.解码packet
    c.对解码后的frame进行sws转换
    d.渲染：updateTexture, clearRender, copy, present
 6.结束释放各种资源
 
 疑问：
 1.输入数据小于纹理选择更新的画面是，会怎么样
 2.目前画面下方有4个小图像，为什么？
 */
void playVideoH264( char* fileName );
#endif /* video_player_h */
