
struct COLORPICKOPT
{
	bool Alpha = 1;
	int Mode = 1;
	bool Dlg = 1;
	bool LUpdate = 0;
	bool AlsoUseSystem = 1;
	bool UsePicker = 1;
	float Resolution = 0.1f;
};

class COLORPICK
{
public:

	static float ScaleDPI;
	HWND hH;
	CComPtr<IDWriteFactory> WriteFactory;
	CComPtr<ID2D1HwndRenderTarget> d;
	CComPtr<ID2D1Factory> fa;

	static DWORD ToRGB(bool A, D2D1_COLOR_F C)
	{
		DWORD r = RGB((int)(C.r * 255.0f), (int)(C.g * 255.0f), (int)(C.b * 255.0f));
		if (A)
			r |= ((unsigned int)(C.a * 255.0f) << 24);
		return r;
	}

	static BYTE GetAValue(unsigned long rgb)
	{
		return (((rgb) >> 24));
	}


	static D2D1_COLOR_F FromRGB(DWORD r, bool A)
	{
		D2D1_COLOR_F C = {};
		C.r = GetRValue(r) / 255.0f;
		C.g = GetGValue(r) / 255.0f;
		C.b = GetBValue(r) / 255.0f;
		if (A)
			C.a = GetAValue(r) / 255.0f;
		else
			C.a = 1.0f;
		return C;
	}

	void Format(wchar_t* t, int mx, int Type, bool Alpha)
	{
		if (Type == 0) // RGB
		{
			if (Alpha)
				swprintf_s(t, mx, L"R %i G %i B %i A %i%%", (int)(C.r * 255.0f), (int)(C.g * 255.0f), (int)(C.b * 255.0f), (int)(C.a * 100.0f));
			else
				swprintf_s(t, mx, L"R %i G %i B %i", (int)(C.r * 255.0f), (int)(C.g * 255.0f), (int)(C.b * 255.0f));
		}
	}

	void fromRGBtoHSL(float rgb[], float hsl[])
	{
		const float maxRGB = std::max(rgb[0], std::max(rgb[1], rgb[2]));
		const float minRGB = std::min(rgb[0], std::min(rgb[1], rgb[2]));
		const float delta2 = maxRGB + minRGB;
		hsl[2] = delta2 * 0.5f;

		const float delta = maxRGB - minRGB;
		if (delta < FLT_MIN)
			hsl[0] = hsl[1] = 0.0f;
		else
		{
			hsl[1] = delta / (hsl[2] > 0.5f ? 2.0f - delta2 : delta2);
			if (rgb[0] >= maxRGB)
			{
				hsl[0] = (rgb[1] - rgb[2]) / delta;
				if (hsl[0] < 0.0f)
					hsl[0] += 6.0f;
			}
			else if (rgb[1] >= maxRGB)
				hsl[0] = 2.0f + (rgb[2] - rgb[0]) / delta;
			else
				hsl[0] = 4.0f + (rgb[0] - rgb[1]) / delta;
		}
	}

	void fromHSLtoRGB(const float hsl[], float rgb[])
	{
		if (hsl[1] < FLT_MIN)
			rgb[0] = rgb[1] = rgb[2] = hsl[2];
		else if (hsl[2] < FLT_MIN)
			rgb[0] = rgb[1] = rgb[2] = 0.0f;
		else
		{
			const float q = hsl[2] < 0.5f ? hsl[2] * (1.0f + hsl[1]) : hsl[2] + hsl[1] - hsl[2] * hsl[1];
			const float p = 2.0f * hsl[2] - q;
			float t[] = { hsl[0] + 2.0f, hsl[0], hsl[0] - 2.0f };

			for (int i = 0; i < 3; ++i)
			{
				if (t[i] < 0.0f)
					t[i] += 6.0f;
				else if (t[i] > 6.0f)
					t[i] -= 6.0f;

				if (t[i] < 1.0f)
					rgb[i] = p + (q - p) * t[i];
				else if (t[i] < 3.0f)
					rgb[i] = q;
				else if (t[i] < 4.0f)
					rgb[i] = p + (q - p) * (4.0f - t[i]);
				else
					rgb[i] = p;
			}
		}
	}

	static CComPtr<ID2D1SolidColorBrush> GetD2SolidBrush(ID2D1RenderTarget* p, D2D1_COLOR_F cc)
	{
		CComPtr<ID2D1SolidColorBrush> b = 0;
		p->CreateSolidColorBrush(cc, &b);
		return b;
	}

	void KeyDown(WPARAM ww, LPARAM = 0, bool S = false, bool Cx = false, bool Alt = false)
	{
		if (ww == VK_RETURN)
		{
			if (IsWindow(InEdit))
			{
				EndEdit(1);
				Redraw();
				return;
			}
			toR = S_OK;
			SendMessage(hH, WM_CLOSE, 0, 0);
		}
		if (ww == VK_ESCAPE)
		{
			if (IsWindow(InEdit))
			{
				EndEdit(0);
				Redraw();
				return;
			}
			toR = E_FAIL;
			SendMessage(hH, WM_CLOSE, 0, 0);
		}
	}
	void Wheel(WPARAM ww, LPARAM ll)
	{
		float x = 0, y = 0;
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hH, &pt);
		x = (FLOAT)pt.x;
		y = (FLOAT)pt.y;
		signed short HW = HIWORD(ww);

		float hsl2[3] = { 0 };
		float rgb2[3] = { C.r,C.g,C.b };
		fromRGBtoHSL(rgb2, hsl2);

		auto& Mode = opts.Mode;

		// H/R
		if (InRect<>(HRect, x, y) || InRect<>(RedRect, x, y) || InRect<>(HueRect, x, y))
		{
			if (Mode == 1 || InRect<>(RedRect, x, y))
			{
				if (HW > 0)
					C.r += 0.01f;
				else
					C.r -= 0.01f;
				if (C.r < 0) C.r = 0;
				if (C.r > 1.0f) C.r = 1.0f;
			}
			else
			if (Mode == 0 || InRect<>(HueRect, x, y))
			{
				if (HW > 0)
					hsl2[0] += 0.06f;
				else
					hsl2[0] -= 0.06f;
				if (hsl2[0] < 0) hsl2[0] = 0.0f;
				if (hsl2[0] > 6) hsl2[0] = 0.0f;
				fromHSLtoRGB(hsl2, rgb2);
				C.r = rgb2[0];
				C.g = rgb2[1];
				C.b = rgb2[2];
			}
		}

		// S/G
		if (InRect<>(SRect, x, y) || InRect<>(GreenRect, x, y) || InRect<>(SatRect, x, y))
		{
			if (Mode == 1 || InRect<>(GreenRect, x, y))
			{
				if (HW > 0)
					C.g += 0.01f;
				else
					C.g -= 0.01f;
				if (C.g < 0) C.g = 0;
				if (C.g > 1.0f) C.g = 1.0f;
			}
			else
				if (Mode == 0 || InRect<>(SatRect, x, y))
			{
				if (HW > 0)
					hsl2[1] += 0.01f;
				else
					hsl2[1] -= 0.01f;
				if (hsl2[1] < 0) hsl2[1] = 0.0f;
				if (hsl2[1] > 1) hsl2[1] = 1.0f;
				fromHSLtoRGB(hsl2, rgb2);
				C.r = rgb2[0];
				C.g = rgb2[1];
				C.b = rgb2[2];
			}
		}

		// S/G
		if (InRect<>(ARect, x, y) || InRect<>(AlphaRect, x, y))
		{
			if (HW > 0)
				C.a += 0.01f;
			else
				C.a -= 0.01f;
			if (C.a < 0) C.a = 0;
			if (C.a > 1.0f) C.a = 1.0f;
			A = C.a;
		}

		// L/B
		if (InRect<>(LRect, x, y) || InRect<>(BlueRect, x, y) || InRect<>(LumRect, x, y))
		{
			if (Mode == 1 || InRect<>(BlueRect, x, y))
			{
				if (HW > 0)
					C.b += 0.01f;
				else
					C.b -= 0.01f;
				if (C.b < 0) C.b = 0;
				if (C.b > 1.0f) C.b = 1.0f;
			}
			else
				if (Mode == 0 || InRect<>(LumRect, x, y))
			{
				if (HW > 0)
					hsl2[2] += 0.01f;
				else
					hsl2[2] -= 0.01f;
				if (hsl2[2] < 0) hsl2[2] = 0.0f;
				if (hsl2[2] > 1) hsl2[2] = 1.0f;
				fromHSLtoRGB(hsl2, rgb2);
				if (opts.LUpdate)
					maps.clear();
				L = hsl2[2];
				C.r = rgb2[0];
				C.g = rgb2[1];
				C.b = rgb2[2];
			}
		}

		Redraw();
	}

	void LeftUp(WPARAM ww, LPARAM ll)
	{
		if (Capturing)
			ReleaseCapture();


		FirstScreenMoving = 0;		
		Capturing = 0;
		AdjustingAlpha = 0;
		AdjustingL = 0;
		AdjustingS = 0;
		AdjustingH = 0;
		nnX = -1;
		nnY = -1;
	}

	void EndEdit(bool S)
	{
		int ID = GetWindowLong(InEdit, GWLP_ID);
		int J = GetDlgItemInt(hH, ID, 0, 0);
		if (J < 0) J = 0;
		if (ID == 110)
		{
			if (J > 100)
				J = 100;
		}
		else
		if (ID == 104)
		{
			if (J > 360)
				J = 0;
		}
		else
		{
			if (J > 255) 
				J = 255;
		}
		DestroyWindow(InEdit);
		InEdit = 0;
		if (S)
		{
			if (ID == 110) {
				C.a = J / 100.0f;
				A = C.a;
			}
			if (ID == 101) C.r = J / 255.0f;
			if (ID == 102) C.g = J / 255.0f;
			if (ID == 103) C.b = J / 255.0f;
			if (ID >= 104)
			{
				float hsl[3] = {};
				float rgb[3] = {};
				rgb[0] = C.r;
				rgb[1] = C.g;
				rgb[2] = C.b;
				fromRGBtoHSL(rgb, hsl);


				if (ID == 104)
					hsl[0] = J / 360.0f * 6.0f;
				if (ID == 105)
					hsl[1] = J / 255.0f;
				if (ID == 106)
					hsl[2] = J / 255.0f;

				fromHSLtoRGB(hsl, rgb);
				C.r = rgb[0];
				C.g = rgb[1];
				C.b = rgb[2];

			}
		}
	}

	template <typename T = float> bool InRect(D2D1_RECT_F& r, T x, T y)
	{
		if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom)
			return true;
		return false;
	}

	bool AdjustingAlpha = 0;
	bool AdjustingL = 0;
	bool AdjustingS = 0;
	bool AdjustingH = 0;
	HRESULT toR = E_FAIL;
	bool Capturing = false;
	HWND InEdit = 0;

	bool LeftDown(WPARAM ww, LPARAM ll)
	{
		float x = 0, y = 0;
		x = (FLOAT)LOWORD(ll);
		y = (FLOAT)HIWORD(ll);

		POINT pt;
		GetCursorPos(&pt);
		nnX = pt.x;
		nnY = pt.y;

		if (InRect<>(RedRectN, x, y) || InRect<>(GreenRectN, x, y) || InRect<>(BlueRectN, x, y))
		{
			if (IsWindow(InEdit))
				DestroyWindow(InEdit);
			short ID = 0;
			if (InRect<>(BlueRectN, x, y))
				ID = 103;
			if (InRect<>(GreenRectN, x, y))
				ID = 102;
			if (InRect<>(RedRectN, x, y))
				ID = 101;
			InEdit = CreateWindow(L"edit", L"", WS_CHILD | WS_VISIBLE | ES_NUMBER , 0, 0, 1, 1, hH, (HMENU)ID, 0, 0);
			if (ID == 101)
			{
				SetWindowPos(InEdit, 0, (int)RedRectN.left, (int)RedRectN.top + 5, (int)(RedRectN.right - RedRectN.left) / 4, (int)(RedRectN.bottom - RedRectN.top) - 8, SWP_SHOWWINDOW);
				SetDlgItemInt(hH, ID, (int)(C.r * 255.0f), false);
			}
			if (ID == 102)
			{
				SetWindowPos(InEdit, 0, (int)GreenRectN.left, (int)GreenRectN.top + 5, (int)(GreenRectN.right - GreenRectN.left) / 4, (int)(GreenRectN.bottom - GreenRectN.top) - 8, SWP_SHOWWINDOW);
				SetDlgItemInt(hH, ID, (int)(C.g * 255.0f), false);
			}
			if (ID == 103)
			{
				SetWindowPos(InEdit, 0, (int)BlueRectN.left, (int)BlueRectN.top + 5, (int)(BlueRectN.right - BlueRectN.left) / 4, (int)(BlueRectN.bottom - BlueRectN.top) - 8, SWP_SHOWWINDOW);
				SetDlgItemInt(hH, ID, (int)(C.b * 255.0f), false);
			}
			SetFocus(InEdit);
			SendMessage(InEdit, EM_SETSEL, 0, -1);
			return 1;

		}

		if (InRect<>(HueRectN, x, y) || InRect<>(SatRectN, x, y) || InRect<>(LumRectN, x, y))
		{
			if (IsWindow(InEdit))
				DestroyWindow(InEdit);
			short ID = 0;
			if (InRect<>(HueRectN, x, y))
				ID = 104;
			if (InRect<>(SatRectN, x, y))
				ID = 105;
			if (InRect<>(LumRectN, x, y))
				ID = 106;
			InEdit = CreateWindow(L"edit", L"", WS_CHILD | WS_VISIBLE | ES_NUMBER, 0, 0, 1, 1, hH, (HMENU)ID, 0, 0);
			float hsl[3] = {};
			float rgb[3] = {};
			rgb[0] = C.r;
			rgb[1] = C.g;
			rgb[2] = C.b;
			fromRGBtoHSL(rgb, hsl);
			if (ID == 106)
			{
				SetWindowPos(InEdit, 0, (int)LumRectN.left, (int)LumRectN.top + 5, (int)(LumRectN.right - LumRectN.left) / 4, (int)(LumRectN.bottom - LumRectN.top) - 8, SWP_SHOWWINDOW);
				SetDlgItemInt(hH, ID, (int)(hsl[2] * 255.0f), false);
			}
			if (ID == 105)
			{
				SetWindowPos(InEdit, 0, (int)SatRectN.left, (int)SatRectN.top + 5, (int)(SatRectN.right - SatRectN.left) / 4, (int)(SatRectN.bottom - SatRectN.top) - 8, SWP_SHOWWINDOW);
				SetDlgItemInt(hH, ID, (int)(hsl[1] * 255.0f), false);
			}
			if (ID == 104)
			{
				SetWindowPos(InEdit, 0, (int)HueRectN.left, (int)HueRectN.top + 5, (int)(HueRectN.right - HueRectN.left) / 4, (int)(HueRectN.bottom - HueRectN.top) - 8, SWP_SHOWWINDOW);
				SetDlgItemInt(hH, ID, (int)(((hsl[0]/6.0f) * 360.0f)), false);
			}
			SetFocus(InEdit);
			SendMessage(InEdit, EM_SETSEL, 0, -1);
			return 1;

		}

		if (InRect<>(AlphaRectN, x, y))
		{
			if (IsWindow(InEdit))
				DestroyWindow(InEdit);
			short ID = 110;
			InEdit = CreateWindow(L"edit", L"", WS_CHILD | WS_VISIBLE | ES_NUMBER, 0, 0, 1, 1, hH, (HMENU)ID, 0, 0);
			SetWindowPos(InEdit, 0, (int)AlphaRectN.left, (int)AlphaRectN.top + 5, (int)(AlphaRectN.right - AlphaRectN.left) / 4, (int)(AlphaRectN.bottom - AlphaRectN.top) - 8, SWP_SHOWWINDOW);
			SetDlgItemInt(hH, ID, (int)(C.a * 100.0f), false);
			SetFocus(InEdit);
			SendMessage(InEdit, EM_SETSEL, 0, -1);
			return 1;
		}

		// Cancel all InEdit
		if (IsWindow(InEdit))
		{
			EndEdit(1);
			return 1;
		}

		if (InRect<>(pickc.rect, x, y) && opts.UsePicker)
		{
			Capturing = 1;
			SetCapture(hH);
			return 1;
		}

		if (InRect<>(sysc.rect, x, y) && opts.AlsoUseSystem)
		{
			CHOOSECOLOR c = {};
			c.lStructSize = sizeof(c);
			c.hwndOwner = hH;
			COLORREF Custom1[16] = { 0 };
			const COLORREF DC_C[16] = { RGB(255,255,255),RGB(0,0,0),RGB(0,0,128),RGB(0,128,0),
										 RGB(255,0,0),RGB(128,0,0),RGB(128,0,128),RGB(255,128,0),
										 RGB(255,255,0),RGB(0,255,0),RGB(0,128,128),RGB(0,255,255),
										 RGB(0,0,255),RGB(255,0,255),RGB(128,128,128),RGB(192,192,192) };
			memcpy(&Custom1, (void*)&DC_C, sizeof(Custom1));
			c.Flags = CC_ANYCOLOR | CC_FULLOPEN;
			c.Flags |= CC_RGBINIT;
			c.rgbResult = ToRGB(false,C);
			c.lpCustColors = Custom1;
			if (!ChooseColor(&c))
				return 0;

			C = FromRGB(c.rgbResult, false);
			A = 1.0f;

			opts.Mode = 1;
			maps.clear();
			Redraw();
			return 1;
		}


		if (InRect<>(Mode1Rect, x, y))
		{
			opts.Mode = 0;
			maps.clear();
			Redraw();
			return 1;
		}
		if (InRect<>(Mode2Rect, x, y))
		{
			opts.Mode = 1;
			maps.clear();
			Redraw();
			return 1;
		}

		if (InRect<>(cancel.rect, x, y))
		{
			toR = E_FAIL;
			SendMessage(hH, WM_CLOSE, 0, 0);
			return 1;
		}
		if (InRect<>(ok.rect, x, y))
		{
			toR = S_OK;
			SendMessage(hH, WM_CLOSE, 0, 0);
			return 1;
		}

		if (AdjustingL && y < LRect.top)
		{
			y = LRect.top;
		}
		if (AdjustingL && y > LRect.bottom)
		{
			y = LRect.bottom;
		}
		if (AdjustingH && y < HRect.top)
		{
			y = HRect.top;
		}
		if (AdjustingH && y > HRect.bottom)
		{
			y = HRect.bottom;
		}
		if (AdjustingS && y < SRect.top)
		{
			y = SRect.top;
		}
		if (AdjustingS && y > SRect.bottom)
		{
			y = SRect.bottom;
		}
		if (AdjustingAlpha && y < ARect.top)
		{
			y = ARect.top;
		}
		if (AdjustingAlpha && y > ARect.bottom)
		{
			y = ARect.bottom;
		}

		// L
		if (InRect<>(LRect, x, y))
		{
			AdjustingL = 1;
			float he = LRect.bottom - LRect.top;
			y -= LRect.top;
			// in he, 1.0f
			// in y , ?
			float L2 = (1.0f - (y / he));

			// Convert
			if (opts.Mode == 1)
			{
				C.b = L2;
			}
			else
			{
				float hsl2[3] = { 0 };
				float rgb2[3] = { C.r,C.g,C.b };
				fromRGBtoHSL(rgb2, hsl2);
				hsl2[2] = L2;
				fromHSLtoRGB(hsl2, rgb2);
				C = { rgb2[0],rgb2[1],rgb2[2],A };
				L = hsl2[2];
				if (opts.LUpdate)
					maps.clear();
			}

			Redraw();
			return 1;
		}

				// S
		if (InRect<>(SRect, x, y))
		{
			AdjustingS = 1;
			float he = SRect.bottom - SRect.top;
			y -= SRect.top;
			// in he, 1.0f
			// in y , ?
			float L2 = (1.0f - (y / he));

			// Convert
			// Convert
			if (opts.Mode == 1)
			{
				C.g = L2;
			}
			else
			{
				float hsl2[3] = { 0 };
				float rgb2[3] = { C.r,C.g,C.b };
				fromRGBtoHSL(rgb2, hsl2);
				hsl2[1] = L2;
				fromHSLtoRGB(hsl2, rgb2);
				C = { rgb2[0],rgb2[1],rgb2[2],A };
			}

			Redraw();
			return 1;
		}
		// H
		if (InRect<>(HRect, x, y))
		{
			AdjustingL = 1;
			float he = HRect.bottom - HRect.top;
			y -= HRect.top;
			// in he, 1.0f
			// in y , ?
			float L2 = (1.0f - (y / he));

			// Convert
			if (opts.Mode == 1)
			{
				C.r= L2;
			}
			else
			{
				float hsl2[3] = { 0 };
				float rgb2[3] = { C.r,C.g,C.b };
				fromRGBtoHSL(rgb2, hsl2);
				hsl2[0] = L2 * 6;
				fromHSLtoRGB(hsl2, rgb2);
				C = { rgb2[0],rgb2[1],rgb2[2],A };
			}

			Redraw();
			return 1;
		}


		// A
		if (InRect<>(ARect, x, y))
		{
			AdjustingAlpha = 1;
			float he = LRect.bottom - LRect.top;
			y -= ARect.top;
			// in he, 1.0f
			// in y , ?
			A = (1.0f - (y / he));
			C.a = A;

			Redraw();
			return 1;
		}


		for (auto& v : maps)
		{
			if (InRect<>(v.r,x,y))
			{
				float hsl[3] = {}, rgb[3] = {};
				if (!opts.LUpdate && opts.Mode == 0)
				{
					// Keep L
					C = v.c;
					rgb[0] = C.r;
					rgb[1] = C.g;
					rgb[2] = C.b;
					fromRGBtoHSL(rgb, hsl);
					hsl[2] = L;
					fromHSLtoRGB(hsl, rgb);
					C.r = rgb[0];
					C.g = rgb[1];
					C.b = rgb[2];



				}
				else
				{
					C = v.c;
					C.a = A;
				}
				Redraw();
				return 1;
			}
		}

		FirstScreenMoving = 1;
		GetCursorPos(&ScreenDown);

		return 0;
	}



	void Redraw()
	{
		InvalidateRect(hH, 0, TRUE);
		UpdateWindow(hH);
	}

	int nnX = -1, nnY = -1;
	POINT ScreenDown = {};
	int FirstScreenMoving = 0;
	void MouseMove(WPARAM ww, LPARAM ll)
	{
		bool LeftClick = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);



		if (FirstScreenMoving && LeftClick)
		{
			POINT ScreenDown2 = {};
			GetCursorPos(&ScreenDown2);


			RECT rc;
			GetWindowRect(hH, &rc);

			SetWindowPos(hH, 0, rc.left + (ScreenDown2.x - ScreenDown.x), rc.top + (ScreenDown2.y - ScreenDown.y), 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
			ScreenDown = ScreenDown2;
		}

		if (LeftClick && Capturing)
		{
			POINT pt = { 0 };
			GetCursorPos(&pt);


			bool Shift = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
			//  Get the device context of the desktop and from it get the color 
			//  of the pixel at the current mouse pointer position

			SetCursor(LoadCursor(0, IDC_CROSS));
			auto hDC  = ::GetDCEx(NULL, NULL, 0);
			float fd = ScaleDPI/96.0f;
			DWORD cr = GetPixel(hDC, (int)(pt.x*fd), (int)(pt.y*fd));

			if (Shift)
				DebugBreak();
			ReleaseDC(0,hDC);
			C = FromRGB(cr,false);
			A = 1.0f;
			Redraw();
			return;
		}
		bool M  = false;
		if (LeftClick)
			M = LeftDown(ww, ll);
		if (!M && LeftClick)
		{

			POINT pt;
			GetCursorPos(&pt);
			int x = pt.x;
			int y = pt.y;
			RECT rc = {};
			GetWindowRect(hH, &rc);
			SetWindowPos(hH, 0, rc.left +  (x - nnX), rc.top + (y - nnY), 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
			nnX = x;
			nnY = y;
		}
	}
	void RightDown(WPARAM ww, LPARAM ll)
	{

	}
	void LeftDoubleClick(WPARAM ww, LPARAM ll)
	{
	}


	CComPtr<IDWriteTextFormat> Text;

	void CreateBrushes(IDWriteFactory* WriteFactoryX)
	{
		if (!Text)
		{
			LOGFONT lf;
			GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
			DWRITE_FONT_STYLE fst = DWRITE_FONT_STYLE_NORMAL;
			if (lf.lfItalic)
				fst = DWRITE_FONT_STYLE_ITALIC;
			DWRITE_FONT_STRETCH fsr = DWRITE_FONT_STRETCH_NORMAL;
			FLOAT fs = (FLOAT)abs(lf.lfHeight);
			WriteFactoryX->CreateTextFormat(lf.lfFaceName, 0, lf.lfWeight > 500 ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, fst, fsr, fs, L"", &Text);
		}
	}

	void Paint(ID2D1Factory* fact, IDWriteFactory* WriteFa, ID2D1RenderTarget* p, RECT rc)
	{
		D2D1_RECT_F ff;
		ff.left = (FLOAT)rc.left;
		ff.right = (FLOAT)rc.right;
		ff.bottom = (FLOAT)rc.bottom;
		ff.top = (FLOAT)rc.top;
		Paint(fact, WriteFa, p, ff);
	}


	void Paint(ID2D1Factory*, IDWriteFactory* WriteFa, ID2D1RenderTarget* p, D2D1_RECT_F rc)
	{
		CreateBrushes(WriteFa);
		ColorWheelRect = rc;
		ColorWheelRect.top += 30;
		ColorWheelRect.left += 30;
		ColorWheelRect.bottom -= 30;
		ColorWheelRect.right = ColorWheelRect.left + (ColorWheelRect.bottom - ColorWheelRect.top);


		HRect = ColorWheelRect;
		HRect.bottom -= 50;
		HRect.left = ColorWheelRect.right + 10;
		HRect.right = HRect.left + 20;

		SRect = HRect;
		SRect.left = HRect.right + 10;
		SRect.right = SRect.left + 20;

		LRect = SRect;
		LRect.left = SRect.right + 10;
		LRect.right = LRect.left + 20;

		ARect = LRect;
		ARect.left = ARect.right + 10;
		ARect.right = ARect.left + 20;

		auto black = GetD2SolidBrush(p, { 0.0f,0,0,1.0f });
		auto white = GetD2SolidBrush(p, { 1.0f,1.0f,1.0f,1.0f });
		auto br = GetD2SolidBrush(p, { 1.0f,0,0,1.0f });




		D2D1_POINT_2F Center = { ColorWheelRect.left + (ColorWheelRect.right - ColorWheelRect.left) / 2.0f,
		ColorWheelRect.top + (ColorWheelRect.bottom - ColorWheelRect.top) / 2.0f };
		if (maps.empty())
		{
			if (opts.Mode == 1)
			{
				// RGB
				p->Clear();
				maps.reserve(1000000);
				int X = 9;
				int Y = 13;

				DWORD col[117] = { 
					RGB(0x33,0,0),
					RGB(0x33,0x19,0),
					RGB(0x33,0x33,0),
					RGB(0x19,0x33,0),
					RGB(0,0x33,0),
					RGB(0,0x33,0x19),
					RGB(0,0x33,0x33),
					RGB(0,0x19,0x33),
					RGB(0,0,0x33),
					RGB(0x19,0,0x33),
					RGB(0x33,0,0x33),
					RGB(0x33,0,0x19),
					RGB(0,0,0),

					RGB(0x66,0,0),
					RGB(0x66,0x33,0),
					RGB(0x66,0x66,0),
					RGB(0x33,0x66,0),
					RGB(0,0x66,0),
					RGB(0,0x66,0x33),
					RGB(0,0x66,0x66),
					RGB(0,0x33,0x66),
					RGB(0,0,0x66),
					RGB(0x33,0,0x66),
					RGB(0x66,0,0x66),
					RGB(0x66,0,0x33),
					RGB(0x20,0x20,0x20),

					RGB(0x99,0,0),
					RGB(0x99,0x4C,0),
					RGB(0x99,0x99,0),
					RGB(0x4C,0x99,0),
					RGB(0,0x99,0),
					RGB(0,0x99,0x4C),
					RGB(0,0x99,0x99),
					RGB(0,0x4C,0x99),
					RGB(0,0,0x99),
					RGB(0x4C,0,0x99),
					RGB(0x99,0,0x99),
					RGB(0x99,0,0x4C),
					RGB(0x40,0x40,0x40),

					RGB(0xCC,0,0),
					RGB(0xCC,0x66,0),
					RGB(0xCC,0xCC,0),
					RGB(0x66,0xCC,0),
					RGB(0,0xCC,0),
					RGB(0,0xCC,0x66),
					RGB(0,0xCC,0xCC),
					RGB(0,0x66,0xCC),
					RGB(0,0,0xCC),
					RGB(0x66,0,0xCC),
					RGB(0xCC,0,0xCC),
					RGB(0xCC,0,0x66),
					RGB(0x60,0x60,0x60),

					RGB(0xFF,0,0),
					RGB(0xFF,0x80,0),
					RGB(0xFF,0xFF,0),
					RGB(0x80,0xFF,0),
					RGB(0,0xFF,0),
					RGB(0,0xFF,0x80),
					RGB(0,0xFF,0xFF),
					RGB(0,0x80,0xFF),
					RGB(0,0,0xFF),
					RGB(0x80,0,0xFF),
					RGB(0xFF,0,0xFF),
					RGB(0xFF,0,0x80),
					RGB(0x80,0x80,0x80),


					RGB(0xFF,0x33,0x33),
					RGB(0xFF,0x99,0x33),
					RGB(0xFF,0xFF,0x33),
					RGB(0x99,0xFF,0x33),
					RGB(0x33,0xFF,0x33),
					RGB(0x33,0xFF,0x99),
					RGB(0x33,0xFF,0xFF),
					RGB(0x33,0x99,0xFF),
					RGB(0x33,0x33,0xFF),
					RGB(0x99,0x33,0xFF),
					RGB(0xFF,0x33,0xFF),
					RGB(0xFF,0x33,0x99),
					RGB(0xA0,0xA0,0xA0),


					RGB(0xFF,0x66,0x66),
					RGB(0xFF,0xB2,0x66),
					RGB(0xFF,0xFF,0x66),
					RGB(0xB2,0xFF,0x66),
					RGB(0x66,0xFF,0x66),
					RGB(0x66,0xFF,0xB2),
					RGB(0x66,0xFF,0xFF),
					RGB(0x66,0xB2,0xFF),
					RGB(0x66,0x66,0xFF),
					RGB(0xB2,0x66,0xFF),
					RGB(0xFF,0x66,0xFF),
					RGB(0xFF,0x66,0xB2),
					RGB(0xC0,0xC0,0xC0),

					RGB(0xFF, 0x99, 0x99),
					RGB(0xFF, 0xCC, 0x99),
					RGB(0xFF, 0xFF, 0x99),
					RGB(0xCC, 0xFF, 0x99),
					RGB(0x99, 0xFF, 0x99),
					RGB(0x99, 0xFF, 0xCC),
					RGB(0x99, 0xFF, 0xFF),
					RGB(0x99, 0xCC, 0xFF),
					RGB(0x99, 0x99, 0xFF),
					RGB(0xCC, 0x99, 0xFF),
					RGB(0xFF, 0x99, 0xFF),
					RGB(0xFF, 0x99, 0xCC),
					RGB(0xE0, 0xE0, 0xE0),


					RGB(0xFF, 0xCC, 0xCC),
					RGB(0xFF, 0xE5, 0xCC),
					RGB(0xFF, 0xFF, 0xCC),
					RGB(0xE5, 0xFF, 0xCC),
					RGB(0xCC, 0xFF, 0xCC),
					RGB(0xCC, 0xFF, 0xE5),
					RGB(0xCC, 0xFF, 0xFF),
					RGB(0xCC, 0xE5, 0xFF),
					RGB(0xCC, 0xCC, 0xFF),
					RGB(0xE5, 0xCC, 0xFF),
					RGB(0xFF, 0xCC, 0xFF),
					RGB(0xFF, 0xCC, 0xE5),
					RGB(0xFF, 0xFF, 0xFF),

				};

				int ni = 0;

				float ItemW = (ColorWheelRect.right - ColorWheelRect.left) / (float)Y;
				ItemW -= 2;
				float ItemH = (ColorWheelRect.bottom - ColorWheelRect.top) / (float)X;
				ItemH -= 2;
				for (int x = 0; x < X; x++)
				{
					for (int y = 0; y < Y; y++)
					{
						D2D1_RECT_F r = {};
						r.left = ItemW * y + 15;
						r.right = r.left + ItemW - 5;
						r.top = ItemH * x + 15;
						r.bottom = r.top + ItemH - 5;

						auto cc = FromRGB(col[ni++],false);
						br->SetColor(cc);
						p->FillRectangle(r,br);

						PT pt;
						pt.r = r;
						pt.c = cc;
						maps.push_back(pt);

						p->DrawRectangle(r, white);

					}
				}
				//  
			}
			else
			{
				p->Clear();
				maps.reserve(1000000);
				D2D1_ROUNDED_RECT r2 = { rc, 2,2 };
				p->DrawRoundedRectangle(r2, white, 2);

				float Radius = (ColorWheelRect.right - ColorWheelRect.left) / 2.0f;

				for (float hue = 0; hue < 360.0f; hue += opts.Resolution)
				{
					for (int sat = 0; sat < (int)Radius; sat++)
					{
						// Convert S to [0,1]
						float S = (float)sat / Radius;
						// Convert H to [0,6]
						float H = (hue * 6) / 360.0f;

						float hsl[3] = { H,S,L };
						float rgb[3] = { 0 };
						fromHSLtoRGB(hsl, rgb);

						D2D1_COLOR_F col = { rgb[0],rgb[1],rgb[2],1.0f };
						br->SetColor(col);

						// in 360 deg, 2pi
						// in   hue deg, ? 
						float hue2 = (2 * 3.1415f * hue) / 360.0f;
						float xPoint = (float)sat;
						float nX = xPoint * cos(hue2);
						float nY = -xPoint * sin(hue2);

		//				D2D1_POINT_2F p2 = Center;
			//			p2.x += sat;

						D2D1_POINT_2F p2 = { nX,nY };
						p2.x += Center.x;
						p2.y += Center.y;

						D2D1_RECT_F r3 = {};
						r3.left = p2.x;
						r3.top = p2.y;
						r3.right = p2.x + 1;
						r3.bottom = p2.y + 1;
						p->FillRectangle(r3, br);

						PT pt;
						pt.r.left = p2.x;
						pt.r.top = p2.y;
						pt.r.right = pt.r.left + 1;
						pt.r.bottom = pt.r.top + 1;
						pt.c = col;
						maps.push_back(pt);
					}

				}
			}
		}

		// LRect
		CComPtr<ID2D1LinearGradientBrush> lbr;
		CComPtr<ID2D1GradientStopCollection> pGradientStops = NULL;
		D2D1_GRADIENT_STOP gradientStops[2];



		// Saturation or Green
		if (opts.Mode == 0)
		{
			lbr = 0;
			pGradientStops = 0;
			float hsl2[3] = { 0 };
			float rgb2[3] = { C.r,C.g,C.b };
			fromRGBtoHSL(rgb2, hsl2);
			hsl2[1] = 0;
			fromHSLtoRGB(hsl2, rgb2);
			gradientStops[1].color = { rgb2[0],rgb2[1],rgb2[2],1.0f };
			gradientStops[0].position = 0.0f;

			float rgb3[3] = { C.r,C.g,C.b };
			fromRGBtoHSL(rgb3, hsl2);
			hsl2[1] = 1.0f;
			fromHSLtoRGB(hsl2, rgb3);
			gradientStops[0].color = { rgb3[0],rgb3[1],rgb3[2],1.0f };
			gradientStops[1].position = 1.0f;
			// Create the ID2D1GradientStopCollection from a previously
			// declared array of D2D1_GRADIENT_STOP structs.
			p->CreateGradientStopCollection(
				gradientStops,
				2,
				D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				&pGradientStops
			);
			p->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(
					D2D1::Point2F(SRect.left, SRect.top),
					D2D1::Point2F(SRect.right, SRect.bottom)),
				pGradientStops,
				&lbr
			);
			p->FillRectangle(SRect, lbr);

			// "S"
			Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			auto r2 = SRect;
			r2.top -= 25;
			r2.bottom = SRect.top;
			p->FillRectangle(r2, black);
			p->DrawTextW(L"S", 1, Text, r2, white);
		}
		else
		{
			p->FillRectangle(LRect, black);

			lbr = 0;
			pGradientStops = 0;
			gradientStops[0].color = { 0.0,0.0,0,1.0f };
			gradientStops[0].position = 1.0;
			gradientStops[1].color = { 0.0,1.0f,0,1.0f };
			gradientStops[1].position = 0.0f;
			p->CreateGradientStopCollection(
				gradientStops,
				2,
				D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				&pGradientStops
			);
			p->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(
					D2D1::Point2F(SRect.left, SRect.top),
					D2D1::Point2F(SRect.right, SRect.bottom)),
				pGradientStops,
				&lbr
			);

			p->FillRectangle(SRect, lbr);

			// "G"
			Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			auto r2 = SRect;
			r2.top -= 25;
			r2.bottom = SRect.top;
			p->FillRectangle(r2, black);
			p->DrawTextW(L"G", 1, Text, r2, white);
		}


		if (opts.Mode == 0)
		{
			lbr = 0;
			pGradientStops = 0;
			float hsl2[3] = { 0 };
			float rgb2[3] = { C.r,C.g,C.b };
			fromRGBtoHSL(rgb2, hsl2);
			hsl2[2] = L;
			fromHSLtoRGB(hsl2, rgb2);
			gradientStops[1].color = { rgb2[0],rgb2[1],rgb2[2],1.0f };
			gradientStops[0].position = 0.0f;

			float rgb3[3] = { C.r,C.g,C.b };
			fromRGBtoHSL(rgb3, hsl2);
			hsl2[2] = 1.0f;
			fromHSLtoRGB(hsl2, rgb3);
			gradientStops[0].color = { rgb3[0],rgb3[1],rgb3[2],1.0f };
			gradientStops[1].position = 1.0f;
			// Create the ID2D1GradientStopCollection from a previously
			// declared array of D2D1_GRADIENT_STOP structs.
			p->CreateGradientStopCollection(
				gradientStops,
				2,
				D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				&pGradientStops
			);
			p->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(
					D2D1::Point2F(LRect.left, LRect.top),
					D2D1::Point2F(LRect.right, LRect.bottom)),
				pGradientStops,
				&lbr
			);
			p->FillRectangle(LRect, lbr);

			// "L"
			Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			auto r2 = LRect;
			r2.top -= 25;
			r2.bottom = LRect.top;
			p->FillRectangle(r2, black);
			p->DrawTextW(L"L", 1, Text, r2, white);
		}
		else
		{
			p->FillRectangle(LRect, black);

			lbr = 0;
			pGradientStops = 0;
			gradientStops[0].color = { 0.0,0.0,0,1.0f };
			gradientStops[0].position = 1.0;
			gradientStops[1].color = { 0.0,0.0f,1.0f,1.0f };
			gradientStops[1].position = 0.0f;
			p->CreateGradientStopCollection(
				gradientStops,
				2,
				D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				&pGradientStops
			);
			p->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(
					D2D1::Point2F(LRect.left, LRect.top),
					D2D1::Point2F(LRect.right, LRect.bottom)),
				pGradientStops,
				&lbr
			);

			p->FillRectangle(LRect, lbr);

			// "b"
			Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			auto r2 = LRect;
			r2.top -= 25;
			r2.bottom = LRect.top;
			p->FillRectangle(r2, black);
			p->DrawTextW(L"B", 1, Text, r2, white);
		}


		if (opts.Mode == 0)
		{
			// Create the linear brush for hue
			std::vector<D2D1_GRADIENT_STOP>  gst(360);
			for (int i = 0; i < 360; i++)
			{
				float hsl[3] = { 1,1,L };
				hsl[0] = (360 -  i) / 360.0f;
				hsl[0] *= 6.0f;
				float rgb[3] = {};
				fromHSLtoRGB(hsl, rgb);
				gst[i].position = i / 360.0f;
				gst[i].color.r = rgb[0];
				gst[i].color.g = rgb[1];
				gst[i].color.b = rgb[2];
				gst[i].color.a = 1.0f;


			}
			pGradientStops = 0;
			lbr = 0;
			p->CreateGradientStopCollection(
				gst.data(),
				360,
				D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				&pGradientStops
			);
			p->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(
					D2D1::Point2F(LRect.left, LRect.top),
					D2D1::Point2F(LRect.right, LRect.bottom)),
				pGradientStops,
				&lbr);


			p->FillRectangle(HRect, lbr);

			// "H"
			Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			auto r2 = HRect;
			r2.top -= 25;
			r2.bottom = HRect.top;
			p->FillRectangle(r2, black);
			p->DrawTextW(L"H", 1, Text, r2, white);
		}
		else
		{
			p->FillRectangle(HRect, black);

			// ARect
			lbr = 0;
			pGradientStops = 0;
			gradientStops[0].color = { 0.0,0.0,0,1.0f };
			gradientStops[0].position = 1.0;
			gradientStops[1].color = { 1.0,0,0,1.0f };
			gradientStops[1].position = 0.0f;
			p->CreateGradientStopCollection(
				gradientStops,
				2,
				D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				&pGradientStops
			);
			p->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(
					D2D1::Point2F(LRect.left, LRect.top),
					D2D1::Point2F(LRect.right, LRect.bottom)),
				pGradientStops,
				&lbr
			);

			p->FillRectangle(HRect, lbr);

			// "R"
			Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			auto r2 = HRect;
			r2.top -= 25;
			r2.bottom = HRect.top;
			p->FillRectangle(r2, black);
			p->DrawTextW(L"R", 1, Text, r2, white);
		}


		// Hue
		float rgb[3] = { C.r,C.g,C.b };
		float hsl[3] = { 0 };
		fromRGBtoHSL(rgb, hsl);

		if (true)
		{
			// LRect line
			float he = LRect.bottom - LRect.top;
			// in he, 1.0f
			// in ? ,  L
			float h2 = 0;
			if (opts.Mode == 1)
				h2 = he * (1.0f - C.b);
			else
				h2 = he* (1.0f - hsl[2]);
			h2 += LRect.top;
			p->DrawLine({ LRect.left - 0,h2 }, { LRect.right + 0,h2 }, black, 3);
		}

		if (true)
		{
			// SRect line
			float he = SRect.bottom - SRect.top;
			// in he, 1.0f
			// in ? ,  L
			float h2 = 0;
			if (opts.Mode == 1)
				h2 = he * (1.0f - C.g);
			else
				h2 = he * (1.0f - hsl[1]);
			h2 += SRect.top;
			p->DrawLine({ SRect.left - 0,h2 }, { SRect.right + 0,h2 }, black, 3);
		}

		if (true)
		{
			//HRect line
			float he = HRect.bottom - HRect.top;
			// in he, 1.0f
			// in ? ,  L
			float h2 = 0;
			if (opts.Mode == 1)
				h2 = he * (1.0f - C.r);
			else
				h2 = he * (1.0f - (hsl[0]/6.0f));
			h2 += SRect.top;
			p->DrawLine({ HRect.left - 0,h2 }, { HRect.right + 0,h2 }, black, 3);
		}

		if (opts.Alpha)
		{
			p->FillRectangle(ARect, black);

			// ARect
			lbr = 0;
			pGradientStops = 0;
			gradientStops[0].color = { C.r,C.g,C.b,1.0f };
			gradientStops[0].position = 0.0;
			gradientStops[1].color = { C.r,C.g,C.b,0.0f };
			gradientStops[1].position = 1.0f;
				// Create the ID2D1GradientStopCollection from a previously
			// declared array of D2D1_GRADIENT_STOP structs.
			p->CreateGradientStopCollection(
				gradientStops,
				2,
				D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				&pGradientStops
			);
			p->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(
					D2D1::Point2F(LRect.left, LRect.top),
					D2D1::Point2F(LRect.right, LRect.bottom)),
				pGradientStops,
				&lbr
			);

			p->FillRectangle(ARect, lbr);

			// "A"
			Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			auto r2 = ARect;
			r2.top -= 25;
			r2.bottom = ARect.top;
			p->FillRectangle(r2, black);
			p->DrawTextW(L"A", 1, Text, r2, white);
		}

		if (opts.Alpha)
		{
			// ARect line
			float he = ARect.bottom - ARect.top;
			float h2 = he * (1.0f - C.a);
			h2 += ARect.top;
			p->DrawLine({ ARect.left - 0,h2 }, { ARect.right + 0,h2 }, black, 3);
		}

		// mode
		Mode1Rect = HRect;
		Mode1Rect.right = SRect.right;
		Mode1Rect.top = SRect.bottom + 10;
		Mode1Rect.bottom = SRect.bottom + 30;
		Mode2Rect = LRect;
		Mode2Rect.right = ARect.right;
		Mode2Rect.top = LRect.bottom + 10;
		Mode2Rect.bottom = LRect.bottom + 30;
		p->FillRectangle(Mode2Rect, black);
		p->FillRectangle(Mode1Rect, black);
		if (opts.Mode == 0)
		{
			D2D1_ROUNDED_RECT r1 = { Mode1Rect, 2,2 };
			D2D1_ROUNDED_RECT r2 = {Mode2Rect, 2,2};
			p->FillRoundedRectangle(r1, white);
			p->DrawRoundedRectangle(r2, white);
			Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			p->DrawTextW(L"HSL", 3, Text, Mode1Rect, black);
			p->DrawTextW(L"RGB", 3, Text, Mode2Rect, white);
		}
		else
		{
			D2D1_ROUNDED_RECT r1 = { Mode2Rect, 2,2 };
			D2D1_ROUNDED_RECT r2 = { Mode1Rect, 2,2 };
			p->FillRoundedRectangle(r1, white);
			p->DrawRoundedRectangle(r2, white);
			Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			p->DrawTextW(L"RGB", 3, Text, Mode2Rect, black);
			p->DrawTextW(L"HSL", 3, Text, Mode1Rect, white);
		}


		// Sys 
		if (true)
		{
			sysc.radiusX = sysc.radiusY = 1.0f;
			sysc.rect.left = rc.left + 15;
			sysc.rect.right = sysc.rect.left + 20;
			sysc.rect.top = rc.bottom - 50;
			sysc.rect.bottom = rc.bottom - 30;
			if (opts.AlsoUseSystem)
			{
				p->FillRoundedRectangle(sysc, white);
				Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
				Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
				p->DrawTextW(L"S", 1, Text, sysc.rect, black);
			}
		}

		// Pick
		if (opts.UsePicker)
		{
			pickc = sysc;
			pickc.rect.left += 25;
			pickc.rect.right = pickc.rect.left + 20;
			p->FillRoundedRectangle(pickc, white);
			float m1 = pickc.rect.top + (pickc.rect.bottom - pickc.rect.top) / 2.0f;
			float m2 = pickc.rect.left + (pickc.rect.right - pickc.rect.left) / 2.0f;
			p->DrawLine({
				pickc.rect.left + 2,m1 }, {
				pickc.rect.right - 2,m1 }, black, 1);
			p->DrawLine({
				m2,pickc.rect.top + 2 }, {
				m2,pickc.rect.bottom - 2 }, black, 1);
		}

		// OK 
		ok.radiusX = ok.radiusY = 1.0f;
		ok.rect.left = rc.right - 250;
		ok.rect.right = ok.rect.left + 100;
		ok.rect.top = rc.bottom - 50;
		ok.rect.bottom = rc.bottom - 20;


		p->FillRoundedRectangle(ok, white);
		Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		p->DrawTextW(L"OK", 2, Text, ok.rect, black);

		// Cancel
		cancel = ok;
		cancel.rect.left += 115;
		cancel.rect.right += 115;


		p->FillRoundedRectangle(cancel, white);
		Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		p->DrawTextW(L"CANCEL", 6, Text, cancel.rect, black);

		// Color
		SelC = ok;
		SelC.rect.right = cancel.rect.right;
		SelC.rect.top -= 100;
		SelC.rect.bottom -= 50;

		auto b2 = GetD2SolidBrush(p, C);
		p->FillRectangle(SelC.rect, black);
		p->FillRoundedRectangle(SelC, b2);


		wchar_t t[100] = {};
		D2D1_RECT_F r2 = {};

		// Red
		RedRect.left = ok.rect.left;
		RedRect.right = cancel.rect.right;
		RedRect.top = ColorWheelRect.top;
		RedRect.bottom = RedRect.top + 30;
		p->FillRectangle(RedRect, black);
		Text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		Text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		swprintf_s(t, 100, L"RED");
		p->DrawTextW(t, (UINT32)(wcslen(t)), Text, RedRect, white);
		RedRect.left += 50;
		RedRectN = RedRect;
		swprintf_s(t, 100, L"%i", (int)(C.r * 255.0f));
		p->DrawTextW(t, (UINT32)(wcslen(t)), Text, RedRect, white);
		RedRect.left -= 50;
		r2 = RedRect;
		r2.left += 100;
		r2.top += 5;
		r2.bottom -= 5;
		br->SetColor({ C.r,0,0,A });
		p->FillRectangle(r2, br);

		// Red
		GreenRect = RedRect;
		GreenRect.top += 30;
		GreenRect.bottom += 30;
		p->FillRectangle(GreenRect, black);
		swprintf_s(t, 100, L"GREEN");
		p->DrawTextW(t, (UINT32)(wcslen(t)), Text, GreenRect, white);
		GreenRect.left += 50;
		GreenRectN = GreenRect;
		swprintf_s(t, 100, L"%i", (int)(C.g * 255.0f));
		p->DrawTextW(t, (UINT32)(wcslen(t)), Text, GreenRect, white);
		GreenRect.left -= 50;
		r2 = GreenRect;
		r2.left += 100;
		r2.top += 5;
		r2.bottom -= 5;
		br->SetColor({ 0.0,C.g,0,A });
		p->FillRectangle(r2, br);

		// Blue
		BlueRect = GreenRect;
		BlueRect.top += 30;
		BlueRect.bottom += 30;
		p->FillRectangle(BlueRect, black);
		swprintf_s(t, 100, L"BLUE");
		p->DrawTextW(t, (UINT32)(wcslen(t)), Text, BlueRect, white);
		BlueRect.left += 50;
		BlueRectN = BlueRect;
		swprintf_s(t, 100, L"%i", (int)(C.b * 255.0f));
		p->DrawTextW(t, (UINT32)(wcslen(t)), Text, BlueRect, white);
		BlueRect.left -= 50;
		r2 = BlueRect;
		r2.left += 100;
		r2.top += 5;
		r2.bottom -= 5;
		br->SetColor({ 0.0,0.0,C.b,A });
		p->FillRectangle(r2, br);


		HueRect = BlueRect;
		HueRect.top += 50;
		HueRect.bottom += 50;
		p->FillRectangle(HueRect, black);
		swprintf_s(t, 100, L"HUE");
		p->DrawTextW(t, (UINT32)(wcslen(t)), Text, HueRect, white);
		HueRect.left += 50;
		HueRectN = HueRect;
		swprintf_s(t, 100, L"%i", (int)(hsl[0] / 6.0f * 360.0f));
		p->DrawTextW(t, (UINT32)(wcslen(t)), Text, HueRect, white);
		HueRect.left -= 50;

		SatRect = HueRect;
		SatRect.top += 30;
		SatRect.bottom += 30;
		p->FillRectangle(SatRect, black);
		swprintf_s(t, 100, L"SAT");
		p->DrawTextW(t, (UINT32)(wcslen(t)), Text, SatRect, white);
		SatRect.left += 50;
		SatRectN = SatRect;
		swprintf_s(t, 100, L"%i", (int)(hsl[1] * 255.0f));
		p->DrawTextW(t, (UINT32)(wcslen(t)), Text, SatRect, white);
		SatRect.left -= 50;

		LumRect = SatRect;
		LumRect.top += 30;
		LumRect.bottom += 30;
		p->FillRectangle(LumRect, black);
		swprintf_s(t, 100, L"LIGHT");
		p->DrawTextW(t, (UINT32)(wcslen(t)), Text, LumRect, white);
		LumRect.left += 50;
		LumRectN = LumRect;
		swprintf_s(t, 100, L"%i", (int)(hsl[2] * 255.0f));
		p->DrawTextW(t, (UINT32)(wcslen(t)), Text, LumRect, white);
		LumRect.left -= 50;

		if (opts.Alpha)
		{
			AlphaRect = LumRect;
			AlphaRect.top += 50;
			AlphaRect.bottom += 50;
			p->FillRectangle(AlphaRect, black);
			swprintf_s(t, 100, L"ALPHA");
			p->DrawTextW(t, (UINT32)(wcslen(t)), Text, AlphaRect, white);
			AlphaRect.left += 50;
			AlphaRectN = AlphaRect;
			swprintf_s(t, 100, L"%i%%", (int)(A * 100.0f));
			p->DrawTextW(t, (UINT32)(wcslen(t)), Text, AlphaRect, white);
			AlphaRect.left -= 50;
		}
		p->DrawRectangle(rc, white, 5);
	}
	D2D1_RECT_F RedRect = {};
	D2D1_RECT_F GreenRect = {};
	D2D1_RECT_F BlueRect = {};

	D2D1_RECT_F HueRect = {};
	D2D1_RECT_F SatRect = {};
	D2D1_RECT_F LumRect = {};

	D2D1_RECT_F RedRectN = {};
	D2D1_RECT_F GreenRectN = {};
	D2D1_RECT_F BlueRectN = {};

	D2D1_RECT_F HueRectN = {};
	D2D1_RECT_F SatRectN = {};
	D2D1_RECT_F LumRectN = {};

	D2D1_RECT_F AlphaRect = {};
	D2D1_RECT_F AlphaRectN = {};

	D2D1_RECT_F ColorWheelRect = {};
	D2D1_RECT_F HRect = {};
	D2D1_RECT_F SRect = {};
	D2D1_RECT_F LRect = {};
	D2D1_RECT_F ARect = {};
	D2D1_RECT_F Mode1Rect = {};
	D2D1_RECT_F Mode2Rect = {};
	D2D1_COLOR_F C = { 0,0,0,1.0f };
	D2D1_ROUNDED_RECT SelC = {};
	D2D1_ROUNDED_RECT ok = {};
	D2D1_ROUNDED_RECT cancel = {};
	D2D1_ROUNDED_RECT sysc = {};
	D2D1_ROUNDED_RECT pickc = {};
	float L = 0.5f;
	float A = 1.0f;

	struct PT
	{
		D2D1_RECT_F r = {};
		D2D1_COLOR_F c;
	};
	std::vector<PT> maps;
//	std::unordered_map<D2D1_RECT_F, D2D1_COLOR_F> maps;


	LRESULT CALLBACK Main_DP(HWND hh, UINT mm, WPARAM ww, LPARAM ll)
	{
		switch (mm)
		{
			case WM_COMMAND:
			{
				if (LOWORD(ww) == IDOK)
				{
					KeyDown(VK_RETURN, 0, 0, 0, 0);
				}
				if (LOWORD(ww) == IDCANCEL)
				{
					KeyDown(VK_ESCAPE, 0, 0, 0, 0);
				}
				return 0;
			}
			case WM_CLOSE:
			{
				if (opts.Dlg)
					EndDialog(hh, 0);
				else
					DestroyWindow(hh);
				return 0;
			}

			case WM_DESTROY:
			{
				if (opts.Dlg)
					return 0;
				PostQuitMessage(0);
				return 0;
			}

			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				KeyDown(ww, ll);
				return 0;
			}
			case WM_MOUSEMOVE:
			{
				MouseMove(ww, ll);
				return 0;
			}
			case WM_LBUTTONDOWN:
			{
				LeftDown(ww, ll);
				return 0;
			}
			case WM_RBUTTONDOWN:
			{
				RightDown(ww, ll);
				return 0;
			}
			case WM_MOUSEWHEEL:
			{
				int x = LOWORD(ll);
				int y = HIWORD(ll);
				POINT p = { x,y };
				ScreenToClient(hH, &p);
				ll = p.x;
				ll |= p.y << 16;
				Wheel(ww, ll);
				return 0;
			}
			case WM_LBUTTONUP:
			{
				LeftUp(ww, ll);
				return 0;
			}
			case WM_LBUTTONDBLCLK:
			{
				LeftDoubleClick(ww, ll);
				return 0;
			}
			case WM_ERASEBKGND:
				return 1;

			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				BeginPaint(hh, &ps);

				RECT rc;
				GetClientRect(hh, &rc);
				if (!WriteFactory)
					DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&WriteFactory);
				if (!fa)
					D2D1CreateFactory(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_MULTI_THREADED, &fa);
				float dpiX = 0, dpiY = 0;
#pragma warning(disable: 4996)
				fa->GetDesktopDpi(&dpiX, &dpiY);
#pragma warning(default: 4996)
				if (!d)
				{
					//				D2D1_RENDER_TARGET_PROPERTIES p;
					D2D1_HWND_RENDER_TARGET_PROPERTIES hp;
					hp.hwnd = hh;
					hp.pixelSize.width = rc.right;
					hp.pixelSize.height = rc.bottom;
					d.Release();

					auto props = D2D1::RenderTargetProperties();
					props.dpiX = 96;
					props.dpiY = 96;
					fa->CreateHwndRenderTarget(props, D2D1::HwndRenderTargetProperties(hh, D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)), &d);
				}
				d->BeginDraw();
//				d->SetTransform(D2D1::Matrix3x2F::Scale(96.0f/dpiX, 96.0f/dpiY));
				Paint(fa, WriteFactory, d, rc);
				d->EndDraw();
				EndPaint(hh, &ps);
				return 0;
			}

			case WM_SIZE:
			{
				if (!d)
					return 0;

				RECT rc;
				GetClientRect(hh, &rc);
				D2D1_SIZE_U u;
				u.width = rc.right;
				u.height = rc.bottom;
				d->Resize(u);
				return 0;
			}

		}
		if (opts.Dlg)
			return 0;
		return DefWindowProc(hh, mm, ww, ll);
	}


	static LRESULT CALLBACK Main2_DP(HWND hh, UINT mm, WPARAM ww, LPARAM ll)
	{
		COLORPICK* c = (COLORPICK*)GetWindowLongPtr(hh, GWLP_USERDATA);
		if (mm == WM_CREATE)
		{
			CREATESTRUCT* cs = (CREATESTRUCT*)ll;
			c = (COLORPICK*)cs->lpCreateParams;
			SetWindowLongPtr(hh, GWLP_USERDATA, (LONG_PTR)c);
		}
		if (mm == WM_INITDIALOG)
		{
			c = (COLORPICK*)ll;
			c->hH = hh;
			SetWindowLongPtr(hh, GWLP_USERDATA, (LONG_PTR)c);
		}
		if (c)
			return c->Main_DP(hh, mm, ww, ll);
		return DefWindowProc(hh, mm, ww, ll);
	}

	void CenterWindow(RECT* r)
	{
		int xMax = GetSystemMetrics(SM_CXFULLSCREEN);
		int yMax = GetSystemMetrics(SM_CYFULLSCREEN);

		int x = r->right;
		int y = r->bottom;

		xMax -= x;
		yMax -= y;
		xMax /= 2;
		yMax /= 2;

		r->left = xMax;
		r->top = yMax;
	}

	COLORPICKOPT opts;


	LRESULT Show(HWND hParent, D2D1_COLOR_F& init, COLORPICKOPT* opt = 0)
	{
		if (opt)
			opts = *opt;

		C = init;
		if (!opts.Alpha)
			C.a = 1.0f;
		else
			A = C.a;

		if (C.b == 0 && C.g == 0 && C.r == 0 && C.a == 0)
		{
			C.a = 1.0f;
			A = 1.0f;
		}
		if (opts.Dlg)
		{
				const char* res = "\x01\x00\xFF\xFF\x00\x00\x00\x00\x00\x00\x00\x00\x48\x08\x08\x80\x00\x00\x00\x00\x00\x00\x21\x02\xFA\x00\x00\x00\x00\x00\x00\x00\x08\x00\x90\x01\x00\x01\x4D\x00\x53\x00\x20\x00\x53\x00\x68\x00\x65\x00\x6C\x00\x6C\x00\x20\x00\x44\x00\x6C\x00\x67\x00\x00\x00";
				DialogBoxIndirectParam(GetModuleHandle(0), (DLGTEMPLATE*)res, hParent, (DLGPROC)Main2_DP, (LPARAM)this);

			if (toR == S_OK)
			{
				init = C;
				if (!opts.Alpha)
					init.a = 1.0f;
			}
			return toR;

		}
		else
		{
			WNDCLASSEX wClass = { 0 };
			wClass.cbSize = sizeof(wClass);

			wClass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_PARENTDC;
			wClass.lpfnWndProc = (WNDPROC)Main2_DP;
			wClass.hInstance = GetModuleHandle(0);
			wClass.hIcon = 0;
			wClass.hCursor = LoadCursor(0, IDC_ARROW);
			wClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
			wClass.lpszClassName = _T("CLASS");
			wClass.hIconSm = 0;
			RegisterClassEx(&wClass);

			RECT r = { 0,0,800,400 };
			CenterWindow(&r);
			auto MainWindow = CreateWindowEx(0,
				_T("CLASS"),
				L"",
				WS_POPUP, r.left, r.top, r.right, r.bottom, hParent, 0, GetModuleHandle(0), (LPVOID)this);

			hH = MainWindow;
			ShowWindow(MainWindow, SW_SHOW);
			MSG msg;
			while (GetMessage(&msg, 0, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (toR == S_OK)
			{
				init = C;
				if (!opts.Alpha)
					init.a = 1.0f;
			}
			return toR;

		}

	}

};



