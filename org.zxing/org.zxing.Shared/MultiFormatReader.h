#pragma once

#include <wrl\client.h>

namespace org { namespace zxing {
	
	public delegate void OnCodeHandler(Platform::String^ code);

	public ref class MultiFormatReader sealed {
	public:
		MultiFormatReader();

		static event OnCodeHandler^ OnCode;

		static void callOnCode(Platform::String^ text) {
			OnCode(text);
		}

		Windows::Foundation::IAsyncAction^ ReadAsync(Windows::Media::Capture::MediaCapture^ capture, int width, int height, OnCodeHandler^ callback);
	private:
		Windows::UI::Core::CoreDispatcher^ dispatcher;
	};
}}