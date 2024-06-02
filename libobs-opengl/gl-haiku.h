/******************************************************************************
    Copyright (C) 2024 by Jacob Secunda <secundaja@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#pragma once

#include <stdint.h>

#include <glad/glad_egl.h>

#include <View.h>

class BBitmap;
class SwapChainBitmapHook;

struct gl_windowinfo {
	BView *surfaceView;
	BBitmap *surfaceBitmap;
	SwapChainBitmapHook *bitmapHook;
	EGLSurface surface;
};

class BitmapHook {
public:
	virtual ~BitmapHook() {};
	virtual void GetSize(uint32_t &width, uint32_t &height) = 0;
	virtual BBitmap *SetBitmap(BBitmap *bitmap) = 0;
};

class SwapChainBitmapHook : public BitmapHook {
public:
	SwapChainBitmapHook(gl_windowinfo *info) : fGLWindowInfo(info) {}
	virtual ~SwapChainBitmapHook() {};

	virtual void GetSize(uint32_t &width, uint32_t &height) override
	{
		BView *view = fGLWindowInfo->surfaceView;
		if (view != NULL) {
			BRect frame = view->Frame();
			width = frame.IntegerWidth() + 1;
			height = frame.IntegerHeight() + 1;
		}
	}

	virtual BBitmap *SetBitmap(BBitmap *bitmap) override
	{
		BBitmap *previousBitmap = fGLWindowInfo->surfaceBitmap;
		fGLWindowInfo->surfaceBitmap = bitmap;

		return previousBitmap;
	}

private:
	gl_windowinfo *fGLWindowInfo;
};
