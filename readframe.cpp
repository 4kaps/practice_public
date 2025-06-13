#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}
using namespace std;

int main(int argc, char* argv[]){ 
    // ./readframe <input> 
    // argc = 2, argv[0] = ./readframe, argv[1] = <input>

    if(argc < 2){
        cout << "Usage : " << argv[0] << "<input>" << '\n';
        return 1; // retry
    }

    const char* input_file = argv[1];
    
    // open input file 
    AVFormatContext* fmt_ctx = NULL;

    // fail to open input file
    if(avformat_open_input(&fmt_ctx, input_file, NULL, NULL) < 0){
        cout << "Could not open file : " << input_file << '\n';
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // stream info missing
    if(avformat_find_stream_info(fmt_ctx, NULL) < 0){
        cout << "Could not find stream info" << '\n';
        return 1;
    }

    // find video stream
    int stream_idx = -1;
    for(int i=0; i<fmt_ctx->nb_streams; i++){
        if(fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            stream_idx = i;
            break;
        }
    }

    // codec parameters & context
    AVCodecParameters* codecpar = fmt_ctx->streams[stream_idx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codecpar);

    if(avcodec_open2(codec_ctx, codec, NULL) < 0){
        cout << "Could not open codec" << '\n';
        avcodec_free_context(&codec_ctx);
    }

    AVFrame* frame = av_frame_alloc();
    AVPacket pkt;
    av_init_packet(&pkt);

    while(av_read_frame(fmt_ctx, &pkt) >= 0){
        if(pkt.stream_index == stream_idx){
            int ret = avcodec_send_packet(codec_ctx, &pkt); // decode frame
            while(ret >= 0){
                if(avcodec_receive_frame(codec_ctx, frame) == AVERROR(EAGAIN)){
                    break;
                }else if(avcodec_receive_frame(codec_ctx, frame) == AVERROR_EOF){
                    break;
                }
                
                cout << "frame : " << codec_ctx->frame_number << '\n';
                cout << "resolution : " << frame->width << " X " << frame->height << '\n';
            }
        }
        av_packet_unref(&pkt);
    }

    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return 0;
}