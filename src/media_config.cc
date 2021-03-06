/*
Copyright (c) 2017, rpi-webrtc-streamer Lyu,KeunChang

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <memory>
#include <string>
#include "webrtc/base/checks.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/arraysize.h"

#include "media_config.h"
#include "utils.h"

namespace media_config {

///////////////////////////////////////////////////////////////////////////////////////////
//
// config key name and media config constants values
//
///////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Config Key
////////////////////////////////////////////////////////////////////////////////
// video
static const char kConfigMaxBitrate[] = "max_bitrate";
static const char kConfigResolution4_3[] = "use_4_3_video_resolution";
static const char kConfigVideoResolution[] = "initial_video_resolution";
static const char kConfigVideoFrameRate[] = "initial_video_framerate";
static const char kConfigVideoInitialResolution[] = "use_initial_video_resolution";
static const char kConfigVideoDynamicResolution[] = "use_dynamic_video_resolution";

static const char kConfigVideoResolutionList43[] = "video_resolution_list_4_3";
static const char kConfigVideoResolutionList169[] = "video_resolution_list_16_9";

static const char kConfigVideoRotation[] = "video_rotation";
static const char kConfigVideoVFlip[] = "video_vflip";
static const char kConfigVideoHFlip[] = "video_hflip";

// audio
static const char kConfigAudioProcessing[] = "audio_processing_enable";
static const char kConfigAudioEchoCancel[] = "audio_echo_cancellation";
static const char kConfigAudioGainControl[] = "audio_gain_control";
static const char kConfigAudioHighPassFilter[] = "audio_high_passfilter";
static const char kConfigAudioNoiseSuppression[] = "audio_noise_suppression";
static const char kConfigAudioLevelControl[] = "audio_level_control_enable";

static const char kConfigVideoResolutionDelimiter=',';


////////////////////////////////////////////////////////////////////////////////
// Default Values
////////////////////////////////////////////////////////////////////////////////
static const int  kDefaultVideoMaxFrameRate=30;
static const int  kDefaultMaxBitrate = 3500000;
static const int  kDefaultVideoRotation = 0;
static const bool  kDefaultVideoVFlip = false;
static const bool  kDefaultVideoHFlip = false;

static const char kDefaultVideoResolution43[] =  
    "320x240,400x300,512x384,640x480,1024x768,1152x864,1296x972,1640x1232";
static const char kDefaultVideoResolution169[] = 
    "384x216,512x288,640x360,768x432,896x504,1024x576,1152x648,1280x720,1408x864,1920x1080";

#define CONFIG_LOAD_BOOL(key,value) \
        { \
            std::string flag_value; \
            if( config_.GetStringValue(key, &flag_value ) == true){ \
                if(flag_value.compare("true") == 0) value = true; \
                else if (flag_value.compare("false") == 0) value = false; \
                else LOG(INFO) << "Default Config \"" <<  key \
                        << "\" value is not valid" << flag_value; \
            }; \
        };

#define CONFIG_LOAD_BOOL_WITH_DEFAULT(key,value,default_value) \
        { \
            std::string flag_value; \
            if( config_.GetStringValue(key, &flag_value ) == true){ \
                if(flag_value.compare("true") == 0) value = true; \
                else if (flag_value.compare("false") == 0) value = false; \
                else { \
                    LOG(INFO) << "Default Config \"" <<  key \
                        << "\" value is not valid" << flag_value; \
                    value = default_value; \
                };  \
            }; \
        };

#define CONFIG_LOAD_INT_WITH_DEFAULT(key,value,validate_function,default_value) \
        { \
            if( config_.GetIntValue(key, &value ) == true){ \
                validate_function(value, default_value); \
            } \
            else value = default_value; \
        };

///////////////////////////////////////////////////////////////////////////////////////////
// Config Value
///////////////////////////////////////////////////////////////////////////////////////////

// video
int max_bitrate = kDefaultMaxBitrate;
bool resolution_4_3_enable = true;
int video_rotation = 0;
bool video_vflip = false;
bool video_hflip = false;

struct ResolutionConfig initial_video_resolution(640,480);
int default_video_framerate = kDefaultVideoMaxFrameRate;

bool use_initial_video_resolution = false;
bool use_dynamic_video_resolution = true;

std::list <ResolutionConfig> resolution_list_4_3;
std::list <ResolutionConfig> resolution_list_16_9;

// audio related config
// this feature will require high CPU usage 
bool audio_processing_enable = false;
bool audio_echo_cancel = true;
bool audio_gain_control = true;
bool audio_highpass_filter = true;
bool audio_noise_suppression = true;
bool audio_level_control = true;


///////////////////////////////////////////////////////////////////////////////////////////
//
// config loading and helper functions
//
///////////////////////////////////////////////////////////////////////////////////////////
void validate__video_rotation(int &video_rotation, int default_value ) {
    if( !((video_rotation == 0) || (video_rotation == 90) ||
                (video_rotation == 180) || (video_rotation == 270)) ) {
        LOG(LS_ERROR) << "Error in video roration value: " 
            << video_rotation << " is not a valid video rotation value";
        LOG(LS_ERROR) << "Resetting to default value : "
            << default_value;
        video_rotation = default_value;
    }
}

void validate__video_maxbitrate(int &video_maxbitrate, int default_value ) {
    // 17000000 values from RaspiVid.c bitrate for 1080p
    if ((video_maxbitrate < 200) || (video_maxbitrate > 17000000)) {
        LOG(LS_ERROR) << "Error in video max bitrate value: " 
            << video_rotation << " is not a valid video max bitrate value";
        LOG(LS_ERROR) << "Resetting to default value : "
            << default_value;
        video_maxbitrate = default_value;
    }
}

//
bool parse_vidio_resolution(const std::string resolution_list, 
        std::list <ResolutionConfig> &resolution ) {
    std::stringstream ss(resolution_list);
    std::string token;
    int count=0;

    while( getline(ss, token, kConfigVideoResolutionDelimiter) ) {
        int width, height;
        if( utils::ParseVideoResolution(token, &width, &height ) == true )  {
            count++;
            resolution.push_back(ResolutionConfig(width,height));
        }
        else {
            LOG(LS_ERROR) << "Failed to add resolution : " << token;
        }
    }
    return (count?true:false);
}

// Returns true if the resolution exists in the resolution_list, 
// or false otherwise.
bool validate_resolution(int width, int height) {
    if( resolution_4_3_enable ) {
        for(std::list<ResolutionConfig>::iterator iter = 
                resolution_list_4_3.begin(); 
            iter != resolution_list_4_3.end(); iter++) {
            if( iter->width_ == width && iter->height_ == height)
                return true;
        }
    }
    else {
        for(std::list<ResolutionConfig>::iterator iter = 
                resolution_list_16_9.begin(); 
            iter != resolution_list_16_9.end(); iter++) {
            if( iter->width_ == width && iter->height_ == height)
                return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
// mainx config loading function
//
///////////////////////////////////////////////////////////////////////////////////////////
bool config_load(const std::string config_filename) {
    rtc::OptionsFile config_(config_filename);

    if( config_.Load() == false ) {
        return false;
    };

    // loading max_bitrate
    CONFIG_LOAD_INT_WITH_DEFAULT(kConfigMaxBitrate, max_bitrate,  
            validate__video_maxbitrate, kDefaultMaxBitrate );

    // loading video rotation config
    CONFIG_LOAD_INT_WITH_DEFAULT(kConfigVideoRotation, video_rotation,  
            validate__video_rotation, kDefaultVideoRotation );

    // loading vflip & hflip
    CONFIG_LOAD_BOOL_WITH_DEFAULT(kConfigVideoVFlip,
            video_vflip,false);
    CONFIG_LOAD_BOOL_WITH_DEFAULT(kConfigVideoHFlip,
            video_hflip,false);

    // loading 4:3 or 16:9 resolution config
    CONFIG_LOAD_BOOL_WITH_DEFAULT(kConfigResolution4_3,
            resolution_4_3_enable,true);

    // loading dynamic video resolution config
    //
    // The use_dynamic_video_resolution config value is used 
    // in QualityConfig :: GetBestMatch. When enabled, the resolution is changed 
    // to a resolution similar to the average bitrate value of the resolution.
    //
    // If it is disabled, it keeps the initial resolution set in InitEncoder. 
    // If you want to disable it, you must enable use_initial_video_resoltuion 
    // and set the desired resolution to initial_video_resolution.
    CONFIG_LOAD_BOOL_WITH_DEFAULT(kConfigVideoDynamicResolution,
            use_dynamic_video_resolution, true);

    // loading default video resolution config
    std::string resolution_list;
    config_.GetStringValue(kConfigVideoResolutionList43, &resolution_list );
    if( parse_vidio_resolution( resolution_list, resolution_list_4_3 ) == false ) {
        // Loading default video resolution list
        parse_vidio_resolution( kDefaultVideoResolution43, resolution_list_4_3 );
    };
    config_.GetStringValue(kConfigVideoResolutionList169, &resolution_list );
    if( parse_vidio_resolution( resolution_list, resolution_list_16_9 ) == false ) {
        // Loading default video resolution list
        parse_vidio_resolution( kDefaultVideoResolution169, resolution_list_16_9 );
    };

    // loading flag for default video resolution 
    std::string flag_use_initial_resolution;
    if( config_.GetStringValue(kConfigVideoInitialResolution, 
                &flag_use_initial_resolution ) == true ) {
        if( flag_use_initial_resolution.compare("true") == 0 )  {

            // loading default video resolution config
            std::string resolution_config;
            if( config_.GetStringValue(kConfigVideoResolution, 
                        &resolution_config ) == true ){

                // Getting default framerate config
                config_.GetIntValue(kConfigVideoFrameRate, 
                        &default_video_framerate);
                if( default_video_framerate == 0 ) default_video_framerate = 30;

                // need video width and height config 
                // to enable use_initial_video_resolution
                int width, height;
                if( utils::ParseVideoResolution( resolution_config, &width, &height ) == true ) {
                    if(validate_resolution(width, height) == true ) {
                        initial_video_resolution.width_ = width;
                        initial_video_resolution.height_ = height;
                        use_initial_video_resolution = true;
                    }
                    else {
                        LOG(LS_ERROR) << "Default resolution \"" 
                            <<  width << "x" << height << "\" is not valid";
                    }
                }
            }
            else {
                LOG(LS_ERROR) << "Initial Video Resolution config is not found.";
            } 
        }
        else if( flag_use_initial_resolution.compare("false") == 0 ) 
            use_initial_video_resolution = false;
        else {
            LOG(LS_ERROR) << "Initial Resolution \"" << kConfigVideoInitialResolution
                << "\" value is not valid" << flag_use_initial_resolution;
            // default value for use_initial_video_resolution is false
            use_initial_video_resolution = false;
        }
    };

    // validate the initial_video_resolution and dynamic_video_resolution
    // one of setting should be enabled.
    if( use_dynamic_video_resolution == false && 
            use_initial_video_resolution == false ) {
        LOG(LS_ERROR) << "Both of dynamic video resolution and initial resolution disabled";
        LOG(LS_ERROR) << "Overriding configuration setting to initial video resolution to enable";
        use_initial_video_resolution = true;
    };
 
    // loading flag for audio processing 
    std::string flag_use_audio_processing;
    if( ( config_.GetStringValue(kConfigAudioProcessing, 
                &flag_use_audio_processing ) == true )  && 
        (flag_use_audio_processing.compare("true") == 0 ) ) {

        // audio processing is enabled
        audio_processing_enable = true;

        CONFIG_LOAD_BOOL(kConfigAudioEchoCancel,audio_echo_cancel);
        CONFIG_LOAD_BOOL(kConfigAudioGainControl,audio_gain_control);
        CONFIG_LOAD_BOOL(kConfigAudioHighPassFilter,audio_highpass_filter);
        CONFIG_LOAD_BOOL(kConfigAudioNoiseSuppression,audio_noise_suppression);
    };

    // level control is not depend on the audio processing
    CONFIG_LOAD_BOOL(kConfigAudioLevelControl,audio_level_control);

    return true;
}

}   // media_config namespace



