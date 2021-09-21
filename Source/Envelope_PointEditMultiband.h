#pragma once

#include <JuceHeader.h>

#include "Envelope_Fwd.h"
#include "Envelope_Data.h"
#include "Envelope_Manager.h"

namespace myplug
{
	namespace envelope
	{
		struct GridInfo
		{
			bool isVerticalGridVisible = true;
			bool isVerticalSubGridVisible = false;
			bool isHorizontalGridVisible = true;
			bool isHorizontalSubGridVisible = false;

			int verticalGridSubdiv = 3;
			int verticalSubGridSubdiv = 3;
			double horizontalGridInterval = 0.2;
			int horizontalSubGridSubdiv = 3;

			std::function<juce::String(int)> verticalLabel = [](int index) -> juce::String { return juce::String{ index }; };
			std::function<juce::String(int)> horizontalLabel = [](int index) -> juce::String { return juce::String{ index }; };
		};

		class PointEditMBListener
		{
		public:
			virtual ~PointEditMBListener() = default;
			virtual void handleMoved(PointEditMB* p) = 0;
		};

		class PointEditMB : public juce::AnimatedAppComponent
		{
			EnvelopeManager* envManager_;
			std::function<void()> callbackWhenShownRangeChanged_;
			std::vector<PointEditMBListener*> listener_;

			// Zoom & Offset
			double zoomRateRange_ = 5.0;
			double baseZoomRate_ = 1000.0;
			double zoomRate_ = 1000.0;
			double offsetX_ = 0.0;

			// Handles
			const double circleRadius_ = 7.0;
			bool isEndPointMovableHorizontally = true;

			// Mouse
			int mouseIntersectedHandleIndex_ = -1;
			int mouseIntersectedBranchIndex_ = -1;
			int mouseIntersectedHGridIndex_ = -1;
			int mouseIntersectedVGridIndex_ = -1;
			juce::Point<float> prevMousePos;
			bool isMouseDown_ = false;
			bool isMouseOnBounds_ = false;
			bool canShrinkBeyondEndPoint_ = true;

			GridInfo grid_;
			bool snapToGrid_ = true;
			double snapDistance_ = 10.0;

			bool isMultiband_ = false;
			std::string editingBand_ = "low";

			void updateHandleIntersectionInfo(const juce::MouseEvent& e);
			void updateGridIntersectionInfo(const juce::MouseEvent& e);

		protected:
			void paint(juce::Graphics& g) override;
			void update() override;
			void resized() override;
			void mouseMove(const juce::MouseEvent& e) override;
			void mouseDown(const juce::MouseEvent& e) override;
			void mouseDrag(const juce::MouseEvent& e) override;
			void mouseUp(const juce::MouseEvent& e) override;
			void mouseExit(const juce::MouseEvent& e) override;
			void mouseDoubleClick(const juce::MouseEvent& e) override;
			void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
			void callAllListeners();

		public:
			PointEditMB(EnvelopeManager* envManager);
			double calcScreenXPos(double x);
			double calcEnvGenXPos(double x);
			double calcScreenYPos(double y);  // 0.0 <= y <= 1.0
			double calcEnvGenYPos(double y);

			void setMultiband(bool shouldBeMultiband) { isMultiband_ = shouldBeMultiband; }
			void setEditingBand(const std::string& editingBand = "mid") { editingBand_ = editingBand; }

			void setSnap(bool snap) { snapToGrid_ = snap; }
			void setCallbackWhenShownRangeChanged(std::function<void()> callbackWhenShownRangeChanged) { callbackWhenShownRangeChanged_ = callbackWhenShownRangeChanged; }
			void setZoomRateToWholeRange();
			void setZoomRate(double rate) { zoomRate_ = rate; }
			void setOffsetX(double x) { offsetX_ = x; }
			void setEndPointXMovement(bool isMovable) { isEndPointMovableHorizontally = isMovable; }
			void setZoomRateRestriction(bool canShrinkBeyondEndPoint) { canShrinkBeyondEndPoint_ = canShrinkBeyondEndPoint; }
			void setNewEnvelopeManager(EnvelopeManager* envManager) { envManager_ = envManager; }

			double getZoomRate() { return zoomRate_; }
			double getOffsetX() { return offsetX_; }
			double getZoomedWidth();
			EnvelopeManager* getEnvelopeManager() { return envManager_; }
			GridInfo& getGridDataRef() { return grid_; }

			void addListener(PointEditMBListener* listener) { listener_.push_back(listener); }
			void removeListener(PointEditMBListener* listener) { listener_.erase(std::remove(listener_.begin(), listener_.end(), listener), listener_.end()); }
		};
	}
}