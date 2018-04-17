#include "VideoFileCut.h"

#ifdef __cplusplus
extern "C" {
#endif
    
#include "libavformat/avformat.h"
    
#ifdef __cplusplus
}
#endif

using namespace std;


bool VideoFileCut(std::string& input_filename, std::string& output_filename,
	int start_frame, int end_frame, int *offset)
{
	if (end_frame <= start_frame)
		return false;

	// 1. init decoder...
	AVFormatContext  *pFormatCtxD;
	AVPacket          packet;
	AVOutputFormat   *ofmt;

	// 2. init encoder...
	AVFormatContext  *pFormatCtxE;
//    AVStream         *pStreamE;

	av_register_all();

	// 3. open video...
	pFormatCtxD = NULL;
	if (avformat_open_input(&pFormatCtxD, input_filename.c_str(), NULL, NULL) != 0)
	{
		printf("error : cannot OPEN file %s...\n", input_filename.c_str());
		return false;
	}

	if (avformat_find_stream_info(pFormatCtxD, NULL) < 0)
	{
		printf("error : cannot READ stream...\n");
		avformat_close_input(&pFormatCtxD);
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
	//if (videoStreamIndex == -1 || audioStreamIndex == -1)
	if (videoStreamIndex == -1)
	{
		printf("error : cannot FIND video or audio stream...\n");
		avformat_close_input(&pFormatCtxD);
		return false;
	}

	AVRational video_frame_rate = pFormatCtxD->streams[videoStreamIndex]->r_frame_rate;
	AVRational video_time_base = pFormatCtxD->streams[videoStreamIndex]->time_base;
	int64_t video_first_dts = pFormatCtxD->streams[videoStreamIndex]->first_dts;
//    double frame_rate = av_q2d(video_frame_rate);

	// 5. resaving...
	AVStream *in_stream, *out_stream;

	// 6. setup new encoder...
	pFormatCtxE = NULL;
	avformat_alloc_output_context2(&pFormatCtxE, NULL, NULL, output_filename.c_str());
	if (!pFormatCtxE)
	{
		printf("Open Error\n");
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
			printf("Failed avcodec_copy_context \n");
			avformat_close_input(&pFormatCtxD);
			avformat_free_context(pFormatCtxE);
			return false;
		}

		if (!out_stream)
		{
			printf("Failed allocating output stream\n");
			avformat_close_input(&pFormatCtxD);
			avformat_free_context(pFormatCtxE);
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
			printf("error: cannot OPEN Format Context...\n");
			avformat_close_input(&pFormatCtxD);
			avformat_free_context(pFormatCtxE);
			return false;
		}
	}

	// find nearest key frame...
//    int pre = 0;
	int64_t seek_nearest_key_before = -1;
	int64_t nearest_key_end = -1;
	bool find_key_flag = true;
	int frame_id = start_frame + 1;
	while (seek_nearest_key_before == -1 || frame_id <= start_frame)
	{
		if (frame_id > start_frame)
		{
			if (find_key_flag && seek_nearest_key_before == -1)
			{
				//av_seek_frame(pFormatCtxD, -1, start_frame / frame_rate * 1000000, AVSEEK_FLAG_BACKWARD);

				int64_t target_dts_usecs = (int64_t)round(start_frame * (double)video_frame_rate.den / video_frame_rate.num * AV_TIME_BASE);
				// Remove first dts: when non zero seek should be more accurate
				auto first_dts_usecs = (int64_t)round(video_first_dts * (double)video_time_base.num / video_time_base.den * AV_TIME_BASE);
				target_dts_usecs += first_dts_usecs;
				int rv = av_seek_frame(pFormatCtxD, -1, target_dts_usecs, AVSEEK_FLAG_BACKWARD);

				find_key_flag = false;
			}
		}

		if (av_read_frame(pFormatCtxD, &packet) < 0)
		{
//            av_free_packet(&packet);
            av_packet_unref(&packet);
			avformat_close_input(&pFormatCtxE);
			break;
		}

		if (packet.stream_index == videoStreamIndex)
		{
			frame_id = (int)packet.pts / packet.duration;//current video frame number?

			if (!find_key_flag)
			{
				if (frame_id > start_frame)
				{
//                    av_free_packet(&packet);
                    av_packet_unref(&packet);
					continue;
				}
				find_key_flag = true;
			}

			if (packet.flags & AV_PKT_FLAG_KEY)
			{
				if (frame_id > start_frame)
				{
//                    av_free_packet(&packet);
                    av_packet_unref(&packet);
					continue;
				}
				seek_nearest_key_before = packet.pts;
				*offset = start_frame - frame_id;
				nearest_key_end = packet.duration * end_frame;
//                av_free_packet(&packet);
                av_packet_unref(&packet);
				break;
			}
		}
//        av_free_packet(&packet);
        av_packet_unref(&packet);
	}

	if (seek_nearest_key_before == -1)
	{
		avformat_close_input(&pFormatCtxD);
		return false;
	}

	// video cut...
	//av_seek_frame(pFormatCtxD, -1, start_frame / frame_rate * 1000000, AVSEEK_FLAG_BACKWARD);
	int64_t target_dts_usecs = (int64_t)round(start_frame * (double)video_frame_rate.den / video_frame_rate.num * AV_TIME_BASE);
	auto first_dts_usecs = (int64_t)round(video_first_dts * (double)video_time_base.num / video_time_base.den * AV_TIME_BASE);
	target_dts_usecs += first_dts_usecs;
	int rv = av_seek_frame(pFormatCtxD, -1, target_dts_usecs, AVSEEK_FLAG_BACKWARD);


	bool finish_cut = false;
	bool cut_begin = false;
	bool first_audio = true;
	bool first_video = true;
	int64_t audio_base_pts = 0;
	int64_t video_base_pts = 0;
	//local client target cut count is "end - start",but server target cut count is "end - start + 1".
	int target_count = end_frame - start_frame + (*offset);
	int curr_count = 0;
	bool cut_succ = false;

	// write the video head...
	if (avformat_write_header(pFormatCtxE, NULL) < 0)
	{
		printf("Write Header Error!\n");
		avformat_close_input(&pFormatCtxD);
		avformat_close_input(&pFormatCtxE);
		return false;
	}
	while (!finish_cut)
	{
		if (av_read_frame(pFormatCtxD, &packet) < 0)
		{
//            av_free_packet(&packet);
            av_packet_unref(&packet);
			break;
		}

		if (packet.stream_index != videoStreamIndex && packet.stream_index != audioStreamIndex)
		{
//            av_free_packet(&packet);
            av_packet_unref(&packet);
			continue;
		}

		if (!cut_begin)
		{
			if (packet.stream_index == videoStreamIndex && packet.pts == seek_nearest_key_before)
			{
				cut_begin = true;
			}
			else
			{
//                av_free_packet(&packet);
                av_packet_unref(&packet);
				continue;
			}
		}

		if (packet.pts >= nearest_key_end)
		{
			cut_succ = true;
			if (curr_count >= target_count)
			{
				finish_cut = true;
//                av_free_packet(&packet);
                av_packet_unref(&packet);
				continue;
			}
		}

		in_stream = pFormatCtxD->streams[packet.stream_index];
		out_stream = pFormatCtxE->streams[packet.stream_index];
		packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);

		if (packet.stream_index == videoStreamIndex)
		{
			if (first_video)
			{
				first_video = false;
				video_base_pts = packet.pts;
			}
			packet.pts -= video_base_pts;
			packet.dts -= video_base_pts;
			curr_count++;
//            (*out_duration)++;
		}
		else if (packet.stream_index == audioStreamIndex)
		{
			if (first_audio)
			{
				first_audio = false;
				audio_base_pts = packet.pts;
			}
			packet.pts -= audio_base_pts;
			packet.dts -= audio_base_pts;
		}
		else
		{
//            av_free_packet(&packet);
            av_packet_unref(&packet);
			continue;
		}
		av_interleaved_write_frame(pFormatCtxE, &packet);
//        av_free_packet(&packet);
        av_packet_unref(&packet);
	}

	av_write_trailer(pFormatCtxE);
	avformat_close_input(&pFormatCtxE);
	avformat_close_input(&pFormatCtxD);

	if (!cut_succ)
		return false;
	else
		return true;
}