# Audio-Video-processing-APIs
Some audio and video processing interface
Base on FFmpeg
Please use FFmpeg 3.0 or later

1.VideoRemuxer

	Description:
		Add the specified audio to the video.
	files:
		src/VideoRemuxer.h
		src/VideoRemuxer.cpp

2.VideoFileMerge

	Description:
		Synthesize multiple video files into one video file.
	files:
		src/VideoFileMerge.h
		src/VideoFileMerge.cpp

3.VideoFileCut

	Description:
		Clips from the original video file within a piece of content to generate a new file.
	files:
		src/VideoFileCut.h
		src/VideoFileCut.cpp
4.VideoEncryption

	Description:
		Encrypt and decrypt video files.
	files:
		src/VideoEncryptioner.h
		src/VideoEncryptioner.cpp
...
