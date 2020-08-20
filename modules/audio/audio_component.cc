/******************************************************************************
 * Copyright 2020 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include "modules/audio/audio_component.h"
#include "modules/audio/proto/audio_conf.pb.h"
#include "modules/common/proto/geometry.pb.h"

namespace apollo {
namespace audio {

using apollo::common::Point3D;
using apollo::drivers::microphone::config::AudioData;

AudioComponent::~AudioComponent() {}

std::string AudioComponent::Name() const {
  // TODO(all) implement
  return "";
}

bool AudioComponent::Init() {
  AudioConf audio_conf;
  if (!ComponentBase::GetProtoConfig(&audio_conf)) {
    AERROR << "Unable to load audio conf file: "
           << ComponentBase::ConfigFilePath();
    return false;
  }
  localization_reader_ =
      node_->CreateReader<localization::LocalizationEstimate>(
          audio_conf.topic_conf().localization_topic_name(), nullptr);
  audio_writer_ = node_->CreateWriter<AudioDetection>(
      audio_conf.topic_conf().audio_detection_topic_name());
  respeaker_extrinsics_file_ = audio_conf.respeaker_extrinsics_path();
  return true;
}

bool AudioComponent::Proc(const std::shared_ptr<AudioData>& audio_data) {
  // TODO(all) remove GetSignals() multiple calls
  audio_info_.Insert(audio_data);
  AudioDetection audio_detection;
  *audio_detection.mutable_position() =
      direction_detection_.EstimateSoundSource(
          audio_info_.GetSignals(audio_data->microphone_config().chunk()),
          respeaker_extrinsics_file_,
          audio_data->microphone_config().sample_rate(),
          audio_data->microphone_config().mic_distance());

  auto signals =
      audio_info_.GetSignals(audio_data->microphone_config().chunk());
  // TODO(Hongyi): enable after upload model files
  // bool is_siren = siren_detection_.Evaluate(signals);
  // audio_detection.set_is_siren(is_siren);
  MovingResult moving_result = moving_detection_.Detect(signals);
  audio_detection.set_moving_result(moving_result);
  // TODO(all) add header to audio_detection
  audio_writer_->Write(audio_detection);
  return true;
}

}  // namespace audio
}  // namespace apollo