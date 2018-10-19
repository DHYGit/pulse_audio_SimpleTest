#include "libpcm_aac.h"
#include "Raspberry_Pi_Record.h"
#include "spdlog/spdlog.h"

namespace spd = spdlog;

std::string get_selfpath() {
    char buff[1024];
    ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
    if (len != -1) {
        std::string str(buff);
        return str.substr(0, str.rfind('/') + 1);
    }
}

std::string selfpath = get_selfpath() + "/log/stitch.log";
auto logger = spd::daily_logger_mt("daily_logger", selfpath.c_str(), 0, 0);
pthread_mutex_t log_lock;

void LOG(bool flag, std::string str){
    pthread_mutex_lock(&log_lock);
    if(flag){
        logger->info(str);
    }else{
        logger->error(str + " error");
    }
    pthread_mutex_unlock(&log_lock);
}

Alsa2PCM pcm_obj;
int Alsa2PCMCallback(unsigned char*buff, int len, void*args);
int ilen = 0;
int pcm_length = 1024;
unsigned char pcm_data[1024];
int main(){
    std::string function = __FUNCTION__;
    pthread_mutex_init(&log_lock, NULL);
    spd::set_level(spd::level::trace);
    spd::set_pattern("[%l][%H:%M:%S:%f] %v");
    logger->flush_on(spd::level::trace);
    LOG(true, "Progress start");

    Stream_Record_Info s_info;
    s_info.Channel = 1;
    s_info.Frames = 160;
    s_info.Rate = 16000;
    s_info.pcm_type=PCM_TYPE_PULSEaUDIO;
    s_info.Format  = SND_PCM_FORMAT_S16_LE;
    int ret = pcm_obj.Init(s_info,Alsa2PCMCallback,NULL);
    if(ret == 0){
        printf("pcm_obj init success \n");
        LOG(true, function + " pcm_obj init success");
    }else{
        printf("pcm_obj init failed \n");
        LOG(false, function + " pcm_obj init failed");
        return ret;
    }
    
    pcm_obj.Process();
    return 0;
}

int Alsa2PCMCallback(unsigned char*buff, int len, void*args){
    std::string function = __FUNCTION__;
    int ret = PulseAudioPlay(pcm_obj.g_pulse_handle, buff, len);
    if(ret != 0){
        LOG(false, function + " play audio failed");
        printf("%s play audio failed+++++++++++++++++++++++++++\n", __FUNCTION__);
    }else{
        printf("%s play audio success len %d+++++++++++++++++++++++++++\n", __FUNCTION__, len);
    }
    /*if(ilen == 0){
          memset(pcm_data, 0, pcm_length);
    }
    if((ilen + len) < pcm_length || (ilen + len) >= pcm_length){
        memcpy(pcm_data + ilen, buff, len);
        ilen += len;
    }
    if(ilen >= pcm_length){
        ilen = 0;
        //play audio
        int ret = PulseAudioPlay(pcm_obj.g_pulse_handle, pcm_data, pcm_length);
        if(ret != 0){
            LOG(false, function + " play audio failed");
            printf("%s play audio failed+++++++++++++++++++++++++++\n", __FUNCTION__);
        }else{
            printf("%s play audio success len %d+++++++++++++++++++++++++++\n", __FUNCTION__, pcm_length);
        }
    }*/
    return 0;
}
