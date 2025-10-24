/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2021 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <string>
enum EPARCompensationType {
	EPCTDisabled,
	EPCTDownscale,
	EPCTUpscale,
	EPCTAccurateSize,
	EPCTAccurateSize_ISR
};

class STSStyle
{
public:
	enum RelativeTo {
		WINDOW = 0,
		VIDEO = 1,
		AUTO // ~video for SSA/ASS, ~window for the rest
	};
	RECT marginRect;		// measured from the sides
	std::wstring	fontNameW;


	int			scrAlignment;	// 1 - 9: as on the numpad, 0: default
	int			borderStyle;	// 0: outline, 1: opaque box
	double		outlineWidthX, outlineWidthY;
	double		shadowDepthX, shadowDepthY;
	COLORREF	colors[4];		// usually: {primary, secondary, outline/background, shadow}
	BYTE		alpha[4];
	int			charSet;

	double		fontSize;		// height
	double		fontScaleX, fontScaleY;	// percent
	double		fontSpacing;	// +/- pixels
	LONG		fontWeight;
	int			fItalic;
	int			fUnderline;
	int			fStrikeOut;
	int			fBlur;
	double		fGaussianBlur;
	double		fontAngleZ, fontAngleX, fontAngleY;
	double		fontShiftX, fontShiftY;
	RelativeTo	relativeTo;

	STSStyle();

	void SetDefault();

	bool operator == (const STSStyle& s) const;
	bool operator != (const STSStyle& s) const {
		return !(*this == s);
	};
	bool IsFontStyleEqual(const STSStyle& s) const;

	STSStyle& operator = (LOGFONT& lf);
	friend std::wstring& operator <<= (std::wstring& style, const STSStyle& s);
	friend STSStyle& operator <<= (STSStyle& s, const std::wstring& style);
	friend LOGFONTA& operator <<= (LOGFONTA& lfa, STSStyle& s);
	friend LOGFONTW& operator <<= (LOGFONTW& lfw, STSStyle& s);
};


