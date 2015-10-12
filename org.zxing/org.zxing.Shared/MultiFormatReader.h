#pragma once

#include "lib/zxing-all-in-one.h"

namespace org { namespace zxing {
	
	public delegate void OnCodeHandler(Platform::String^ code);

  /*public class Enum BarcodeFormat {
    None = 0,
    QrCode = 1 < ::zxing::BarcodeFormat::BarcodeFormat_QR_CODE,
    DataMatrix = 1 < ::zxing::BarcodeFormat::BarcodeFormat_DATA_MATRIX,
    UPCE = 1 < ::zxing::BarcodeFormat::BarcodeFormat_UPC_E,
    UPCA = 1 < ::zxing::BarcodeFormat::BarcodeFormat_UPC_A,
    EAN8 = 1 < ::zxing::BarcodeFormat::BarcodeFormat_EAN_8,
    EAN13 = 1 < ::zxing::BarcodeFormat::BarcodeFormat_EAN_13,
    CODE128 = 1 < ::zxing::BarcodeFormat::BarcodeFormat_CODE_128,
    CODE39 = 1 < ::zxing::BarcodeFormat::BarcodeFormat_CODE_39,
    ITF = 1 < ::zxing::BarcodeFormat::BarcodeFormat_ITF
  };*/

	public ref class MultiFormatReader sealed {
	public:
		static event OnCodeHandler^ OnCode;

		static void callOnCode(Platform::String^ text) {
			OnCode(text);
		}
	};
}}