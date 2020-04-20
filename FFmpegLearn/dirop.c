//
//  dirop.c
//  extraAudio
//
//  Created by pinky on 2020/4/11.
//  Copyright © 2020 pinky. All rights reserved.
//

#include "dirop.h"
#include "libavformat/avio.h"

static const char *type_string(int type)
{
    switch (type) {
    case AVIO_ENTRY_DIRECTORY:
        return "<DIR>";
    case AVIO_ENTRY_FILE:
        return "<FILE>";
    case AVIO_ENTRY_BLOCK_DEVICE:
        return "<BLOCK DEVICE>";
    case AVIO_ENTRY_CHARACTER_DEVICE:
        return "<CHARACTER DEVICE>";
    case AVIO_ENTRY_NAMED_PIPE:
        return "<PIPE>";
    case AVIO_ENTRY_SYMBOLIC_LINK:
        return "<LINK>";
    case AVIO_ENTRY_SOCKET:
        return "<SOCKET>";
    case AVIO_ENTRY_SERVER:
        return "<SERVER>";
    case AVIO_ENTRY_SHARE:
        return "<SHARE>";
    case AVIO_ENTRY_WORKGROUP:
        return "<WORKGROUP>";
    case AVIO_ENTRY_UNKNOWN:
    default:
        break;
    }
    return "<UNKNOWN>";
}

int list_dir( const  char* dir )
{
    AVIODirContext * ctx = NULL;
    AVIODirEntry* entry = NULL;
    int ret = 0 ;
    int cnt = 0;
    
    char filemode[4],uid_and_gid[20];
    if( (ret = avio_open_dir( &ctx, dir, NULL) ) < 0 )
    {
        av_log( NULL, AV_LOG_ERROR, "Cannot open dir:%s\n", av_err2str( ret ));
        goto fail;
    }
    
    for(;;)
    {
        if( ( ret = avio_read_dir( ctx,  &entry ))<0)
        {
            av_log(NULL, AV_LOG_ERROR, "Cannot list dir:%s\n", av_err2str( ret ));
            goto fail;
        }
        
        if( !entry )
            break;
        
        if( entry->filemode == -1 ){
            snprintf( filemode, 4, "???");
        }
        else{
            snprintf( filemode, 4, "%3"PRIo64, entry->filemode);
        }
        
        snprintf( uid_and_gid,  20 , "%"PRId64"(%"PRId64")", entry->user_id, entry->group_id );
        if( cnt == 0 )
        {
            av_log(NULL, AV_LOG_INFO, "%-9s %12s %30s %10s %s %16s %16s %16s\n",
            "TYPE", "SIZE", "NAME", "UID(GID)", "UGO", "MODIFIED",
            "ACCESSED", "STATUS_CHANGED");
        }
        
        av_log(NULL, AV_LOG_INFO, "%-9s %12"PRId64" %30s %10s %s %16"PRId64" %16"PRId64" %16"PRId64"\n", type_string(entry->type),
        entry->size,
        entry->name,
        uid_and_gid,
        filemode,
        entry->modification_timestamp,
        entry->access_timestamp,
        entry->status_change_timestamp);
        
        avio_free_directory_entry( &entry );
        cnt++;
    }
    
    
    
fail:
    avio_close_dir(&ctx );
    
    return ret;
}

int move_dir( const  char* from , const  char* to )
{
    int ret = avpriv_io_move( from ,  to );
    if( ret < 0 )
    {
        av_log( NULL, AV_LOG_ERROR, "Cannot move:'%s' to '%s': %s\n", from , to ,av_err2str( ret ));
    }
    return 0;
}

int del_dir( const  char* dir )
{
    int ret = avpriv_io_delete( dir );
    if( ret < 0 )
    {
        av_log( NULL, AV_LOG_ERROR, "Cannot delete '%s':%s\n", dir, av_err2str( ret ));
    }
    return ret ;
}
