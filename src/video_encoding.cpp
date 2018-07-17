#include "video_encoding.h"

bool videoEncoding(vector<string>& imgs_names, Size output_size, int output_br, const char * output_file)
{
	// Setting ffmpeg informations
	AVFormatContext  *pFormatCtxE;
	AVOutputFormat   *pOutFmtE;
	AVStream         *pStreamE;
	AVCodecContext   *pCodecCtxE;
	AVCodec          *pCodecE;
	AVFrame          *pYUVFrameE;
	AVFrame          *pRGBFrameE;
	SwsContext       *pSWSCtxE;
	AVPacket          packet;

	int nWidth  = output_size.width;
	int nHeight = output_size.height;
	int nByte = nWidth * nHeight * 3;
	int br = output_br;

	av_register_all();
	avformat_alloc_output_context2(&pFormatCtxE, NULL, NULL, output_file);
	x = pFormatCtxE->oformat;
	if (avio_open(&pFormatCtxE->pb, output_file, AVIO_FLAG_READ_WRITE) < 0)
	{
		printf("error: cannot OPEN Format Context...\n");
		return false;
	}

	pStreamE = avformat_new_stream(pFormatCtxE, 0);
	if (pStreamE == NULL)
	{
		printf("error : cannot Open Stream...\n");
		return false;
	}

	pCodecCtxE = pStreamE->codec;
	pCodecCtxE->codec_id = pOutFmtE->video_codec;
	pCodecCtxE->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtxE->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtxE->width = nWidth;
	pCodecCtxE->height = nHeight;
	pCodecCtxE->time_base.num = 1;
	pCodecCtxE->time_base.den = 30;
	//CBR
	pCodecCtxE->bit_rate = br;
	pCodecCtxE->rc_min_rate = br;
	pCodecCtxE->rc_max_rate = br;
	pCodecCtxE->bit_rate_tolerance = br;
	pCodecCtxE->rc_buffer_size = br;
	pCodecCtxE->rc_initial_buffer_occupancy = pCodecCtxE->rc_buffer_size * 3 / 4;
	pCodecCtxE->rc_buffer_aggressivity = (float)1.0;
	pCodecCtxE->rc_initial_cplx = 0.5;

	pCodecCtxE->gop_size = 30;
	pCodecCtxE->qmin = 10;
	pCodecCtxE->qmax = 51;
	pCodecCtxE->sample_aspect_ratio.num = 1; // nWidth;
	pCodecCtxE->sample_aspect_ratio.den = 1; // nHeight;

	pSWSCtxE = sws_getContext(nWidth, nHeight, AV_PIX_FMT_BGR24, nWidth, nHeight, AV_PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);
	uint8_t* rgb_buff = new uint8_t[nByte];
	uint8_t* yuv_buff = new uint8_t[nByte / 2];

	pCodecE = avcodec_find_encoder(pCodecCtxE->codec_id);
	if (!pCodecE)
	{
		printf("error 9: cannot FIND Encoder...\n");
		return false;
	}
	if (avcodec_open2(pCodecCtxE, pCodecE, NULL) < 0)
	{
		printf("error : cannot OPEN Encoder");
		return false;
	}

	// Begin Encoding.
	av_new_packet(&packet, nByte);
	avformat_write_header(pFormatCtxE, NULL);
	
	pRGBFrameE = av_frame_alloc();
	pYUVFrameE = av_frame_alloc();
	avpicture_fill((AVPicture*)pRGBFrameE, rgb_buff, AV_PIX_FMT_RGB24, nWidth, nHeight);
	avpicture_fill((AVPicture*)pYUVFrameE, yuv_buff, AV_PIX_FMT_YUV420P, nWidth, nHeight);

	for (int i = 0; i<imgs_names.size(); ++i)
	{	
		printf("Load img: %d: %s...\n", i, imgs_names[i].c_str());
		Mat3b image = imread(imgs_names[i]);
		resize(image, image, output_size);	
		memcpy(rgb_buff, (uint8_t*)image.data, nByte * sizeof(uint8_t));

		pYUVFrameE->width = nWidth;
		pYUVFrameE->height = nHeight;
		sws_scale(pSWSCtxE, pRGBFrameE->data, pRGBFrameE->linesize, 0, nHeight, 
			pYUVFrameE->data, pYUVFrameE->linesize);
		pYUVFrameE->pts = i;

		int got_picture = 0;
		printf("Encoding frame %d ...\n", i);
		int ret = avcodec_encode_video2(pCodecCtxE, &packet, pYUVFrameE, &got_picture);
		if (ret < 0)
		{
			printf("error : failing in encoder...\n");
			return false;
		}
		printf("avcodec_encode_video2 done %d...\n", ret);
		if (got_picture == 1)
		{
			packet.stream_index = pStreamE->index;
			ret = av_write_frame(pFormatCtxE, &packet);
			av_free_packet(&packet);
			av_new_packet(&packet, nByte);
			printf("Writing frame...\n");
		}
	}

	int ret, got_picture;
	while (true)
	{
		ret = avcodec_encode_video2(pCodecCtxE, &packet, NULL, &got_picture);
		if (got_picture == 0)
		{
			av_free_packet(&packet);
			break;
		}
		packet.stream_index = pStreamE->index;
		av_write_frame(pFormatCtxE, &packet);
		av_free_packet(&packet);
		av_new_packet(&packet, nByte);
	}

	av_free_packet(&packet);
	av_write_trailer(pFormatCtxE);
	printf("Finishing encodeing...\n");
	sws_freeContext(pSWSCtxE);
	delete[] rgb_buff;
	delete[] yuv_buff;
	rgb_buff = NULL;
	yuv_buff = NULL;
	av_frame_free(&pRGBFrameE);
	av_frame_free(&pYUVFrameE);

	avcodec_close(pCodecCtxE);
	avformat_close_input(&pFormatCtxE);
	return true;
}

bool convertFormat(const char* input_h264, const char* output_video)
{
	AVOutputFormat *ofmt = NULL;
	AVFormatContext *ifmt_ctx_v = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;

	int ret, i;
	int videoindex_v = -1, videoindex_out = -1;
	int frame_index = 0;

	av_register_all();

	//输入（Input）
	if ((ret = avformat_open_input(&ifmt_ctx_v, input_h264, 0, 0)) < 0) {
		printf("Could not open input file.");
		return false;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx_v, 0)) < 0) {
		printf("Failed to retrieve input stream information");
		return false;
	}

	//输出（Output）
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, output_video);
	if (!ofmt_ctx) {
		printf("Could not create output context\n");
		return false;
	}
	ofmt = ofmt_ctx->oformat;

	for (i = 0; i < ifmt_ctx_v->nb_streams; i++)
	{
		if (ifmt_ctx_v->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			AVStream *in_stream = ifmt_ctx_v->streams[i];
			AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
			videoindex_v = i;
			if (!out_stream) {
				printf("Failed allocating output stream\n");
				return false;
			}
			videoindex_out = out_stream->index;
			//复制AVCodecContext的设置（Copy the settings of AVCodecContext）
			if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
				printf("Failed to copy context from input to output stream codec context\n");
				return false;
			}
			out_stream->codec->codec_tag = 0;
			if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
				out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			break;
		}
	}

	//打开输出文件（Open output file）
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&ofmt_ctx->pb, output_video, AVIO_FLAG_WRITE) < 0) {
			printf("Could not open output file '%s'", output_video);
			return false;
		}
	}
	//写文件头（Write file header）
	if (avformat_write_header(ofmt_ctx, NULL) < 0) {
		printf("Error occurred when opening output file\n");
		return false;
	}

	AVFormatContext *ifmt_ctx;
	int stream_index = 0;
	AVStream *in_stream, *out_stream;
	ifmt_ctx = ifmt_ctx_v;
	stream_index = videoindex_out;

	while (av_read_frame(ifmt_ctx, &pkt)>=0)
	{
		in_stream = ifmt_ctx->streams[pkt.stream_index];
		out_stream = ofmt_ctx->streams[stream_index];

		//Write PTS
		AVRational time_base1 = in_stream->time_base;
		//Duration between 2 frames (us)
		int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
		//Parameters
		pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
		pkt.dts = pkt.pts;
		pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
		frame_index++;

		/* copy packet */
		//转换PTS/DTS（Convert PTS/DTS）
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		pkt.stream_index = stream_index;

		printf("Write 1 Packet. size:%5d\tpts:%8d\n", pkt.size, pkt.pts);
		//写入（Write）
		if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
			printf("Error muxing packet\n");
			break;
		}
		av_free_packet(&pkt);

	}
	//写文件尾（Write file trailer）
	av_write_trailer(ofmt_ctx);
	avformat_close_input(&ifmt_ctx_v);

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

bool Image2Video(const char* input_path, const char *image_fmt, int start_id, int image_num,
	const char* output_path, const char* output_name, Size output_size, int output_br)
{
	vector<string> imgs_names;
	imgs_names.resize(image_num);
	char input_file[512] = { 0 };

	for (int i = 0; i < image_num; ++i)
	{
		sprintf(input_file, "%s\\%d.%s", input_path, i + start_id, image_fmt);
		imgs_names[i] = input_file;
	}

	// 1. image to h264;
	string temp(output_path);
	temp += "\\temp.h264";
	if (!videoEncoding(imgs_names, output_size, output_br, temp.c_str()))
		return false;
	// 2. h264 to mp4;
	string output(output_path);
	output = output + "\\" + output_name;
	if (!convertFormat(temp.c_str(), output.c_str()))
		return false;

	return true;
}


