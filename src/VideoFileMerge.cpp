#include "VideoFileMerge.h"


bool VideoFileMerge(std::vector<std::string> &input_filename_list, std::vector<int> &offsets,
	std::string &output_filename)
{
	// 1. init decoder...
	AVFormatContext  *pFormatCtxD;
	AVOutputFormat   *ofmt;

	// 2. init encoder...
	AVFormatContext  *pFormatCtxE;
	AVStream         *pStreamE;
	AVPacket          packet;

	av_register_all();

	// 3. open video...
	pFormatCtxD = NULL;
	if (avformat_open_input(&pFormatCtxD, input_filename_list[0].c_str(), NULL, NULL) != 0)
	{
		std::cout << "error : cannot OPEN file =" << input_filename_list[0] << std::endl;
		return false;
	}

	if (avformat_find_stream_info(pFormatCtxD, NULL) < 0)
	{
		avformat_close_input(&pFormatCtxD);
		std::cout << "error : cannot READ stream file =" << input_filename_list[0] << std::endl;
		return false;
	}

	// 4. find video/audio stream...
	int videoStreamIndex = -1;
	int audioStreamIndex = -1;
	for (int i = 0; i < pFormatCtxD->nb_streams; i++)
	{
		if (pFormatCtxD->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			videoStreamIndex = i;
		if (pFormatCtxD->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			audioStreamIndex = i;
	}
	if (videoStreamIndex == -1)
	{
		avformat_close_input(&pFormatCtxD);
		std::cout << "error : cannot FIND video or audio stream..." << std::endl;
		return false;
	}

	double frame_rate = av_q2d(pFormatCtxD->streams[videoStreamIndex]->r_frame_rate);
	int64_t frame_total_num = frame_rate * pFormatCtxD->duration / AV_TIME_BASE;
	int frame_num = frame_total_num - offsets[0];

	// 5. set encoder...
	AVStream *in_stream, *out_stream;
	pFormatCtxE = NULL;
	avformat_alloc_output_context2(&pFormatCtxE, NULL, NULL, output_filename.c_str());
	if (!pFormatCtxE)
	{
		std::cout << "Open Error" << std::endl;
		avformat_free_context(pFormatCtxE);
		avformat_close_input(&pFormatCtxD);
		return false;
	}
	ofmt = pFormatCtxE->oformat;

	// copy streams...
	for (int i = 0; i < pFormatCtxD->nb_streams; i++)
	{
		if (pFormatCtxD->streams[i]->codec->codec_type != AVMEDIA_TYPE_VIDEO
			&& pFormatCtxD->streams[i]->codec->codec_type != AVMEDIA_TYPE_AUDIO)
			continue;

		in_stream = pFormatCtxD->streams[i];
		out_stream = avformat_new_stream(pFormatCtxE, in_stream->codec->codec);

		if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0)
		{
			avformat_free_context(pFormatCtxE);
			avformat_close_input(&pFormatCtxD);
			std::cout << "Failed avcodec_copy_context " << std::endl;
			return false;
		}

		if (!out_stream)
		{
			avformat_free_context(pFormatCtxE);
			avformat_close_input(&pFormatCtxD);
			std::cout << "Failed allocating output stream" << std::endl;
			return false;
		}

		out_stream->codec->codec_tag = 0;
		if (pFormatCtxE->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	} //! for 

	if (!(ofmt->flags & AVFMT_NOFILE))
	{
		if (avio_open(&pFormatCtxE->pb, output_filename.c_str(), AVIO_FLAG_READ_WRITE) < 0)
		{
			avformat_free_context(pFormatCtxE);
			avformat_close_input(&pFormatCtxD);
			std::cout << "error: cannot OPEN Format Context..." << std::endl;
			return false;
		}
	}

	// write the video head...
	if (avformat_write_header(pFormatCtxE, NULL) < 0)
	{
		avformat_close_input(&pFormatCtxE);
		avformat_close_input(&pFormatCtxD);
		std::cout << "Write Header Error!" << std::endl;
		return false;
	}

	bool finish_flag = false;
	int video_frame_count = 0;
	int open_video_num = 0;
	int64_t base_pts_audio = 0;
	int64_t base_pts_video = 0;
	int64_t next_pts_audio = 0;
	int64_t next_pts_video = 0;
	while (!finish_flag)
	{
		// decoding...
		if (video_frame_count >= frame_num || av_read_frame(pFormatCtxD, &packet) < 0)
		{
			std::cout << "Add " << video_frame_count << " frames from " << input_filename_list[open_video_num].c_str()
				<< ",idx=" << open_video_num << std::endl;
			avformat_close_input(&pFormatCtxD);
			av_free_packet(&packet);

			// set up new decoding file...
			open_video_num++;
			if (open_video_num < input_filename_list.size())
			{
				if (avformat_open_input(&pFormatCtxD, input_filename_list[open_video_num].c_str(), NULL, NULL) != 0)
				{
					av_write_trailer(pFormatCtxE);
					avformat_close_input(&pFormatCtxE);
					std::cout << "error : cannot OPEN file=" << input_filename_list[open_video_num] << std::endl;
					break;
				}

				if (avformat_find_stream_info(pFormatCtxD, NULL) < 0)
				{
					av_write_trailer(pFormatCtxE);
					avformat_close_input(&pFormatCtxE);
					avformat_close_input(&pFormatCtxD);
					std::cout << "error : cannot READ stream..." << std::endl;
					break;
				}

				int temp_videoStreamIndex = -1;
				int temp_audioStreamIndex = -1;
				for (int i = 0; i < pFormatCtxD->nb_streams; i++)
				{
					if (pFormatCtxD->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
						temp_videoStreamIndex = i;
					if (pFormatCtxD->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
						temp_audioStreamIndex = i;
				}
				if (temp_videoStreamIndex == -1)
				{
					av_write_trailer(pFormatCtxE);
					avformat_close_input(&pFormatCtxE);
					avformat_close_input(&pFormatCtxD);
					std::cout << "error : cannot FIND video or audio stream..." << std::endl;
					break;
				}

				frame_rate = av_q2d(pFormatCtxD->streams[videoStreamIndex]->r_frame_rate);
				frame_total_num = frame_rate * pFormatCtxD->duration / AV_TIME_BASE;
				frame_num = frame_total_num - offsets[open_video_num];

				video_frame_count = 0;
				base_pts_audio = next_pts_audio;
				base_pts_video = next_pts_video;
				continue;
			} // ! if (open_video_num < input_filename_list.size())
			else
			{
				av_write_trailer(pFormatCtxE);
				avformat_close_input(&pFormatCtxE);
				finish_flag = true;
				std::cout << "Finish Video Merging:" << open_video_num << std::endl;
			}
		} // ! if(av_read_frame(pFormatCtxD, &packet) < 0)

		if (finish_flag)
		{
			av_free_packet(&packet);
			continue;
		}

		in_stream = pFormatCtxD->streams[packet.stream_index];
		out_stream = pFormatCtxE->streams[packet.stream_index];

		packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);

		if (packet.stream_index == videoStreamIndex)
		{
			packet.pts += base_pts_video;
			packet.dts += base_pts_video;
			video_frame_count++;
			next_pts_video = packet.pts + packet.duration;
		}
		else if (packet.stream_index == audioStreamIndex)
		{
			packet.pts += base_pts_audio;
			packet.dts += base_pts_audio;
			next_pts_audio = packet.pts + packet.duration;
		}
		else
		{
			av_free_packet(&packet);
			continue;
		}

		// write packet...
		av_interleaved_write_frame(pFormatCtxE, &packet);
		av_free_packet(&packet);
	}

	if (!finish_flag)
	{
		return false;
	}

	return true;
}
