#pragma once
#include "Constants.h"
#include <cmath>

namespace myplug::DSP
{
	class FilterBase
	{
	public:
		virtual float processSample(float i) = 0;
	};

	class Butterworth_2nd : public FilterBase
	{
		float a0 = 1, a1 = 0, a2 = 0, b0 = 0, b1 = 0, b2 = 0;
		float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

	public:
		void calcCoeff(float sampleRate, float freq, bool isHighPass)
		{
			const float omega = 2.0 * PIf * freq / sampleRate;
			const float sqrt2 = 1.41421356;

			a0 = 2 + sqrt2 * std::sinf(omega);
			a1 = -4 * std::cosf(omega);
			a2 = 2 - sqrt2 * std::sinf(omega);

			if (!isHighPass)
			{
				b0 = 1 - std::cos(omega);
				b1 = 2 - 2 * std::cos(omega);
				b2 = 1 - std::cos(omega);
			}
			else
			{
				b0 = 1 + std::cos(omega);
				b1 = - 2 - 2 * std::cos(omega);
				b2 = 1 + std::cos(omega);
			}
		}

		float processSample(float i)
		{
			float y = b0 * i + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
			y /= a0;
			
			x2 = x1;
			x1 = i;
			y2 = y1;
			y1 = y;

			return y;
		}
	};
}