

 -profile:v high -pix_fmt yuvj420p -level:v 4.1 -preset ultrafast -tune zerolatency -vcodec libx264 -r 10 -b:v 512k -s 640x360 -acodec aac -ac 2 -ab 32k -ar 44100 -f mpegts -flush_packets 0 udp://192.168.1.4:5000?pkt_size=1316
 
 
 ffmpeg -f dshow video="HD Web Camera" -profile:v high -pix_fmt yuvj420p -level:v 4.1 -preset ultrafast -tune zerolatency -vcodec libx264 -r 10 -b:v 512k -s 640x360 -acodec aac -ac 2 -ab 32k -ar 44100 -f mpegts -flush_packets 0 udp://192.168.1.40:5000?pkt_size=1316
 
 ffmpeg -f dshow -list_options true -i video="HD Web Camera"
 
 
ffmpeg -f dshow -s 1280x960 -r 30 -vcodec mjpeg -i video="HD Web Camera" -pix_fmt yuvj420p -preset ultrafast -tune zerolatency -vcodec libx264 -f mpegts -flush_packets 0 udp://192.168.1.40:5000?pkt_size=1316