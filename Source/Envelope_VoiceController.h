#pragma once

#include "Envelope_Fwd.h"

namespace myplug
{
	namespace envelope
	{
		// The instance is intended to be owned by SynthVoice or equivalents.
		// then pointer of this class is registered to EnvelopeManager so that gui can access to the voicecontroller through EnvManager
		class EnvelopeVoiceController
		{
			myplug::envelope::EnvelopeManager* parentManager_ = nullptr;
			double rate_ = 1.0; // set through envManager
			double deltaX_ = 0.0;
			double x_ = 0.0;
			bool isReturning_ = false; // pingpong loop
			bool prevNoteOn = false;
			double loopStartX_ = 0.0;
			double loopEndX_ = 0.0;
			
			double getXPosWithLoop(bool isNoteOn = false, double offset = 0.0, bool* retShouldReturn = nullptr)
			{
				if (parentManager_ == nullptr) return 0.0;
				//const auto& points = parentManager_->getEnvGeneratorPtr()->getPointsRef();

				double bufx = x_ + offset * (isReturning_ == false ? 1.0 : -1.0);

				if (isNoteOn)
				{
					if (bufx > loopEndX_)
					{
						switch (parentManager_->getLoopMode())
						{
						case envelope::LoopMode::Loop:
							// Jump to start
							bufx -= (loopEndX_ - loopStartX_);
							break;
						case envelope::LoopMode::LoopThenRelease:
							bufx -= (loopEndX_ - loopStartX_);
							break;
						case envelope::LoopMode::LoopThenJumptoRelease:
							bufx -= (loopEndX_ - loopStartX_);
							break;
						case envelope::LoopMode::PingpongLoop:
							bufx -= deltaX_ * rate_ * 2.0;
							if (retShouldReturn) *retShouldReturn = true;
							break;
						case envelope::LoopMode::PingPongLoopThenRelease:
							bufx -= deltaX_ * rate_ * 2.0;
							if (retShouldReturn) *retShouldReturn = true;
							break;
						case envelope::LoopMode::PingPongLoopThenJumptoRelease:
							bufx -= deltaX_ * rate_ * 2.0;
							if (retShouldReturn) *retShouldReturn = true;
							break;
						default:
							break;
						}
					}
					else if (bufx < loopStartX_ && isReturning_ == true)
					{
						// Ping Pong
						bufx += deltaX_ * rate_ * 2.0;
						if (retShouldReturn) *retShouldReturn = false;
					}
				}
				else
				{
					if (prevNoteOn == true) // when note-on turned into note-off
					{
						switch (parentManager_->getLoopMode())
						{
						case envelope::LoopMode::LoopThenJumptoRelease:
							bufx = loopEndX_;
							break;
						case envelope::LoopMode::PingPongLoopThenRelease:
							if (retShouldReturn) *retShouldReturn = false;
							break;
						case envelope::LoopMode::PingPongLoopThenJumptoRelease:
							bufx = loopEndX_;
							if (retShouldReturn) *retShouldReturn = false;
							break;
						default:
							break;
						}
					}

					if (bufx > loopEndX_)
					{
						switch (parentManager_->getLoopMode())
						{
						case envelope::LoopMode::Loop:
							// Jump to start
							bufx -= (loopEndX_ - loopStartX_);
							break;
						case envelope::LoopMode::PingpongLoop:
							bufx -= deltaX_ * rate_ * 2.0;
							if (retShouldReturn) *retShouldReturn = true;
							break;
						default:
							break;
						}
					}
					else if (bufx < loopStartX_ && isReturning_ == true && parentManager_->getLoopMode() == envelope::LoopMode::PingpongLoop)
					{
						// Ping Pong
						bufx += deltaX_ * rate_ * 2.0;
						if (retShouldReturn) *retShouldReturn = false;
					}
				}

				return bufx;
			}

		public:
			// Automatically called in EnvelopeManager::addEnvVoiceController()
			void setManagerPtr(EnvelopeManager* ptr) { parentManager_ = ptr; }

			double getCurrentValue(double offsetInRatioOfDeltaX_ = 0.0, bool isNoteOn = false)
			{
				if (parentManager_  != nullptr)
				{
					return parentManager_->getEnvGeneratorPtr()->getCachedInterpolatitonValue(getXPosWithLoop(isNoteOn, offsetInRatioOfDeltaX_ * deltaX_ * rate_));
				}
				else
				{
					return 0.0;
				}
			}
			void setXPos(double x)
			{
				x_ = x;
			}
			double getXPos()
			{
				return x_;
			}
			bool getIsXAmongEnvelope()
			{
				if (parentManager_->getEnvGeneratorPtr() == nullptr)
				{
					return false;
				}
				auto points = parentManager_->getEnvGeneratorPtr();
				return x_ <= points->getFirstPoint().x || points->getLastPoint().x <= x_ ? false : true;
			}
			void update(bool isNoteOn = false, bool dontUpdateBeyondEnvgen = false)
			{
				if (parentManager_ == nullptr) return;

				if (dontUpdateBeyondEnvgen)
				{
					auto points = parentManager_->getEnvGeneratorPtr();
					if (getXPosWithLoop(isNoteOn, deltaX_ * rate_) > points->getLastPoint().x)
					{
						prevNoteOn = isNoteOn;
						return;
					}
				}
				x_ = getXPosWithLoop(isNoteOn, deltaX_ * rate_, &isReturning_);

				prevNoteOn = isNoteOn;
			}

			void reset() { x_ = 0.0; isReturning_ = false; }
			void setDelta(double delta) { deltaX_ = delta; }
			void notifyRateHasChanged() { rate_ = parentManager_->getRate(); }
			void notifyLoopPosChanged()
			{
				auto points = parentManager_->getEnvGeneratorPtr();
				for (int i = 0; i < points->getNumPoint(); ++i)
				{
					if (points->getPoint(i).loop == LoopState::LoopStart)
					{
						loopStartX_ = points->getPoint(i).x;
					}
					else if (points->getPoint(i).loop == LoopState::LoopEnd)
					{
						loopEndX_ = points->getPoint(i).x;
					}
				}
			}
		};
	}
}