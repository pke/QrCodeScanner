#pragma once

#include "lib/zxing-all-in-one.h"

namespace org {
	namespace zxing {

		public delegate void OnCodeHandler(Platform::String^ code);

		using namespace Platform::Metadata;
		[Flags]
		public enum class BarcodeFormat : unsigned int {
			None = 0,
			QrCode = 1 << ::zxing::BarcodeFormat::BarcodeFormat_QR_CODE,
			DataMatrix = 1 << ::zxing::BarcodeFormat::BarcodeFormat_DATA_MATRIX,
			UPCE = 1 << ::zxing::BarcodeFormat::BarcodeFormat_UPC_E,
			UPCA = 1 << ::zxing::BarcodeFormat::BarcodeFormat_UPC_A,
			EAN8 = 1 << ::zxing::BarcodeFormat::BarcodeFormat_EAN_8,
			EAN13 = 1 << ::zxing::BarcodeFormat::BarcodeFormat_EAN_13,
			CODE128 = 1 << ::zxing::BarcodeFormat::BarcodeFormat_CODE_128,
			CODE39 = 1 << ::zxing::BarcodeFormat::BarcodeFormat_CODE_39,
			ITF = 1 << ::zxing::BarcodeFormat::BarcodeFormat_ITF,
			TryHarder = ::zxing::DecodeHints::TRYHARDER_HINT
		};

#if WINAPI_FAMILY!=WINAPI_FAMILY_PHONE_APP
		public interface class IVideoEffectDefinition {
			property Platform::String^ ActivatableClassId { Platform::String^ get(); }
			property Windows::Foundation::Collections::IPropertySet^ Properties { Windows::Foundation::Collections::IPropertySet^ get(); }
		};
#endif

		public ref class MultiFormatReaderDefinition sealed
#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
			: public Windows::Media::Effects::IVideoEffectDefinition
#else
			: public IVideoEffectDefinition
#endif
		{
		public:
			MultiFormatReaderDefinition(BarcodeFormat formats, OnCodeHandler^ handler)
				: activatableClassId(L"org.zxing.BarcodeFilter")
				, properties(ref new Windows::Foundation::Collections::PropertySet()) {
				properties->Insert(L"oncode", handler);
				properties->Insert("formats", formats);
			}
			virtual property Platform::String^ ActivatableClassId {
				Platform::String^ get() {
					return activatableClassId;
				}
			}

			virtual property Windows::Foundation::Collections::IPropertySet^ Properties {
				Windows::Foundation::Collections::IPropertySet^ get() {
					return properties;
				}
			}

			property BarcodeFormat Formats {
				BarcodeFormat get() {
					return formats;
				}
			}
		private:
			Platform::String^ activatableClassId;
			BarcodeFormat formats;
			Windows::Foundation::Collections::PropertySet^ properties;
		};
	}
}