#include "ParamAttachment.h"

#include "PluginEditor.h"

void myplug::Listener_PlugEditGUIScaling::endGesture(NumericSpinBox* s) 
{
	if (proc_)
	{
		static_cast<MyplugAudioProcessorEditor*>(proc_)->setMyplugGUIScaling(s->getCurrentNumber() / 100.0);
	}
}
