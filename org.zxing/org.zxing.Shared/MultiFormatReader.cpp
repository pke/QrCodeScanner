﻿#include "pch.h"
#include "MultiFormatReader.h"

#include "BaseFilter.h"

#include <wrl\client.h>
#include <wrl\event.h>

#include "WinRTBufferOnMF2DBuffer.h"

Windows::UI::Core::CoreDispatcher^ dispatcher(Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher);

// The following XML snippet needs to be added to Package.appxmanifest:
//
//<Extensions>
//  <Extension Category = "windows.activatableClass.inProcessServer">
//    <InProcessServer>
//      <Path>org.zxing.WindowsPhone.dll</Path>
//      <ActivatableClass ActivatableClassId = "BarcodeReader" ThreadingModel = "both" />
//    </InProcessServer>
//  </Extension>
//</Extensions>
//

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")


// A partially-functional MF Source (just enough to get SourceReader started)
class PlaceHolderVideoSource :
    public Microsoft::WRL::RuntimeClass <
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
    IMFMediaEventGenerator,
    IMFMediaSource
    >
{
public:
    PlaceHolderVideoSource::PlaceHolderVideoSource(_In_ const Microsoft::WRL::ComPtr<IMFMediaType>& mediaType) {
        IMFMediaType* arrayTypes[] = { mediaType.Get() };
        Microsoft::WRL::ComPtr<IMFStreamDescriptor> streamDescr;
        CHK(MFCreateStreamDescriptor(0, ARRAYSIZE(arrayTypes), arrayTypes, &streamDescr));
        CHK(MFCreatePresentationDescriptor(1, streamDescr.GetAddressOf(), &_presDescr));
    }

    //
    // IMFMediaEventGenerator
    //

    IFACEMETHOD(GetEvent)(_In_ DWORD /*dwFlags*/, _COM_Outptr_ IMFMediaEvent ** /*ppEvent*/) override {
        __debugbreak();
        return E_NOTIMPL;
    }

    IFACEMETHOD(BeginGetEvent)(_In_ IMFAsyncCallback * /*pCallback*/, _In_ IUnknown * /*punkState*/) override {
        return S_OK;
    }

    IFACEMETHOD(EndGetEvent)(_In_ IMFAsyncResult * /*pResult*/, _COM_Outptr_  IMFMediaEvent ** /*ppEvent*/) override {
        __debugbreak();
        return E_NOTIMPL;
    }

    IFACEMETHOD(QueueEvent)(
        _In_ MediaEventType /*met*/,
        _In_ REFGUID /*guidExtendedType*/,
        _In_ HRESULT /*hrStatus*/,
        _In_opt_ const PROPVARIANT * /*pvValue*/
        ) override {
        __debugbreak();
        return E_NOTIMPL;
    }


    //
    // IMFMediaSource
    //

    IFACEMETHOD(GetCharacteristics)(_Out_ DWORD *pdwCharacteristics) {
        *pdwCharacteristics = 0;
        return S_OK;
    }

    IFACEMETHOD(CreatePresentationDescriptor)(_COM_Outptr_  IMFPresentationDescriptor **ppPresentationDescriptor) {
        return _presDescr.CopyTo(ppPresentationDescriptor);
    }

    IFACEMETHOD(Start)(
        _In_opt_ IMFPresentationDescriptor * /*pPresentationDescriptor*/,
        _In_opt_ const GUID * /*pguidTimeFormat*/,
        _In_opt_ const PROPVARIANT * /*pvarStartPosition*/
        ) {
        __debugbreak();
        return E_NOTIMPL;
    }

    IFACEMETHOD(Stop)() {
        __debugbreak();
        return E_NOTIMPL;
    }

    IFACEMETHOD(Pause)() {
        __debugbreak();
        return E_NOTIMPL;
    }

    IFACEMETHOD(Shutdown)() {
        return S_OK;
    }

private:
    Microsoft::WRL::ComPtr<IMFPresentationDescriptor> _presDescr;
};

Microsoft::WRL::ComPtr<IMFTransform> CreateVideoProcessor() {
    //Logger.VideoProcessor_CreateStart();

    //
    // Create two different formats to force the SourceReader to create a video processor
    //

    Microsoft::WRL::ComPtr<IMFMediaType> outputFormat;
    CHK(MFCreateMediaType(&outputFormat));
    CHK(outputFormat->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
    CHK(outputFormat->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
    CHK(outputFormat->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
    CHK(MFSetAttributeSize(outputFormat.Get(), MF_MT_FRAME_SIZE, 640, 480));
    CHK(MFSetAttributeRatio(outputFormat.Get(), MF_MT_FRAME_RATE, 1, 1));
    CHK(MFSetAttributeRatio(outputFormat.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1));

    Microsoft::WRL::ComPtr<IMFMediaType> inputFormat;
    CHK(MFCreateMediaType(&inputFormat));
    CHK(outputFormat->CopyAllItems(inputFormat.Get()));
    CHK(MFSetAttributeSize(outputFormat.Get(), MF_MT_FRAME_SIZE, 800, 600));

    //
    // Create the SourceReader
    //

    auto source = Microsoft::WRL::Make<PlaceHolderVideoSource>(inputFormat);
    CHKOOM(source);

    Microsoft::WRL::ComPtr<IMFAttributes> sourceReaderAttr;
    CHK(MFCreateAttributes(&sourceReaderAttr, 2));
    CHK(sourceReaderAttr->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, true));
    CHK(sourceReaderAttr->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, false));

    Microsoft::WRL::ComPtr<IMFReadWriteClassFactory> readerFactory;
    Microsoft::WRL::ComPtr<IMFSourceReaderEx> sourceReader;
    CHK(CoCreateInstance(CLSID_MFReadWriteClassFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&readerFactory)));
    CHK(readerFactory->CreateInstanceFromObject(
        CLSID_MFSourceReader,
        static_cast<IMFMediaSource*>(source.Get()),
        sourceReaderAttr.Get(),
        IID_PPV_ARGS(&sourceReader)
        ));

    CHK(sourceReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputFormat.Get()));

    //
    // Extract its video processor
    //

    Microsoft::WRL::ComPtr<IMFTransform> processor;
    unsigned int n = 0;
    while (true) {
        GUID category;
        Microsoft::WRL::ComPtr<IMFTransform> transform;
        CHK(sourceReader->GetTransformForStream((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, n, &category, &transform));

        // Only care about MFTs which are video processors and D3D11 aware
        Microsoft::WRL::ComPtr<IMFAttributes> transformAttr;
        if ((category == MFT_CATEGORY_VIDEO_PROCESSOR)
            && SUCCEEDED(transform->GetAttributes(&transformAttr))
            && (MFGetAttributeUINT32(transformAttr.Get(), MF_SA_D3D11_AWARE, 0) != 0)
            )
        {
            processor = transform;
            break;
        }

        n++;
    }

    //Logger.VideoProcessor_CreateStop(processor.Get());
    return processor;
}

class LumiaAnalyzer WrlSealed : public Microsoft::WRL::RuntimeClass<Video1in1outEffect> {
    InspectableClass(L"org.zxing.BarcodeFilter", TrustLevel::BaseTrust);

public:
    LumiaAnalyzer() : _outputSubtype(MFVideoFormat_NV12), _processingSample(0), _length(640), formats(::zxing::DecodeHints::DEFAULT_HINT) {
        _passthrough = true;
    }

	// Without implementing this here, the compiler gets confused which RuntimeClassInitialize method to use.
    HRESULT RuntimeClassInitialize() {
        return Video1in1outEffect::RuntimeClassInitialize();
    }

	virtual void Initialize(_In_ Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ props) {
		//_length = (int)props->Lookup(L"length");
		oncode = (org::zxing::OnCodeHandler^)props->Lookup(L"oncode");
		formats = ::zxing::DecodeHints((unsigned int)props->Lookup("formats"));
	}

	org::zxing::OnCodeHandler^ oncode;

    // Format management
    virtual std::vector<unsigned long> GetSupportedFormats() const override {
        std::vector<unsigned long> formats;

        // MFT supports pretty much anything uncompressed (pass-through)
        formats.push_back(MFVideoFormat_NV12.Data1);
        formats.push_back(MFVideoFormat_420O.Data1);
        formats.push_back(MFVideoFormat_YUY2.Data1);
        formats.push_back(MFVideoFormat_YV12.Data1);
        formats.push_back(MFVideoFormat_RGB32.Data1);
        formats.push_back(MFVideoFormat_ARGB32.Data1);

        return formats;
    }

    // Data processing
    virtual void StartStreaming(_In_ unsigned long format, _In_ unsigned int width, _In_ unsigned int height) override;
    virtual void ProcessSample(_In_ const Microsoft::WRL::ComPtr<IMFSample>& sample) override;

private:

    Microsoft::WRL::ComPtr<IMFMediaBuffer> _ConvertBuffer(
        _In_ const Microsoft::WRL::ComPtr<IMFMediaBuffer>& buffer
        ) const;

    unsigned int _length;
	::zxing::DecodeHints formats;
    Microsoft::WRL::ComPtr<IMFTransform> _processor;

    GUID _outputSubtype;
    unsigned int _outputWidth;
    unsigned int _outputHeight;

    volatile unsigned int _processingSample;
        
    mutable ::Microsoft::WRL::Wrappers::SRWLock _analyzerLock;
};
ActivatableClass(LumiaAnalyzer);

void LumiaAnalyzer::StartStreaming(_In_ unsigned long /*format*/, _In_ unsigned int width, _In_ unsigned int height) {
    auto lock = _analyzerLock.LockExclusive();

    // Isotropic scaling
    float scale = _length / (float)max(width, height);
    _outputWidth = (unsigned int)(scale * width);
    _outputHeight = (unsigned int)(scale * height);

    _processor = CreateVideoProcessor();

    // Create the output media type
    Microsoft::WRL::ComPtr<IMFMediaType> outputType;
    CHK(MFCreateMediaType(&outputType));
    CHK(outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
    CHK(outputType->SetGUID(MF_MT_SUBTYPE, _outputSubtype));
    CHK(outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
    CHK(MFSetAttributeSize(outputType.Get(), MF_MT_FRAME_SIZE, _outputWidth, _outputHeight));
    CHK(MFSetAttributeRatio(outputType.Get(), MF_MT_FRAME_RATE, 1, 1));
    CHK(MFSetAttributeRatio(outputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1));

    // Set the input/output formats
    bool useGraphicsDevice = (_deviceManager != nullptr);
    if (useGraphicsDevice) {
        CHK(_processor->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(_deviceManager.Get())));

        HRESULT hrOutput = S_OK;
        HRESULT hrInput = _processor->SetInputType(0, _inputType.Get(), 0);
        if (SUCCEEDED(hrInput))	{
            hrOutput = _processor->SetOutputType(0, outputType.Get(), 0);
        }

        // Fall back on software if media types were rejected
        if (FAILED(hrInput) || FAILED(hrOutput)) {
            useGraphicsDevice = false;
        }
    }

    if (!useGraphicsDevice) {
        CHK(_processor->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, 0));
    CHK(_processor->SetInputType(0, _inputType.Get(), 0));
        CHK(_processor->SetOutputType(0, outputType.Get(), 0));
    }
}

static Platform::String^ charToPlatformString(const std::string& stdString) {
    std::wstring stdWString;
    stdWString.assign(stdString.begin(), stdString.end());
    Platform::String^ result = ref new Platform::String(stdWString.c_str());
    return result;
}

void LumiaAnalyzer::ProcessSample(_In_ const Microsoft::WRL::ComPtr<IMFSample>& sample) {
    // Ignore current sample if still processing the previous one
    if (InterlockedExchange(&_processingSample, true)) {
        return;
    }

    // Run async to reduce impact on video stream
    Windows::System::Threading::ThreadPool::RunAsync(ref new Windows::System::Threading::WorkItemHandler([this, sample](Windows::Foundation::IAsyncAction^)	{
        auto lock = _analyzerLock.LockExclusive();
        long long time = 0;
        sample->GetSampleTime(&time);

        Microsoft::WRL::ComPtr<IMFMediaBuffer> inputBuffer;
        CHK(sample->GetBufferByIndex(0, &inputBuffer));
        Microsoft::WRL::ComPtr<IMFMediaBuffer> outputBuffer = _ConvertBuffer(inputBuffer);

        // Create IBuffer wrappers
        Microsoft::WRL::ComPtr<WinRTBufferOnMF2DBuffer> outputWinRTBuffer;
        CHK(Microsoft::WRL::MakeAndInitialize<WinRTBufferOnMF2DBuffer>(&outputWinRTBuffer, outputBuffer, MF2DBuffer_LockFlags_Read, _inputDefaultStride));

        // Query the IBufferByteAccess interface.
        Microsoft::WRL::ComPtr<Windows::Storage::Streams::IBufferByteAccess> bufferByteAccess;
        reinterpret_cast<IInspectable*>(outputWinRTBuffer->GetIBuffer())->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));
        // Retrieve the buffer data.
        byte* pixels = nullptr;
        bufferByteAccess->Buffer(&pixels);

        ::zxing::MultiFormatReader reader;
        ::zxing::Ref<::zxing::GreyscaleLuminanceSource> imageRef(new ::zxing::GreyscaleLuminanceSource(pixels, this->_outputWidth, this->_outputHeight, 0, 0, this->_outputWidth, this->_outputHeight));
        ::zxing::GlobalHistogramBinarizer *binz = new ::zxing::GlobalHistogramBinarizer(imageRef);
        ::zxing::Ref<::zxing::Binarizer> bz(binz);
        ::zxing::BinaryBitmap *bb = new ::zxing::BinaryBitmap(bz);
        ::zxing::Ref<::zxing::BinaryBitmap> ref(bb);
        try {
            auto result = reader.decode(ref, formats);
            if (result && result->count() > 0) {
                auto text = result->getText()->getText();
                auto format = result->getBarcodeFormat();
                auto handler = ref new Windows::UI::Core::DispatchedHandler([text, this]() {
                    this->oncode(charToPlatformString(text));
                });
                dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, handler);
                OutputDebugStringA(text.c_str());
                OutputDebugStringA("\n");
            }
        } catch (const std::exception& e) {
            OutputDebugStringA(e.what());
            OutputDebugStringA("\n");
            int i = 0;
        }
        
        // Force MF buffer unlocking
        outputWinRTBuffer->Close();

        InterlockedExchange(&_processingSample, false);
    }));
}

// An RAII version of MFT_OUTPUT_DATA_BUFFER
class MftOutputDataBuffer : public MFT_OUTPUT_DATA_BUFFER {
public:
    MftOutputDataBuffer(_In_opt_ const Microsoft::WRL::ComPtr<IMFSample>& sample) {
        this->dwStreamID = 0;
        this->pSample = sample.Get();
        this->dwStatus = 0;
        this->pEvents = nullptr;

        if (this->pSample != nullptr) {
            this->pSample->AddRef();
        }
    }

    ~MftOutputDataBuffer() {
        if (this->pSample != nullptr) {
            this->pSample->Release();
        }
        if (this->pEvents != nullptr) {
            this->pEvents->Release();
        }
    }
};

Microsoft::WRL::ComPtr<IMFMediaBuffer> LumiaAnalyzer::_ConvertBuffer(
    _In_ const Microsoft::WRL::ComPtr<IMFMediaBuffer>& inputBuffer
    ) const
{
  //Logger.LumiaAnalyzer_ConvertStart((void*)this, _processor.Get());

    // Create the input MF sample
    Microsoft::WRL::ComPtr<IMFSample> inputSample;
    CHK(MFCreateSample(&inputSample));
    CHK(inputSample->AddBuffer(inputBuffer.Get()));
    CHK(inputSample->SetSampleTime(0));
    CHK(inputSample->SetSampleDuration(10000000));

    // Create the output MF sample
    // In SW mode, we allocate the output buffer
    // In HW mode, the video proc allocates
    Microsoft::WRL::ComPtr<IMFSample> outputSample;
    MFT_OUTPUT_STREAM_INFO outputStreamInfo;
    CHK(_processor->GetOutputStreamInfo(0, &outputStreamInfo));
    if (!(outputStreamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES))
    {
        Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer1D;
        CHK(MFCreate2DMediaBuffer(_outputWidth, _outputHeight, _outputSubtype.Data1, false, &buffer1D));

        CHK(MFCreateSample(&outputSample));
        CHK(outputSample->AddBuffer(buffer1D.Get()));
        CHK(outputSample->SetSampleTime(0));
        CHK(outputSample->SetSampleDuration(10000000));
    }

    // Process data
    DWORD status;
    MftOutputDataBuffer output(outputSample);
    CHK(_processor->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0));
    CHK(_processor->ProcessInput(0, inputSample.Get(), 0));
    CHK(_processor->ProcessOutput(0, 1, &output, &status));

    // Get the output buffer
    Microsoft::WRL::ComPtr<IMFMediaBuffer> outputBuffer;
    CHK(output.pSample->GetBufferByIndex(0, &outputBuffer));

    //Logger.LumiaAnalyzer_ConvertStop((void*)this);
    return outputBuffer;
}

using Microsoft::WRL::ComPtr;
using Windows::Storage::Streams::IBuffer;

using namespace Platform;
using concurrency::cancellation_token;
using concurrency::create_task;
using concurrency::task_continuation_context;

