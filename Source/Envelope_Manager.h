#pragma once

#include "Envelope_Fwd.h"
#include "Envelope_Data.h"

#include <vector>

namespace myplug
{
	namespace envelope
	{
		enum class LoopMode
		{
			Oneshot,
			Loop,
			PingpongLoop,
			LoopThenRelease,
			PingPongLoopThenRelease,
			LoopThenJumptoRelease,
			PingPongLoopThenJumptoRelease,
		};

		class EnvelopeManager
		{
			std::vector<CachedEnvelopeGenerator*> envGenerator_;
			std::vector<EnvelopeVoiceController*> envControllers_;
			LoopMode loopMode_ = LoopMode::Oneshot;

			double envelopeRate_ = 1.0;
			double envelopeAttackTime_ = 0.0;

		public:
			void registerEnvGenerator(CachedEnvelopeGenerator* envGenerator);
			void removeEnvGenerator(CachedEnvelopeGenerator* envGenerator);
			void clearEnvVoices();

			void addEnvVoiceController(EnvelopeVoiceController* envVoice);
			void removeEnvVoiceController(EnvelopeVoiceController* envVoice);

			void setRate(double rate);
			void setAttackTime(double time) { envelopeAttackTime_ = time; }
			void setLoopMode(LoopMode mode) { loopMode_ = mode; }

			double getRate() { return envelopeRate_; }
			double getAttackTime() { return envelopeAttackTime_; }
			LoopMode getLoopMode() { return loopMode_; }
			CachedEnvelopeGenerator* getEnvGeneratorPtr()
			{
				if (envGenerator_.size() == 0)
				{
					return nullptr;
				}
				return *envGenerator_.begin(); 
			}

			CachedEnvelopeGenerator* getSpecificEnvGeneratorPtrFromName(const std::string& name)
			{ 
				for (const auto& i : envGenerator_)
				{
					if (i->getName() == name)
					{
						return i;
					}
				}
				return nullptr; 
			}
			std::vector<CachedEnvelopeGenerator*>& getEnvGeneratorsPtr() { return envGenerator_; }
			std::vector<EnvelopeVoiceController*>& getVoiceControllerRef() { return envControllers_; }
		};
	}
}