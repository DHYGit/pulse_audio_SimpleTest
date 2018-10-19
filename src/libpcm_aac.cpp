#include "libpcm_aac.h"
#include "Raspberry_Pi_Record.h"
#include <string.h>
#include <string>

extern void LOG(bool flag, std::string str);

Alsa2PCM::Alsa2PCM()
:pStream_Record_Info(NULL)
,record_handle(NULL)
,Rec_Buff(NULL)
,Rec_Buff_Size(0)
,m_pulse_handle(NULL)
{

}


Alsa2PCM::~Alsa2PCM()
{

}

int Alsa2PCM::Init(Stream_Record_Info stream_info,OnMessages proc,void*args)
{
    std::string function = __FUNCTION__;
	m_pcm_type = stream_info.pcm_type;
	
	if(stream_info.pcm_type == PCM_TYPE_ALSA)
	{
		if(!pStream_Record_Info)
		{
			pStream_Record_Info = (struct Stream_Record_Info *)malloc(sizeof(struct Stream_Record_Info));
            if(pStream_Record_Info == NULL){
                LOG(false, function + " pStream_Record_Info malloc failed");
                return -1;
            }
            memset(pStream_Record_Info, 0, sizeof(Stream_Record_Info));
			memcpy(pStream_Record_Info,&stream_info,sizeof(Stream_Record_Info));
			printf("channelid(%d) frame(%d) rate(%d) format(%d)\n",pStream_Record_Info->Channel,pStream_Record_Info->Frames,pStream_Record_Info->Rate,pStream_Record_Info->Format);
		}
		//std::string dev_name = "plughw:0,0";
		//record_handle=Raspberry_Pi_Record_Init((char*)dev_name.c_str(),pStream_Record_Info);
		record_handle=Raspberry_Pi_Record_Init("hw:1,0",pStream_Record_Info);
		if(record_handle<0)
		{
			printf("Raspberry Pi Record Init Error!\n");
            LOG(false, function + " Raspberry Pi Record Init Error");
			return -1;
		}
		Rec_Buff_Size=pStream_Record_Info->Channel*pStream_Record_Info->Frames*2;
		Rec_Buff=(unsigned char *)malloc(Rec_Buff_Size);
        if(Rec_Buff == NULL){
            LOG(false, function + " Rec_Buff malloc failed");
            return -1;
        }
		memset(Rec_Buff,0,Rec_Buff_Size);
	}else if(stream_info.pcm_type == PCM_TYPE_PULSEaUDIO){

        static const pa_sample_spec ss = {
            .format = PA_SAMPLE_S16LE,
            .rate = 16000,
            .channels = stream_info.Channel
        };
        int error = 0;
        if (!(m_pulse_handle= pa_simple_new(NULL, "PulseAudio", PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error)))
        {
            fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
            LOG(false, function + " pa_simple_new excute failed");
            return -1;
        }
        Rec_Buff_Size	= 1024;
        Rec_Buff		=(unsigned char *)malloc(Rec_Buff_Size);
        if(Rec_Buff == NULL){
            LOG(false, function + " Rec_Buff malloc failed");
            return -1;
        }
        memset(Rec_Buff,0,Rec_Buff_Size);
        
        int res = PulseAudioInit(&g_pulse_handle, ss);
        if(res){
            LOG(false, function + " pulse audio player Init failed");
            return -1;
        }
        LOG(true, function + " pulse audio player Init success");
    }else{
        return -1;
	}
	
    m_on_proc = proc;
    m_video_audio_proc = args;

	return 0;
}



int Alsa2PCM::Process()
{
    std::string function = __FUNCTION__;
	int rc = 0;
	printf("In Alsa2PCM Process \n");
    LOG(true, "In " + function);
	//while (pStream_Record_Info) 
    while(1)
	{ 
		if(m_pcm_type == PCM_TYPE_ALSA)
		{
			rc = snd_pcm_readi(record_handle, Rec_Buff, pStream_Record_Info->Frames); 
		
			if (rc == -EPIPE) /* EPIPE means overrun */ 
			{  	 
				fprintf(stderr, "overrun occurred\n");  
                LOG(false, function + " snd_pcm_readi overrun occurred");
				snd_pcm_prepare(record_handle);  
			} 
			else if (rc < 0) 
			{ 
				fprintf(stderr,"error from read: %s\n",snd_strerror(rc));
                LOG(false, function + " snd_pcm_readi read failed");
			} 
			else if (rc != (int)pStream_Record_Info->Frames)
			{  
				fprintf(stderr, "short read, read %d frames\n", rc);
                LOG(false, function + " snd_pcm_readi short read");
			}
			else
			{  	
				if(Rec_Buff && Rec_Buff_Size>0)
					m_on_proc(Rec_Buff,Rec_Buff_Size,m_video_audio_proc);
			}
		}else if(m_pcm_type == PCM_TYPE_PULSEaUDIO){
			int error = 0;
			if (pa_simple_read(m_pulse_handle, Rec_Buff, Rec_Buff_Size, &error) < 0)
			{
				fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
                LOG(false, function + " pa_simple_read failed");
                printf("%s:pa_simple_read() failed \n", __FUNCTION__);
			}
			if(Rec_Buff && Rec_Buff_Size>0){
                //play
				m_on_proc(Rec_Buff,Rec_Buff_Size,m_video_audio_proc);
            }else{
				printf("Rec_Buff:%X, Rec_Buff_Size is %d \n", Rec_Buff, Rec_Buff_Size);
                LOG(false, function + " Rec_Buff and Rec_Buff_Size failed");
            }
		}else{

		}
	}
}

int Alsa2PCM::UnInit(void)
{
	if(pStream_Record_Info)
	{
		free(pStream_Record_Info);
		pStream_Record_Info = NULL;
	}

	if(Rec_Buff)
	{
		free(Rec_Buff);
		Rec_Buff = NULL;
	}
	
}

Pcm2AAC::Pcm2AAC()
    :nSampleRate(16000)
    ,nChannels(1)
    ,nBit(16)
    ,nInputSamples(0)
    ,nMaxInputBytes(0)
    ,nMaxOutputBytes(0)
    ,hEncoder(NULL)
    ,pConfiguration(NULL)
    ,pbPCMBuffer(NULL)
    ,pbAACBuffer(NULL)
     ,easy_handle(NULL)
{

}

Pcm2AAC::~Pcm2AAC()
{

}

int Pcm2AAC::Init(OnMessages on_proc,void* args){
    hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples, &nMaxOutputBytes);
    printf("In %s nInputSamples %d nMaxOutputBytes %d +++++++++++++++++++++++++++\n", __FUNCTION__, nInputSamples, nMaxOutputBytes);
    nMaxInputBytes=nInputSamples*nBit/8;
    if(hEncoder == NULL)
    {
        printf("[ERROR] Failed to call faacEncOpen()\n");
        return -1;
    }
    pbPCMBuffer = new unsigned char[nMaxInputBytes];
    pbAACBuffer = new unsigned char[nMaxOutputBytes];
#if defined(FAAC_ENCODER_)
    pConfiguration = faacEncGetCurrentConfiguration(hEncoder);//ï¿½ï¿½È¡ï¿½ï¿½ï¿½Ã½á¹¹Ö¸ï¿½ï¿½
    pConfiguration->inputFormat = FAAC_INPUT_16BIT;
    pConfiguration->outputFormat= 1;
    pConfiguration->useTns=true;
    pConfiguration->useLfe=false;
    pConfiguration->aacObjectType=LOW;
    pConfiguration->mpegVersion = MPEG4;
    //pConfiguration->shortctl=SHORTCTL_NORMAL;
    pConfiguration->quantqual=50;
    pConfiguration->bandWidth=0;
    pConfiguration->bitRate=0;

    // (2.2) Set encoding configuration
    if(!faacEncSetConfiguration(hEncoder, pConfiguration))//ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ã£ï¿½ï¿½ï¿½ï¿½Ý²ï¿½Í¬ï¿½ï¿½ï¿½Ã£ï¿½ï¿½ï¿½Ê±ï¿½ï¿½Ò»ï¿½ï¿½
    {
        printf("faac encoder set configuration failed \n");
        return -2;
    }
#elif defined(EASY_AAC_ENCODER_)

    //easyaacencoder ini
    InitParam initParam;
    initParam.u32AudioSamplerate=16000;
    initParam.ucAudioChannel=1;
    initParam.u32PCMBitSize=16;
    initParam.ucAudioCodec = Law_PCM16;
    //initParam.g726param.ucRateBits=Rate16kBits;include

    easy_handle= Easy_AACEncoder_Init(initParam);
#endif
    m_mess_proc = on_proc;
    m_video_audio_proc = args;
    return 0;
}

int Pcm2AAC::Process(char* buff, int len){
    if(!buff || len<=0)
    {
        printf("----(%s)--(%s)----(%d)- Pcm2AAC::Process(%d)!--\n",__FILE__,__FUNCTION__,__LINE__,len);
        return -1;
    }

    int nRet = 0;
#if defined(FAAC_ENCODER_)
    nInputSamples = len/ (nBit / 8);

    nRet = faacEncEncode(hEncoder, (int*) buff, nInputSamples, pbAACBuffer, nMaxOutputBytes);
    if (nRet<1)
    {
        printf("-----faacEncEncode failed!\n");
        return -1;
    }
    m_mess_proc(pbAACBuffer,nRet,m_video_audio_proc);
#elif defined(EASY_AAC_ENCODER_)
    unsigned int out_len=0;
    nRet = Easy_AACEncoder_Encode(easy_handle, (unsigned char*)buff, len, pbAACBuffer, &out_len);
    if(nRet>0)
    {
        m_mess_proc(pbAACBuffer,out_len,m_video_audio_proc);endif
    }
#endif
    return 0;
}

int Pcm2AAC::UnInit()
{
#if defined(EASY_AAC_ENCODER_)
    if(easy_handle){
        Easy_AACEncoder_Release(easy_handle);
        easy_handle = NULL;
    }
#endif
    if(hEncoder){
        faacEncClose(hEncoder); 
        hEncoder = NULL;
    }
    if(pbPCMBuffer){
        delete[] pbPCMBuffer; 
        pbPCMBuffer = NULL;
    }
    if(pbAACBuffer){
        delete[] pbAACBuffer;
        pbAACBuffer = NULL;
    }
}

int Pcm2AAC::GetFaacEncDecoderSpecificInfo(unsigned char ** spec_info,unsigned long * len)
{
    faacEncGetDecoderSpecificInfo(hEncoder,spec_info,len);
    return 0;
}

