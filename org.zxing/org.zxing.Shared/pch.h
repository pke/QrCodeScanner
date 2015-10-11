﻿#pragma once

#include <collection.h>
#include <ppltasks.h>

#include <wrl.h>

#include <robuffer.h>
#include <Objbase.h>

#include <d3d11_2.h>

#include <mfapi.h>
#include <mfidl.h>
#include <Mferror.h>
#include <mfreadwrite.h>

#include <windows.media.h>


//
// Asserts
//

#ifndef NDEBUG
#define NT_ASSERT(expr) if (!(expr)) { __debugbreak(); }
#else
#define NT_ASSERT(expr)
#endif

//
// Error handling
//

// Exception-based error handling
#define CHK(statement)  {HRESULT _hr = (statement); if (FAILED(_hr)) { throw ref new Platform::COMException(_hr); };}
#define CHKNULL(p)  {if ((p) == nullptr) { throw ref new Platform::NullReferenceException(L#p); };}
#define CHKOOM(p)  {if ((p) == nullptr) { throw ref new Platform::OutOfMemoryException(L#p); };}

// Exception-free error handling
#define CHK_RETURN(statement) {hr = (statement); if (FAILED(hr)) { return hr; };}

//
// Error origin
//

namespace Details
{
	class ErrorOrigin
	{
	public:

		template <size_t N, size_t L>
		static HRESULT TracedOriginateError(_In_ char const (&function)[L], _In_ HRESULT hr, _In_ wchar_t const (&str)[N])
		{
			if (FAILED(hr))
			{
				//s_logger.Log(function, LogLevel::Error, "failed hr=%08X: %S", hr, str);
				::RoOriginateErrorW(hr, N - 1, str);
			}
			return hr;
		}

		// A method to track error origin
		template <size_t L>
		static HRESULT TracedOriginateError(_In_ char const (&function)[L], __in HRESULT hr)
		{
			if (FAILED(hr))
			{
				//s_logger.Log(function, LogLevel::Error, "failed hr=%08X", hr);
				::RoOriginateErrorW(hr, 0, nullptr);
			}
			return hr;
		}

		ErrorOrigin() = delete;
	};
}

#define OriginateError(_hr, ...) ::Details::ErrorOrigin::TracedOriginateError(__FUNCTION__, _hr, __VA_ARGS__)

//
// Exception boundary (converts exceptions into HRESULTs)
//

namespace Details
{
	template<size_t L /*= sizeof(__FUNCTION__)*/>
	class TracedExceptionBoundary
	{
	public:
		TracedExceptionBoundary(_In_ const char *function /*= __FUNCTION__*/)
			: _function(function)
		{
		}

		TracedExceptionBoundary(const TracedExceptionBoundary&) = delete;
		TracedExceptionBoundary& operator=(const TracedExceptionBoundary&) = delete;

		HRESULT operator()(std::function<void()>&& lambda)
		{
			//s_logger.Log(_function, L, LogLevel::Verbose, "boundary enter");

			HRESULT hr = S_OK;
			try
			{
				lambda();
			}
#ifdef _INC_COMDEF // include comdef.h to enable
			catch (const _com_error& e)
			{
				hr = e.Error();
			}
#endif
#ifdef __cplusplus_winrt // enable C++/CX to use (/ZW)
			catch (Platform::Exception^ e)
			{
				hr = e->HResult;
			}
#endif
			catch (const std::bad_alloc&)
			{
				hr = E_OUTOFMEMORY;
			}
			catch (const std::out_of_range&)
			{
				hr = E_BOUNDS;
			}
			catch (const std::exception&)
			{
				//s_logger.Log(_function, L, LogLevel::Error, "caught unknown STL exception: %s", e.what());
				hr = E_FAIL;
			}
			catch (...)
			{
				//s_logger.Log(_function, L, LogLevel::Error, "caught unknown exception");
				hr = E_FAIL;
			}

			if (FAILED(hr))
			{
				//s_logger.Log(_function, L, LogLevel::Error, "boundary exit - failed hr=%08X", hr);
			}
			else
			{
				//s_logger.Log(_function, L, LogLevel::Verbose, "boundary exit");
			}

			return hr;
		}

	private:
		const char* _function;
	};
}

#define ExceptionBoundary ::Details::TracedExceptionBoundary<sizeof(__FUNCTION__)>(__FUNCTION__)

//
// Exception-safe PROPVARIANT
//

class PropVariant : public PROPVARIANT {
public:
	PropVariant()
	{
		PropVariantInit(this);
	}
	~PropVariant()
	{
		(void)PropVariantClear(this);
	}

	PropVariant(const PropVariant&) = delete;
	PropVariant& operator&(const PropVariant&) = delete;
};