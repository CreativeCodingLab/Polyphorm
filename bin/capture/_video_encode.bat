ffmpeg -r 30 -f image2 -start_number 0 -i ./frame%%d.tga -vcodec libx264 -f mp4 -pix_fmt yuv420p _videos/XXX.mp4

REM -b:v 2.0M ...video bitrate