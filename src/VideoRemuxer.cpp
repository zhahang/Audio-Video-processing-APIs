#include "VideoRemuxer.h"


bool VideoRemuxerEncode(string &video_name, string &audio_name, int offset, string &output_video_name)
{
	av_register_all();

	int frameTol = 0;
	//input video src decoder config
	AVFormatContext *in_fmt_ctx_v = NULL;
	AVCodecContext  *in_codec_ctx_v = NULL;
	AVCodec         *in_codec_v = NULL;
	int v_video_stream_index = -1;
	if (avformat_open_input(&in_fmt_ctx_v, video_name.c_str(), NULL, NULL) != 0)
	{
		printf("error : cannot OPEN file %s...\n", video_name.c_str());
		return false;
	}
	if (avformat_find_stream_info(in_fmt_ctx_v, NULL) < 0)
	{
		printf("error : cannot READ stream...\n");
		return false;
	}
	for (int i = 0; i < in_fmt_ctx_v->nb_streams; i++)
	{
		AVMediaType codecType = in_fmt_ctx_v->streams[i]->codec->codec_type;
		if (codecType == AVMEDIA_TYPE_VIDEO)
		{
			in_codec_ctx_v = in_fmt_ctx_v->streams[i]->codec;
			frameTol = in_fmt_ctx_v->streams[i]->nb_frames;
			v_video_stream_index = i;
		}
	}
	if (!in_codec_ctx_v)
	{
		printf("error : cannot find video stream...\n");
		return false;
	}

	in_codec_v = avcodec_find_decoder(in_codec_ctx_v->codec_id);
	if (in_codec_v == NULL)
	{
		printf("error : cannot find video decoder...\n");
		return false;
	}
	if (avcodec_open2(in_codec_ctx_v, in_codec_v, NULL) < 0)
	{
		printf("error : cannot open video decoder...\n");
		return false;
	}

	int width = in_codec_ctx_v->width;
	int	height = in_codec_ctx_v->height;

	AVFrame *in_v_yuv_frame = av_frame_alloc();
	AVFrame *in_v_rgb_frame = av_frame_alloc();
	in_v_yuv_frame->format = AV_PIX_FMT_YUV420P;
	in_v_rgb_frame->format = AV_PIX_FMT_BGR24;

	AVPacket in_video_packet;
	av_init_packet(&in_video_packet);

	//input audio src decoder config
	AVFormatContext *in_fmt_ctx_a = NULL;
	AVCodecContext  *in_codec_ctx_a_video = NULL;
	AVCodecContext  *in_codec_ctx_a_audio = NULL;
	AVCodec         *in_codec_a_video = NULL;
	AVCodec         *in_codec_a_audio = NULL;
	double in_a_frame_rate;
	AVRational in_a_timebase;
	int in_sample_rate;
	int in_audio_bit_rate;
	int a_video_stream_index = -1;
	int a_audio_stream_index = -1;

	if (avformat_open_input(&in_fmt_ctx_a, audio_name.c_str(), NULL, NULL) != 0)
	{
		printf("error : cannot open file %s...\n", audio_name.c_str());
		return false;
	}
	if (avformat_find_stream_info(in_fmt_ctx_a, NULL) < 0)
	{
		printf("error : cannot read stream...\n");
		return false;
	}
	for (int i = 0; i < in_fmt_ctx_a->nb_streams; i++)
	{
		AVMediaType codecType = in_fmt_ctx_a->streams[i]->codec->codec_type;
		if (codecType == AVMEDIA_TYPE_VIDEO)
		{
			in_codec_ctx_a_video = in_fmt_ctx_a->streams[i]->codec;
			in_a_frame_rate = av_q2d(in_fmt_ctx_a->streams[i]->r_frame_rate);
			in_a_timebase = in_fmt_ctx_a->streams[i]->time_base;
			a_video_stream_index = i;
		}

		if (codecType == AVMEDIA_TYPE_AUDIO)
		{
			in_codec_ctx_a_audio = in_fmt_ctx_a->streams[i]->codec;
			in_sample_rate = in_codec_ctx_a_audio->sample_rate;
			in_audio_bit_rate = in_codec_ctx_a_audio->bit_rate;
			a_audio_stream_index = i;
		}
	}
	if (!in_codec_ctx_a_video || !in_codec_ctx_a_audio)
	{
		printf("error : cannot find video or audio stream %s...\n", audio_name.c_str());
		return false;
	}

	in_codec_a_video = avcodec_find_decoder(in_codec_ctx_a_video->codec_id);
	if (!in_codec_a_video)
	{
		printf("error : cannot find video decoder...\n");
		return false;
	}
	if (avcodec_open2(in_codec_ctx_a_video, in_codec_a_video, NULL) < 0)
	{
		printf("error : cannot open video decoder...\n");
		return false;
	}

	in_codec_a_audio = avcodec_find_decoder(in_codec_ctx_a_audio->codec_id);
	if (!in_codec_a_audio)
	{
		printf("error : cannot find video decoder...\n");
		return false;
	}
	if (avcodec_open2(in_codec_ctx_a_audio, in_codec_a_audio, NULL) < 0)
	{
		printf("error : cannot open video decoder...\n");
		return false;
	}

	AVFrame *in_a_yuv_frame = av_frame_alloc();
	AVPacket in_a_video_packet;
	av_init_packet(&in_a_video_packet);

	uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
	int out_nb_samples = in_codec_ctx_a_audio->frame_size;
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	int out_sample_rate = in_codec_ctx_a_audio->sample_rate;
	int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);

	int in_acc_buf_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);
	uint8_t *in_acc_buf = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

	SwrContext *in_au_convert_ctx = swr_alloc();
	in_au_convert_ctx = swr_alloc_set_opts(in_au_convert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate,
		in_codec_ctx_a_audio->channel_layout, in_codec_ctx_a_audio->sample_fmt, in_codec_ctx_a_audio->sample_rate, 0, NULL);
	swr_init(in_au_convert_ctx);

	AVFrame *in_acc_frame = av_frame_alloc();
	AVPacket in_audio_packet;
	av_init_packet(&in_audio_packet);

	//output video Encoder config
	AVFormatContext  *out_fmt_cxt = NULL;
	AVOutputFormat   *out_fmt = NULL;
	AVStream         *out_v_stream = NULL;
	AVStream         *out_a_stream = NULL;

	AVCodecContext   *out_v_codec_ctx = NULL;
	AVCodecContext   *out_a_codec_ctx = NULL;

	AVCodec          *out_v_codec = NULL;
	AVCodec          *out_a_codec = NULL;

	AVFrame          *out_yuv_frame = NULL;
	AVFrame          *out_rgb_frame = NULL;
	AVFrame          *out_acc_frame = NULL;

	SwsContext       *out_sws_ctx = NULL;
	AVPacket          out_v_packet;
	AVPacket          out_a_packet;


	avformat_alloc_output_context2(&out_fmt_cxt, NULL, NULL, output_video_name.c_str());
	out_fmt = out_fmt_cxt->oformat;
	if (avio_open(&out_fmt_cxt->pb, output_video_name.c_str(), AVIO_FLAG_READ_WRITE) < 0)
	{
		printf("error: cannot OPEN Format Context...\n");
		return false;
	}

	out_v_stream = avformat_new_stream(out_fmt_cxt, 0);
	if (out_v_stream == NULL)
	{
		printf("error : cannot Open video Stream...\n");
		return false;
	}
	out_v_stream->codec->codec_tag = 0;
	if (out_fmt_cxt->oformat->flags & AVFMT_GLOBALHEADER)
		out_v_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

	out_a_stream = avformat_new_stream(out_fmt_cxt, 0);
	if (out_a_stream == NULL)
	{
		printf("error : cannot Open audio Stream...\n");
		return false;
	}
	out_a_stream->codec->codec_tag = 0;
	if (out_fmt_cxt->oformat->flags & AVFMT_GLOBALHEADER)
		out_a_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

	out_v_codec_ctx = out_v_stream->codec;
	out_v_codec_ctx->codec_id = out_fmt->video_codec;
	out_v_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	out_v_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	out_v_codec_ctx->width = width;
	out_v_codec_ctx->height = height;
	out_v_codec_ctx->time_base.num = 1;
	out_v_codec_ctx->time_base.den = 30;
	out_v_codec_ctx->bit_rate = 400000000;
	out_v_codec_ctx->gop_size = 30;
	out_v_codec_ctx->qmin = 10;
	out_v_codec_ctx->qmax = 30;
	out_v_codec_ctx->max_qdiff = 4;
	out_v_codec_ctx->qcompress = (float)0.6;
	out_v_codec_ctx->sample_aspect_ratio.num = 1; // nWidth;
	out_v_codec_ctx->sample_aspect_ratio.den = 1; // nHeight;

	AVDictionary *param = NULL;
	//if codec_id is h264, set encodec delay = 0
	if (out_v_codec_ctx->codec_id == AV_CODEC_ID_H264)
	{
		av_dict_set(&param, "preset", "superfast", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
	}
	//H.265    
	if (out_v_codec_ctx->codec_id == AV_CODEC_ID_H265)
	{
		av_dict_set(&param, "x265-params", "qp=20", 0);
		av_dict_set(&param, "preset", "ultrafast", 0);
		av_dict_set(&param, "tune", "zero-latency", 0);
	}

	out_a_codec_ctx = out_a_stream->codec;
	out_a_codec_ctx->codec_id = out_fmt->audio_codec;
	out_a_codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
	//out_a_codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
	out_a_codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
	out_a_codec_ctx->sample_rate = in_sample_rate;
	out_a_codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
	out_a_codec_ctx->channels = av_get_channel_layout_nb_channels(out_a_codec_ctx->channel_layout);
	out_a_codec_ctx->bit_rate = in_audio_bit_rate;
	out_a_codec_ctx->frame_size = 1024;

	int out_byte = width * height * 3 * sizeof(uint8_t);
	av_new_packet(&out_v_packet, out_byte);

	out_acc_frame = av_frame_alloc();
	out_acc_frame->nb_samples = out_a_codec_ctx->frame_size;
	out_acc_frame->format = out_a_codec_ctx->sample_fmt;
	int audio_size = av_samples_get_buffer_size(NULL, out_a_codec_ctx->channels, out_a_codec_ctx->frame_size, out_a_codec_ctx->sample_fmt, 1);
	av_new_packet(&out_a_packet, audio_size);
	uint8_t* out_acc_buf = (uint8_t *)av_malloc(audio_size);
	avcodec_fill_audio_frame(out_acc_frame, out_a_codec_ctx->channels, out_a_codec_ctx->sample_fmt, (const uint8_t*)out_acc_buf, audio_size, 1);

	out_v_codec = avcodec_find_encoder(out_v_codec_ctx->codec_id);
	if (!out_v_codec)
	{
		printf("error 9: cannot find video Encoder...\n");
		return false;
	}
	if (avcodec_open2(out_v_codec_ctx, out_v_codec, &param) < 0)
	{
		printf("error : cannot OPEN Encoder");
		return false;
	}

	out_a_codec = avcodec_find_encoder(out_a_codec_ctx->codec_id);
	if (!out_a_codec)
	{
		printf("error : cannot FIND audio Encoder...\n");
		return false;
	}
	if (avcodec_open2(out_a_codec_ctx, out_a_codec, NULL) < 0)
	{
		printf("error [%d]: cannot OPEN audio Encoder ...\n");
		return false;
	}

	//begin merge audio into video
	if (avformat_write_header(out_fmt_cxt, NULL) < 0)
	{
		printf("error [%d]: cannot write header...\n");
		return false;
	}

	//audio decoding and encoding ,one by one 
	int count = 0;
	int64_t pts = 0;
	int64_t dts = 0;
	int is_get_audio_frame = false;
	while (!is_get_audio_frame)
	{
		if (count > frameTol + offset)
			break;
		int ret = av_read_frame(in_fmt_ctx_a, &in_audio_packet);
		if (ret < 0)
		{
			av_free_packet(&in_audio_packet);
			break;
		}
		if (in_audio_packet.stream_index != a_audio_stream_index)
		{
			av_free_packet(&in_audio_packet);
			if (in_audio_packet.stream_index == a_video_stream_index)
				count++;
			continue;
		}

		if (count < offset + 1)
		{
			pts = in_audio_packet.pts;
			dts = in_audio_packet.dts;
			continue;
		}
		in_audio_packet.pts -= pts;
		in_audio_packet.dts -= dts;

		int got_frame;
		ret = avcodec_decode_audio4(in_codec_ctx_a_audio, in_acc_frame, &got_frame, &in_audio_packet);
		if (ret < 0) {
			char buf[1024];
			av_strerror(ret, buf, sizeof(buf));
			printf("error [%d]: cannot OPEN video Encoder [%s]...\n", ret, buf);
			continue;
		}
		if (got_frame > 0)
		{
			swr_convert(in_au_convert_ctx, &in_acc_buf, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)in_acc_frame->data, in_acc_frame->nb_samples);
			out_acc_frame->data[0] = in_acc_buf;
			out_acc_frame->pts = in_acc_frame->pkt_pts;

			int got_sound = 0;
			ret = avcodec_encode_audio2(out_a_codec_ctx, &out_a_packet, out_acc_frame, &got_sound);
			if (ret < 0)
			{
				char buf[1024];
				av_strerror(ret, buf, sizeof(buf));
				printf("error [%d] : Encoding audio frame fails [%s]\n", ret, buf);
				break;
			}
			if (got_sound == 1) 
			{
				out_a_packet.stream_index = out_a_stream->index;
				ret = av_write_frame(out_fmt_cxt, &out_a_packet);
				if (ret < 0)
				{
					printf("write audio frame fail! in frame_no=%d\n", count);
				}
			}
			av_free_packet(&out_a_packet);
		}
		av_free_packet(&in_audio_packet);
	}
	av_free_packet(&in_audio_packet);

	//video decoding and encoding ,one by one 
	int skipped = 0;
	for (int i = 0; i < frameTol; i++)
	{
		//decoding one frame
		int is_get_video_frame = false;
		while (!is_get_video_frame)
		{
			if (av_read_frame(in_fmt_ctx_v, &in_video_packet) < 0)
			{
				av_free_packet(&in_video_packet);
				break;
			}
			if (in_video_packet.stream_index != v_video_stream_index)
			{
				av_free_packet(&in_video_packet);
				continue;
			}

			int got_picture_ptr;
			int ret = avcodec_decode_video2(in_codec_ctx_v, in_v_yuv_frame, &got_picture_ptr, &in_video_packet);
			if (got_picture_ptr > 0)
			{
				is_get_video_frame = true;
				break;
			}
			else
			{
				skipped++;
			}
			av_free_packet(&in_video_packet);
		}
		if (!is_get_video_frame)
			break;
		av_free_packet(&in_video_packet);

		//encoding video
		int got_picture = 0;
		int ret = avcodec_encode_video2(out_v_codec_ctx, &out_v_packet, in_v_yuv_frame, &got_picture);
		if (ret < 0)
		{
			printf("error : failing in encoder...\n");
			break;
		}
		if (got_picture == 1)
		{
			out_v_packet.stream_index = out_v_stream->index;
			ret = av_write_frame(out_fmt_cxt, &out_v_packet);
			if (ret < 0)
			{
				printf("write video frame fail in frame_no=%d=\n", i);
			}
			else
			{
				printf("writing video frame succ frame_no=%d...\n", i + 1);
			}
		}
		av_free_packet(&out_v_packet);
	}

	//decoding skipped frame and encoding
	for (int i = 0; i < skipped; i++)
	{
		int is_get_video_frame = false;
		while (!is_get_video_frame)
		{
			av_init_packet(&in_video_packet);
			int got_picture_ptr;
			int ret = avcodec_decode_video2(in_codec_ctx_v, in_v_yuv_frame, &got_picture_ptr, &in_video_packet);
			if (got_picture_ptr > 0)
			{
				is_get_video_frame = true;
				break;
			}
			else
			{
				skipped++;
			}
			av_free_packet(&in_video_packet);
		}
		if (!is_get_video_frame)
			break;
		av_free_packet(&in_video_packet);

		//encoding video
		int got_picture = 0;
		int ret = avcodec_encode_video2(out_v_codec_ctx, &out_v_packet, in_v_yuv_frame, &got_picture);
		if (ret < 0)
		{
			printf("error : failing in encoder...\n");
			break;
		}
		if (got_picture == 1)
		{
			out_v_packet.stream_index = out_v_stream->index;
			ret = av_write_frame(out_fmt_cxt, &out_v_packet);
			if (ret < 0)
			{
				printf("write video frame fail in frame_no=%d=\n", i);
			}
			else
			{
				printf("writing video frame succ frame_no=%d...\n", i + 1);
			}
		}
		av_free_packet(&out_v_packet);
	}

	//end merge 
	av_write_trailer(out_fmt_cxt);

	av_free(in_v_yuv_frame);
	av_free(in_v_rgb_frame);
	av_free_packet(&in_video_packet);
	avcodec_close(in_codec_ctx_v);
	avformat_close_input(&in_fmt_ctx_v);

	av_free_packet(&in_a_video_packet);
	av_free(in_acc_buf);
	av_free(in_acc_frame);
	av_free_packet(&in_audio_packet);
	swr_free(&in_au_convert_ctx);

	avcodec_close(in_codec_ctx_a_video);
	avcodec_close(in_codec_ctx_a_audio);
	avformat_close_input(&in_fmt_ctx_a);

	av_free(out_acc_buf);
	av_free(out_acc_frame);
	av_free_packet(&out_v_packet);
	av_free_packet(&out_a_packet);
	sws_freeContext(out_sws_ctx);
	avcodec_close(out_a_codec_ctx);
	avcodec_close(out_v_codec_ctx);
	avformat_close_input(&out_fmt_cxt);
	return true;
}


//----------------------------------------------------------------------------------------------

bool VideoRemuxer(string &in_filename_v, string &in_filename_a, int offset, string &out_filename)
{
	AVOutputFormat *ofmt = NULL;
	//Input AVFormatContext and Output AVFormatContext  
	AVFormatContext *ifmt_ctx_v = NULL, *ifmt_ctx_a = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	int ret, i;
	int videoindex_v = -1, videoindex_out = -1;
	int audioindex_a = -1, audioindex_out = -1;
	int frame_index = 0;
	int64_t cur_pts_v = 0, cur_pts_a = 0;

	av_register_all();
	//Input  
	if ((ret = avformat_open_input(&ifmt_ctx_v, in_filename_v.c_str(), 0, 0)) < 0) {
		printf("Could not open input file.");
		goto end;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx_v, 0)) < 0) {
		printf("Failed to retrieve input stream information");
		goto end;
	}

	if ((ret = avformat_open_input(&ifmt_ctx_a, in_filename_a.c_str(), 0, 0)) < 0) {
		printf("Could not open input file.");
		goto end;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx_a, 0)) < 0) {
		printf("Failed to retrieve input stream information");
		goto end;
	}
	printf("===========Input Information==========\n");
	av_dump_format(ifmt_ctx_v, 0, in_filename_v.c_str(), 0);
	av_dump_format(ifmt_ctx_a, 0, in_filename_a.c_str(), 0);
	printf("======================================\n");
	//Output  
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename.c_str());
	if (!ofmt_ctx) {
		printf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt = ofmt_ctx->oformat;

	for (i = 0; i < ifmt_ctx_v->nb_streams; i++) {
		//Create output AVStream according to input AVStream  
		if (ifmt_ctx_v->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			AVStream *in_stream = ifmt_ctx_v->streams[i];
			AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
			videoindex_v = i;
			if (!out_stream) {
				printf("Failed allocating output stream\n");
				ret = AVERROR_UNKNOWN;
				goto end;
			}
			videoindex_out = out_stream->index;
			//Copy the settings of AVCodecContext  
			if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
				printf("Failed to copy context from input to output stream codec context\n");
				goto end;
			}
			out_stream->codec->codec_tag = 0;
			if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
				out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			break;
		}
	}

	for (i = 0; i < ifmt_ctx_a->nb_streams; i++) {
		//Create output AVStream according to input AVStream  
		if (ifmt_ctx_a->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			AVStream *in_stream = ifmt_ctx_a->streams[i];
			AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
			audioindex_a = i;
			if (!out_stream) {
				printf("Failed allocating output stream\n");
				ret = AVERROR_UNKNOWN;
				goto end;
			}
			audioindex_out = out_stream->index;
			//Copy the settings of AVCodecContext  
			if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
				printf("Failed to copy context from input to output stream codec context\n");
				goto end;
			}
			out_stream->codec->codec_tag = 0;
			if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
				out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

			break;
		}
	}

	printf("==========Output Information==========\n");
	av_dump_format(ofmt_ctx, 0, out_filename.c_str(), 1);
	printf("======================================\n");
	//Open output file  
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&ofmt_ctx->pb, out_filename.c_str(), AVIO_FLAG_WRITE) < 0) {
			printf("Could not open output file '%s'", out_filename);
			goto end;
		}
	}
	//Write file header  
	if (avformat_write_header(ofmt_ctx, NULL) < 0) {
		printf("Error occurred when opening output file\n");
		goto end;
	}
	//skip offset frame
	int skip_frame_count = 0;
	int64_t base_pts_a = 0;
	int64_t base_dts_a = 0;
	int duration_a = 0;
	while (true)
	{
		if(skip_frame_count >= offset)
			break;
		if (av_read_frame(ifmt_ctx_a, &pkt) < 0)
		{
			av_free_packet(&pkt);
			break;
		}
		if (pkt.stream_index == videoindex_v) 
		{
			skip_frame_count++;
		}
		else if (pkt.stream_index == audioindex_a)
		{
			base_pts_a = pkt.pts;
			base_dts_a = pkt.dts;
			duration_a = pkt.duration;
		}
		av_free_packet(&pkt);
	}
	base_pts_a += duration_a;
	base_dts_a += duration_a;

	//FIX  
#if USE_H264BSF  
	AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif  
#if USE_AACBSF  
	AVBitStreamFilterContext* aacbsfc = av_bitstream_filter_init("aac_adtstoasc");
#endif  

	while (1) {
		AVFormatContext *ifmt_ctx;
		int stream_index = 0;
		AVStream *in_stream, *out_stream;

		//Get an AVPacket  
		if (av_compare_ts(cur_pts_v, ifmt_ctx_v->streams[videoindex_v]->time_base, cur_pts_a, ifmt_ctx_a->streams[audioindex_a]->time_base) <= 0) {
			ifmt_ctx = ifmt_ctx_v;
			stream_index = videoindex_out;

			if (av_read_frame(ifmt_ctx, &pkt) >= 0) {
				do {
					in_stream = ifmt_ctx->streams[pkt.stream_index];
					out_stream = ofmt_ctx->streams[stream_index];

					if (pkt.stream_index == videoindex_v) {
						//FIX：No PTS (Example: Raw H.264)  
						//Simple Write PTS  
						if (pkt.pts == AV_NOPTS_VALUE) {
							//Write PTS  
							AVRational time_base1 = in_stream->time_base;
							//Duration between 2 frames (us)  
							int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
							//Parameters  
							pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
							pkt.dts = pkt.pts;
							pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
							frame_index++;
						}

						cur_pts_v = pkt.pts;
						break;
					}
				} while (av_read_frame(ifmt_ctx, &pkt) >= 0);
			}
			else {
				break;
			}

			//FIX:Bitstream Filter  
#if USE_H264BSF  
			av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif  
#if USE_AACBSF  
			av_bitstream_filter_filter(aacbsfc, out_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif  

			//Convert PTS/DTS  
			pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
			pkt.pos = -1;
			pkt.stream_index = stream_index;
		}
		else {
			ifmt_ctx = ifmt_ctx_a;
			stream_index = audioindex_out;
			if (av_read_frame(ifmt_ctx, &pkt) >= 0) {
				do {
					in_stream = ifmt_ctx->streams[pkt.stream_index];
					out_stream = ofmt_ctx->streams[stream_index];

					if (pkt.stream_index == audioindex_a) {
						//FIX：No PTS  
						//Simple Write PTS  
						if (pkt.pts == AV_NOPTS_VALUE) {
							//Write PTS  
							AVRational time_base1 = in_stream->time_base;
							//Duration between 2 frames (us)  
							int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
							//Parameters  
							pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
							pkt.dts = pkt.pts;
							pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
							frame_index++;
						}
						cur_pts_a = pkt.pts - base_pts_a;

						break;
					}
				} while (av_read_frame(ifmt_ctx, &pkt) >= 0);
			}
			else {
				break;
			}


			//FIX:Bitstream Filter  
#if USE_H264BSF  
			av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif  
#if USE_AACBSF  
			av_bitstream_filter_filter(aacbsfc, out_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif  
			//Convert PTS/DTS  
			pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)) - base_pts_a;
			pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)) - base_dts_a;
			pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
			pkt.pos = -1;
			pkt.stream_index = stream_index;
		}

		printf("Write 1 Packet. size:%5d\tpts:%lld\n", pkt.size, pkt.pts);
		//Write  
		if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
			printf("Error muxing packet\n");
			break;
		}
		av_free_packet(&pkt);

	}
	//Write file trailer  
	av_write_trailer(ofmt_ctx);

#if USE_H264BSF  
	av_bitstream_filter_close(h264bsfc);
#endif  
#if USE_AACBSF  
	av_bitstream_filter_close(aacbsfc);
#endif  

end:
	avformat_close_input(&ifmt_ctx_v);
	avformat_close_input(&ifmt_ctx_a);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf("Error occurred.\n");
		return false;
	}
	return true;
}
