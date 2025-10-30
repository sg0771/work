// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

/*
** Turn. version 0.1
** (c) 2003 - Ernst Pech?
**
*/

#include "avisynth/avisynth_stdafx.h"

#include "turn.h"

//Ðý×ª
extern const AVSFunction Turn_filters[] = {
  { "TurnLeft","c",Turn::Create_TurnLeft },//utils.avs
  { "TurnRight","c",Turn::Create_TurnRight },//utils.avs
  { "Turn180","c",Turn::Create_Turn180 },//utils.avs
  { "Turn","ci",Turn::Create_Turn },//utils.avs
  { 0 }
};



void TurnRGB24(const unsigned char* srcp, unsigned char* dstp, const int rowsize,
	const int height, const int src_pitch, const int dst_pitch,
	const int direction)
{
	int dstp_offset;
	if (direction == -1) { // Left
		for (int y = 0; y < height; y++) {
			dstp_offset = (height - 1 - y) * 3;
			for (int x = 0; x < rowsize; x += 3) {
				dstp[dstp_offset + 0] = srcp[x + 0];
				dstp[dstp_offset + 1] = srcp[x + 1];
				dstp[dstp_offset + 2] = srcp[x + 2];
				dstp_offset += dst_pitch;
			}
			srcp += src_pitch;
		}
	}
	else if (direction == 1) { // Right
		int dstp_base = (rowsize / 3 - 1) * dst_pitch;
		for (int y = 0; y < height; y++) {
			dstp_offset = dstp_base + y * 3;
			for (int x = 0; x < rowsize; x += 3) {
				dstp[dstp_offset + 0] = srcp[x + 0];
				dstp[dstp_offset + 1] = srcp[x + 1];
				dstp[dstp_offset + 2] = srcp[x + 2];
				dstp_offset -= dst_pitch;
			}
			srcp += src_pitch;
		}
	}
	else { // 180
		dstp += (height - 1) * dst_pitch + (rowsize - 3);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < rowsize; x += 3) {
				dstp[-x + 0] = srcp[x + 0];
				dstp[-x + 1] = srcp[x + 1];
				dstp[-x + 2] = srcp[x + 2];
			}
			dstp -= dst_pitch;
			srcp += src_pitch;
		}
	}
}

void TurnRGB32(const unsigned char* srcp, unsigned char* dstp, const int rowsize,
	const int height, const int src_pitch, const int dst_pitch,
	const int direction)
{
	unsigned long* l_srcp = (unsigned long*)srcp;
	unsigned long* l_dstp = (unsigned long*)dstp;
	int l_rowsize = rowsize / 4;
	int l_src_pitch = src_pitch / 4;
	int l_dst_pitch = dst_pitch / 4;

	int dstp_offset;
	if (direction == -1) { // Left
		for (int y = 0; y < height; y++) {
			dstp_offset = (height - 1 - y);
			for (int x = 0; x < l_rowsize; x++) {
				l_dstp[dstp_offset] = l_srcp[x];
				dstp_offset += l_dst_pitch;
			}
			l_srcp += l_src_pitch;
		}
	}
	else if (direction == 1) { // Right
		int dstp_base = (l_rowsize - 1) * l_dst_pitch;
		for (int y = 0; y < height; y++) {
			dstp_offset = dstp_base + y;
			for (int x = 0; x < l_rowsize; x++) {
				l_dstp[dstp_offset] = l_srcp[x];
				dstp_offset -= l_dst_pitch;
			}
			l_srcp += l_src_pitch;
		}
	}
	else { // 180
		l_dstp += (height - 1) * l_dst_pitch + (l_rowsize - 1);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < l_rowsize; x++) {
				l_dstp[-x] = l_srcp[x];
			}
			l_dstp -= l_dst_pitch;
			l_srcp += l_src_pitch;
		}
	}
}

void TurnYUY2(const unsigned char* srcp, unsigned char* dstp, const int rowsize,
	const int height, const int src_pitch, const int dst_pitch,
	const int direction)
{
	unsigned char u, v;
	int dstp_offset;
	if (direction == 1) // Right
	{
		for (int y = 0; y < height; y += 2)
		{
			dstp_offset = ((height - 2 - y) << 1);
			for (int x = 0; x < rowsize; x += 4)
			{
				u = (srcp[x + 1] + srcp[x + 1 + src_pitch] + 1) >> 1;
				v = (srcp[x + 3] + srcp[x + 3 + src_pitch] + 1) >> 1;
				dstp[dstp_offset + 0] = srcp[x + src_pitch];
				dstp[dstp_offset + 1] = u;
				dstp[dstp_offset + 2] = srcp[x];
				dstp[dstp_offset + 3] = v;
				dstp_offset += dst_pitch;
				dstp[dstp_offset + 0] = srcp[x + src_pitch + 2];
				dstp[dstp_offset + 1] = u;
				dstp[dstp_offset + 2] = srcp[x + 2];
				dstp[dstp_offset + 3] = v;
				dstp_offset += dst_pitch;
			}
			srcp += src_pitch << 1;
		}
	}
	else if (direction == -1)// Left
	{
		srcp += rowsize - 4;
		for (int y = 0; y < height; y += 2)
		{
			dstp_offset = (y << 1);
			for (int x = 0; x < rowsize; x += 4)
			{
				u = (srcp[-x + 1] + srcp[-x + 1 + src_pitch] + 1) >> 1;
				v = (srcp[-x + 3] + srcp[-x + 3 + src_pitch] + 1) >> 1;
				dstp[dstp_offset + 0] = srcp[-x + 2];
				dstp[dstp_offset + 1] = u;
				dstp[dstp_offset + 2] = srcp[-x + 2 + src_pitch];
				dstp[dstp_offset + 3] = v;
				dstp_offset += dst_pitch;
				dstp[dstp_offset + 0] = srcp[-x];
				dstp[dstp_offset + 1] = u;
				dstp[dstp_offset + 2] = srcp[-x + src_pitch];
				dstp[dstp_offset + 3] = v;
				dstp_offset += dst_pitch;
			}
			srcp += src_pitch << 1;
		}
	}
	else // 180
	{
		dstp += (height - 1) * dst_pitch + (rowsize - 4);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < rowsize; x += 4) {
				dstp[-x + 2] = srcp[x + 0];
				dstp[-x + 1] = srcp[x + 1];
				dstp[-x + 0] = srcp[x + 2];
				dstp[-x + 3] = srcp[x + 3];
			}
			dstp -= dst_pitch;
			srcp += src_pitch;
		}
	}
}

void TurnPlanar(const unsigned char* srcp_y, unsigned char* dstp_y,
	const unsigned char* srcp_u, unsigned char* dstp_u,
	const unsigned char* srcp_v, unsigned char* dstp_v,
	const int rowsize, const int height,
	const int rowsizeUV, const int heightUV,
	const int src_pitch_y, const int dst_pitch_y,
	const int src_pitch_u, const int dst_pitch_uv,
	const int src_pitch_v, const int direction)
{
	int y, x, offset;
	if (direction == 1) // Right
	{
		for (y = 0; y < height; y++)
		{
			offset = height - 1 - y;
			for (x = 0; x < rowsize; x++)
			{
				dstp_y[offset] = srcp_y[x];
				offset += dst_pitch_y;
			}
			srcp_y += src_pitch_y;
		}
		for (y = 0; y < heightUV; y++)
		{
			offset = heightUV - 1 - y;
			for (x = 0; x < rowsizeUV; x++)
			{
				dstp_u[offset] = srcp_u[x];
				dstp_v[offset] = srcp_v[x];
				offset += dst_pitch_uv;
			}
			srcp_u += src_pitch_u;
			srcp_v += src_pitch_v;
		}
	}
	else if (direction == -1) // Left
	{
		srcp_y += rowsize - 1;
		for (y = 0; y < height; y++)
		{
			offset = y;
			for (x = 0; x < rowsize; x++)
			{
				dstp_y[offset] = srcp_y[-x];
				offset += dst_pitch_y;
			}
			srcp_y += src_pitch_y;
		}
		srcp_u += rowsizeUV - 1;
		srcp_v += rowsizeUV - 1;
		for (y = 0; y < heightUV; y++)
		{
			offset = y;
			for (x = 0; x < rowsizeUV; x++)
			{
				dstp_u[offset] = srcp_u[-x];
				dstp_v[offset] = srcp_v[-x];
				offset += dst_pitch_uv;
			}
			srcp_u += src_pitch_u;
			srcp_v += src_pitch_v;
		}
	}
	else // 180
	{
		dstp_y += (height - 1) * dst_pitch_y + (rowsize - 1);
		for (y = 0; y < height; y++) {
			for (x = 0; x < rowsize; x++) {
				dstp_y[-x] = srcp_y[x];
			}
			dstp_y -= dst_pitch_y;
			srcp_y += src_pitch_y;
		}
		dstp_u += (heightUV - 1) * dst_pitch_uv + (rowsizeUV - 1);
		dstp_v += (heightUV - 1) * dst_pitch_uv + (rowsizeUV - 1);
		for (y = 0; y < heightUV; y++) {
			for (x = 0; x < rowsizeUV; x++) {
				dstp_u[-x] = srcp_u[x];
				dstp_v[-x] = srcp_v[x];
			}
			dstp_u -= dst_pitch_uv;
			dstp_v -= dst_pitch_uv;
			srcp_u += src_pitch_u;
			srcp_v += src_pitch_v;
		}
	}
}




PVideoFrame __stdcall Turn::GetFrame(int n, IScriptEnvironment* env) {

	PVideoFrame src = child->GetFrame(n, env);
	if (src == nullptr || src.m_ptr == nullptr) {
		return nullptr;
	}

    PVideoFrame dst = env->NewVideoFrame(vi);
	if (dst == nullptr || dst.m_ptr == nullptr) {
		return nullptr;
	}

	if (Usource && Vsource) {
		PVideoFrame usrc = Usource->GetFrame(n, env);
		PVideoFrame vsrc = Vsource->GetFrame(n, env);

		TurnPlanFunc( src->GetReadPtr(PLANAR_Y), dst->GetWritePtr(PLANAR_Y),
					 usrc->GetReadPtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U),
					 vsrc->GetReadPtr(PLANAR_Y), dst->GetWritePtr(PLANAR_V),
					  src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y),
					 usrc->GetRowSize(PLANAR_Y), usrc->GetHeight(PLANAR_Y),
					  src->GetPitch(PLANAR_Y),   dst->GetPitch(PLANAR_Y),
					 usrc->GetPitch(PLANAR_Y),   dst->GetPitch(PLANAR_U),
					 vsrc->GetPitch(PLANAR_Y),   direction);
	}
	else if (vi.IsPlanar())
		TurnPlanFunc(src->GetReadPtr(PLANAR_Y), dst->GetWritePtr(PLANAR_Y),
					 src->GetReadPtr(PLANAR_U), dst->GetWritePtr(PLANAR_U),
					 src->GetReadPtr(PLANAR_V), dst->GetWritePtr(PLANAR_V),
					 src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y),
					 src->GetRowSize(PLANAR_U), src->GetHeight(PLANAR_U),
					 src->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_Y),
					 src->GetPitch(PLANAR_U), dst->GetPitch(PLANAR_U),
					 src->GetPitch(PLANAR_V), direction);
	else
		TurnFunc(src->GetReadPtr(),dst->GetWritePtr(),src->GetRowSize(),
				 src->GetHeight(),src->GetPitch(),dst->GetPitch(),direction);
    return dst;
}


AVSValue __cdecl Turn::Create_TurnLeft(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new Turn(args[0].AsClip(),-1, env);
}

AVSValue __cdecl Turn::Create_TurnRight(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new Turn(args[0].AsClip(),+1, env);
}

AVSValue __cdecl Turn::Create_Turn180(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new Turn(args[0].AsClip(),0, env);
}

AVSValue __cdecl Turn::Create_Turn(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new Turn(args[0].AsClip(), args[1].AsInt(), env);
}

