#include "Envelope_Manager.h"
#include "Envelope_VoiceController.h"

void myplug::envelope::EnvelopeManager::registerEnvGenerator(CachedEnvelopeGenerator* envGenerator)
{
	envGenerator_.push_back(envGenerator);
}

void myplug::envelope::EnvelopeManager::removeEnvGenerator(CachedEnvelopeGenerator* envGenerator)
{
	envGenerator_.erase(std::remove(envGenerator_.begin(), envGenerator_.end(), envGenerator), envGenerator_.end());
}

void myplug::envelope::EnvelopeManager::clearEnvVoices()
{
	envControllers_.clear();
}

void myplug::envelope::EnvelopeManager::addEnvVoiceController(EnvelopeVoiceController* envVoice)
{
	if (envVoice != nullptr)
	{
		envControllers_.push_back(envVoice);
		envVoice->setManagerPtr(this);
	}
}

void myplug::envelope::EnvelopeManager::removeEnvVoiceController(EnvelopeVoiceController* envVoice)
{
	envVoice->setManagerPtr(nullptr);
	envControllers_.erase(std::remove(envControllers_.begin(), envControllers_.end(), envVoice), envControllers_.end());
}

void myplug::envelope::EnvelopeManager::setRate(double rate)
{
	envelopeRate_ = rate;
	for (const auto& i : envControllers_)
	{
		i->notifyRateHasChanged();
	}
}
