#pragma once

namespace myplug
{
	template<typename T>
	[[nodiscard]] T normalizeValue(const T& value, const T& min, const T& max)
	{
		T length = max - min;
		if(length == 0.0)
		{
			return 0.0;
		}
		return (value - min) / length;
	}

	template<typename T>
	[[nodiscard]] T denormalizeValue(const T& normalizedValue, const T& min, const T& max)
	{
		T length = max - min;
		return normalizedValue * length + min;
	}
}