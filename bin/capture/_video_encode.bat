ffmpeg -r 30 -f image2 -start_number 18 -i ./frame%%d.tga -vcodec libx264 -f mp4 -pix_fmt yuv420p _videos/xxx.mp4

REM -b:v 2.0M ...video bitrate
REM -i vis2020_fitting_session.mp3
REM -c copy
REM -i "concat:vis2020_title.ts|vis2020_body.ts"
REM -filter:v "crop=out_w:out_h:x:y"
REM -t 20 (encode X seconds of video)

REM docu: https://ffmpeg.org/ffmpeg.html
REM combining two mp4s: https://stackoverflow.com/questions/7567713/combining-two-video-from-ffmpeg
REM add audio: https://superuser.com/questions/590201/add-audio-to-video-using-ffmpeg