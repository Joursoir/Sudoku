/******************************************************************************

   @file    graphics.hpp
   @author  Rajmund Szymanski
   @date    29.10.2020
   @brief   graphics class

*******************************************************************************

   Copyright (c) 2018-2020 Rajmund Szymanski. All rights reserved.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to
   deal in the Software without restriction, including without limitation the
   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
   sell copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.

******************************************************************************/

#pragma once

#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <vector>
#include <map>
#include <chrono>
#include <wchar.h>

class GraphicsTimer
{
	std::chrono::milliseconds tick_;
	std::chrono::time_point<std::chrono::high_resolution_clock> time_;

public:

	GraphicsTimer( const int duration = 0 )
	{
		tick_ = std::chrono::milliseconds(duration);
		time_ = std::chrono::high_resolution_clock::now();
	}

	bool Waiting()
	{
		if (std::chrono::high_resolution_clock::now() - time_ < tick_)
			return true;

		time_ += tick_;
		return false;
	}
};

class Graphics: public GraphicsTimer
{
	ID2D1Factory *factory;
	ID2D1HwndRenderTarget *target;
	IDWriteFactory *writer;
	ID2D1SolidColorBrush *brush;
	std::vector<IDWriteTextFormat *> fnt;

	void done()
	{
		for (auto f: fnt) f->Release();
		fnt.clear();

		if (brush   != NULL) { brush->Release();   brush   = NULL; }
		if (writer  != NULL) { writer->Release();  writer  = NULL; }
		if (target  != NULL) { target->Release();  target  = NULL; }
		if (factory != NULL) { factory->Release(); factory = NULL; }
	}

public:

	using Timer = GraphicsTimer;
	using Color = D2D1::ColorF::Enum;

	enum Alignment: DWORD
	{
		TopLeft     = MAKELONG(DWRITE_PARAGRAPH_ALIGNMENT_NEAR,   DWRITE_TEXT_ALIGNMENT_LEADING),
		Top         = MAKELONG(DWRITE_PARAGRAPH_ALIGNMENT_NEAR,   DWRITE_TEXT_ALIGNMENT_CENTER),
		TopRight    = MAKELONG(DWRITE_PARAGRAPH_ALIGNMENT_NEAR,   DWRITE_TEXT_ALIGNMENT_TRAILING),
		Left        = MAKELONG(DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_TEXT_ALIGNMENT_LEADING),
		Center      = MAKELONG(DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_TEXT_ALIGNMENT_CENTER),
		Right       = MAKELONG(DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_TEXT_ALIGNMENT_TRAILING),
		BottomLeft  = MAKELONG(DWRITE_PARAGRAPH_ALIGNMENT_FAR,    DWRITE_TEXT_ALIGNMENT_LEADING),
		Bottom      = MAKELONG(DWRITE_PARAGRAPH_ALIGNMENT_FAR,    DWRITE_TEXT_ALIGNMENT_CENTER),
		BottomRight = MAKELONG(DWRITE_PARAGRAPH_ALIGNMENT_FAR,    DWRITE_TEXT_ALIGNMENT_TRAILING),
	};

	struct Rectangle
	{
		const int left, top, right, bottom, x, y, width, height;

		Rectangle(int _x, int _y, int _w, int _h):
			left(_x), top(_y), right(_x + _w), bottom(_y + _h),
			x(_x), y(_y), width(_w), height(_h) {}

		operator RECT() const
		{
			return { left, top, right, bottom };
		}

		operator D2D1_RECT_F() const
		{
			return { static_cast<FLOAT>(left), static_cast<FLOAT>(top), static_cast<FLOAT>(right), static_cast<FLOAT>(bottom) };
		}

		bool contains( const int _x, const int _y ) const
		{
			return _x >= left && _x < right && _y >= top && _y < bottom;
		}

		int Center( const int _w ) const
		{
			return left + (width - _w) / 2;
		}

		int Middle( const int _h ) const
		{
			return top + (height - _h) / 2;
		}
	};

	Graphics( const int duration = 0 ): Timer(duration), factory{NULL}, target{NULL}, writer{NULL}, brush{NULL}, fnt{}
	{
	}

	~Graphics()
	{
		done();
	}

	bool init( HWND hWnd )
	{
		HRESULT hr;
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
		if (FAILED(hr)) return false;

		RECT rc;
		GetClientRect(hWnd, &rc);
		D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
		hr = factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &target);
		if (FAILED(hr)) return false;

		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&writer));
		if (FAILED(hr)) return false;

		hr = target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Enum::White), &brush);
		return SUCCEEDED(hr);
    }

	IDWriteTextFormat *font( const FLOAT h, const DWRITE_FONT_WEIGHT w, DWRITE_FONT_STRETCH s, const wchar_t *f )
	{
		IDWriteTextFormat *format = NULL;
		HRESULT hr = writer->CreateTextFormat(f, NULL, w, DWRITE_FONT_STYLE_NORMAL, s, h, L"", &format);
		if (SUCCEEDED(hr))
		{
			format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			fnt.push_back(format);
		}
		return format;
	}

	void quit()
	{
		DestroyWindow(target->GetHwnd());
	}

	void begin( Color c )
	{
		target->BeginDraw();
		target->SetTransform(D2D1::Matrix3x2F::Identity());
		target->Clear(D2D1::ColorF(c));
	}

	void end()
	{
		target->EndDraw();
	}

	ID2D1SolidColorBrush *get( const D2D1::ColorF::Enum c )
	{
		brush->SetColor(D2D1::ColorF(c));
		return brush;
	}

	void draw_line( const int x, const int y, const int w, const int h, const Color c )
	{
		D2D1_POINT_2F p1 = { static_cast<FLOAT>(x), static_cast<FLOAT>(y) };
		D2D1_POINT_2F p2 = { static_cast<FLOAT>(x + w - 1), static_cast<FLOAT>(y + h - 1) };
		target->DrawLine(p1, p2, get(c), 1.0f, NULL);
	}

	void draw_line( const D2D1_RECT_F &r, const Color c )
	{
		D2D1_POINT_2F p1 = { r.left, r.top };
		D2D1_POINT_2F p2 = { r.right - 1, r.bottom - 1 };
		target->DrawLine(p1, p2, get(c), 1.0f, NULL);
	}

	void draw_rect( const D2D1_RECT_F &r, const Color c )
	{
		target->DrawRectangle(&r, get(c));
	}

	void draw_rect( const D2D1_RECT_F &r, const int m, const Color c )
	{
		const D2D1_RECT_F rc = { r.left + m, r.top + m, r.right - m, r.bottom - m };
		draw_rect(rc, c);
	}

	void draw_rect( const int x, const int y, const int w, const int h, const Color c )
	{
		const D2D1_RECT_F rc = { static_cast<FLOAT>(x), static_cast<FLOAT>(y), static_cast<FLOAT>(x + w), static_cast<FLOAT>(y + h) };
		draw_rect(rc, c);
	}

	void fill_rect( const D2D1_RECT_F &r, const Color c )
	{
		target->FillRectangle(&r, get(c));
	}

	void fill_rect( const D2D1_RECT_F &r, const int m, const Color c )
	{
		const D2D1_RECT_F rc = { r.left + m, r.top + m, r.right - m, r.bottom - m };
		fill_rect(rc, c);
	}

	void fill_rect( const int x, const int y, const int w, const int h, const Color c )
	{
		const D2D1_RECT_F rc = { static_cast<FLOAT>(x), static_cast<FLOAT>(y), static_cast<FLOAT>(x + w), static_cast<FLOAT>(y + h) };
		fill_rect(rc, c);
	}

	void fill_ellipse( const D2D1_ELLIPSE *e, const Color c )
	{
		target->FillEllipse(e, get(c));
	}

	void fill_ellipse( const D2D1_RECT_F &r, const Color c )
	{
		D2D1_ELLIPSE e = { (r.right + r.left) / 2, (r.bottom + r.top) / 2, (r.right - r.left) / 2, (r.bottom - r.top) / 2 };
		target->FillEllipse(&e, get(c));
	}

	void fill_ellipse( const D2D1_RECT_F &r, const int m, const Color c )
	{
		D2D1_ELLIPSE e = { (r.right + r.left) / 2, (r.bottom + r.top) / 2, (r.right - r.left) / 2 - m, (r.bottom - r.top) / 2 - m };
		target->FillEllipse(&e, get(c));
	}

	void draw_layout( const D2D1_RECT_F &r, IDWriteTextFormat *f, const Color c, Alignment a, const wchar_t *t, size_t s )
	{
		f->SetParagraphAlignment(static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(LOWORD(a)));
		f->SetTextAlignment(static_cast<DWRITE_TEXT_ALIGNMENT>(HIWORD(a)));

		target->DrawText(t, s, f, &r, get(c), D2D1_DRAW_TEXT_OPTIONS_NO_SNAP, DWRITE_MEASURING_MODE_NATURAL);
	}

	void draw_char( const D2D1_RECT_F &r, IDWriteTextFormat *f, const Color c, Alignment a, const wchar_t t )
	{
		draw_layout(r, f, c, a, &t, 1);
	}

	void draw_char( const D2D1_RECT_F &r, const int m, IDWriteTextFormat *f, const Color c, Alignment a, const wchar_t t )
	{
		const D2D1_RECT_F rc = { r.left + m, r.top + m, r.right - m, r.bottom - m };
		draw_layout(rc, f, c, a, &t, 1);
	}

	void draw_text( const D2D1_RECT_F &r, IDWriteTextFormat *f, const Color c, Alignment a, const wchar_t *t )
	{
		draw_layout(r, f, c, a, t, wcslen(t));
	}

	void draw_text( const D2D1_RECT_F &r, const int m, IDWriteTextFormat *f, const Color c, Alignment a, const wchar_t *t )
	{
		const D2D1_RECT_F rc = { r.left + m, r.top + m, r.right - m, r.bottom - m };
		draw_layout(rc, f, c, a, t, wcslen(t));
	}
};