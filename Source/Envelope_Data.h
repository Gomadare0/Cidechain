#pragma once

#include <vector>
#include <algorithm>
#include <numeric>
#include <cassert>
#include <string>
#include <cmath>
#include "ValueNormalization.h"

namespace myplug
{
	namespace envelope
	{
		enum class InterpolationType
		{
			Linear,
			Sigmoid,
			Step
		};
		enum class LoopState
		{
			None,
			LoopStart,
			LoopEnd,
		};

		struct Point
		{
			double x = 0.0;
			double y = 0.0;
			double param1 = 0.0;
			InterpolationType interpolation = InterpolationType::Linear;
			LoopState loop = LoopState::None;
		};

		struct EnvelopeGenerator
		{
		private:
			std::vector<Point> points;
			std::string name_;

		public:
			void addPoint(double xPos, double yPos, InterpolationType interpolation = InterpolationType::Sigmoid, double Param1 = 0.0, LoopState loop = LoopState::None)
			{
				points.push_back(Point{xPos, yPos, Param1, interpolation, loop });
				std::sort(points.begin(), points.end(), [](Point& l, Point& r) { return l.x < r.x; });
			}
			void removeNearestPoint(double xPos)
			{
				if (points.size() == 0)
				{
					return;
				}
				auto targetItr = points.begin();
				double minDistance = std::numeric_limits<double>::max();
				for (auto itr = points.begin(); itr != points.end(); ++itr)
				{
					double currentDistance = abs(xPos - itr->x);
					if (currentDistance > minDistance)
					{
						points.erase(targetItr);
						points.shrink_to_fit();
						return;
					}
					else
					{
						minDistance = currentDistance;
						targetItr = itr;
					}
				}
			}
			void removePoint(size_t index)
			{
				if (index >= points.size())
				{
					return;
				}
				points.erase(points.begin() + index);
				points.shrink_to_fit();
			}	

			void setName(const std::string& name) { name_ = name; }
			void setPoint(size_t index, const Point& point)
			{
				if (index < points.size())
				{
					points[index] = point;
				}
			}

			int getLeftPointIndex(double xPos)
			{
				if (points.size() == 0)
				{
					return 0;
				}
				else if (xPos <= points.begin()->x)
				{
					return -1;
				}
				else if (xPos >= (--points.end())->x)
				{
					return points.size()-1;
				}
				else
				{
					for (auto itr = points.begin(); itr != points.end(); ++itr)
					{
						if (itr->x <= xPos && (itr + 1)->x > xPos)
						{
							return std::distance(points.begin(), itr);
						}
					}
					return -1;
				}
			}
			int getRightPointIndex(double xPos) 
			{
				if (points.size() == 0)
				{
					return 0;
				}
				else if (xPos <= points.begin()->x)
				{
					return 0;
				}
				else if (xPos >= (--points.end())->x)
				{
					return points.size();
				}
				else
				{
					for (auto itr = points.begin(); itr != points.end(); ++itr)
					{
						if (itr->x <= xPos && (itr + 1)->x > xPos)
						{
							return std::distance(points.begin(), itr) + 1;
						}
					}
					return -1;
				}
			}
			double getInterpolatedValue(double xPos)
			{
				if (points.size() == 0)
				{
					return 0.0;
				}
				else if (xPos <= getFirstPoint().x)
				{
					double ret = getFirstPoint().y;
					return ret;
				}
				else if (xPos >= getLastPoint().x)
				{
					double ret = getLastPoint().y;
					return ret;
				}
				else
				{
					bool canReturn = false;
					double ret = 0.0;
					for (int k = 0; k < getNumPoint(); ++k)
					{
						if (points[k].x <= xPos && points[k+1].x > xPos)
						{
							const Point& first = points[k];
							const Point& last = points[k+1];

							switch (first.interpolation)
							{
							case InterpolationType::Linear:
								ret = (last.y - first.y) / (last.x - first.x) * (xPos - first.x) + first.y;
								canReturn = true;
								break;
							case InterpolationType::Sigmoid:
							{
								double x = normalizeValue<double>(xPos, first.x, last.x);
								double y = 0.0;
								auto calcSigmoid = [](double a, double x) { return 1.0 / (1.0 + exp(-1.0 * a * x)); };
								if (first.param1 == 0.0)
								{
									// same as linear
									y = (last.y - first.y) * x + first.y;
								}
								else
								{
									// sigmoid
									if (first.param1 > 0)
									{
										y = (2.0 * calcSigmoid(first.param1, x) - 1) / (2.0 * calcSigmoid(first.param1, 1) - 1);
									}
									else
									{
										y = (calcSigmoid(first.param1, x - 1.0) - calcSigmoid(first.param1, -1.0)) / (calcSigmoid(first.param1, 0.0) - calcSigmoid(first.param1, -1.0));
									}

									y = denormalizeValue<double>(y, first.y, last.y);
								}
								ret = y;
								canReturn = true;
							}
								break;
							case InterpolationType::Step:
								ret = first.y;
								canReturn = true;
								break;
							default:
								break;
							}
						}
						if (canReturn == true)
						{
							return ret;
						}
					}
				}

				return 0.0;
			}
			double getXRange()
			{
				if (points.size() < 2)
				{
					return 0.0;
				}
				return (--points.end())->x - points.begin()->x;
			}
			double getYRange()
			{
				if (points.size() < 2)
				{
					return 0.0;
				}
				double max = points.begin()->y; double min = points.begin()->y;
				for (const auto& i : points)
				{
					if (i.y > max) max = i.y;
					if (i.y < min) min = i.y;
				}
				return max - min;
			}
			size_t getNumPoint() { return points.size(); }
			Point& getPoint(size_t index) 
			{
				// Index is out of range
				assert(index < points.size());

				return points[index];
			}
			Point& getFirstPoint()
			{
				if (points.size() == 0)
				{
					// No point in points
					assert(true);
				}
				return points[0];
			}
			Point& getLastPoint()
			{
				if (points.size() == 0)
				{
					// No point in points
					assert(false);
				}
				if (points.size() == 1)
				{
					return points[0];
				}
				return points[points.size()-1];
			}

			const std::string& getName() { return name_; }

			void sortPoints()
			{
				std::sort(points.begin(), points.end(), [](Point& l, Point& r) { return l.x < r.x; });
			}
			void clearPoints()
			{
				points.clear();
			}
		};

		struct CachedEnvelopeGenerator : public EnvelopeGenerator
		{
			double resolution_ = 0.001;
			double prevLength = 0.0;
			std::vector<double> buffer;

		public:
			void setResolution(double res)
			{
				resolution_ = res;
			}

			double getResolution()
			{
				return resolution_;
			}

			double getCachedInterpolatitonValue(double xPos)
			{
				if (buffer.size() == 0)
				{
					return 0.0;
				}
				else if (xPos <= 0.0)
				{
					return *buffer.begin();
				}
				else if (xPos >= buffer.size() * resolution_)
				{
					return *(--buffer.end());
				}
				else
				{
					size_t index = std::floor(xPos / resolution_);
					double dy = buffer[std::clamp<size_t>(index + 1, 0, buffer.size()-1)] - buffer[index];
					return dy / resolution_ * (xPos - index * resolution_) + buffer[index];
				}
			}

			// if endXPos < 0, render till last
			void render(double startXPos = 0.0, double endXPos = -1.0)
			{
				double length = getLastPoint().x;
				double curPos = 0;

				// resize vector
				if (length != prevLength)
				{
					buffer.resize(getLastPoint().x / resolution_ + 1);
				}

				size_t endIndex = endXPos < 0.0 ? buffer.size() - 1 : endXPos / resolution_;

				for (size_t i = (startXPos / resolution_); i < endIndex; ++i)
				{
					buffer[i] = getInterpolatedValue(curPos);
					curPos += resolution_;
				}

				// buf of last point to get better soundings
				buffer[buffer.size() - 1] = getLastPoint().y;
			}
		};
	}
}