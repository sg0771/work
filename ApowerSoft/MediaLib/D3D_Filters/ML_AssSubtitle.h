#pragma once

#include "MediaLibAPI.h"
#include <avisynth/avisynth.h>
#include <Utils.hpp>

using ass_track_wrapper = std::unique_ptr<ASS_Track, std::function<void(ASS_Track*)>>;

class ASSTrackHandler : public ass_track_wrapper {
public:
	AssContext m_context;
	ASSTrackHandler(AssContext _context) :
		ass_track_wrapper(AssEngine::Instance().Read(_context.path), ass_free_track) {
		m_context = _context;
		if (this->get() == NULL)
		{
			this->reset(AssEngine::Instance().Read(_context.path));
		}

	}

	ASSTrackHandler(std::string asscontent) :
		ass_track_wrapper(AssEngine::Instance().Read(asscontent), ass_free_track) {}

	ASS_Track* operator()() const noexcept {
		return this->get();
	}
	ass_image* Render(int ts)
	{
		int n = ts;
		if (this->get() != NULL &&
			n >= (int)m_context.offset * 1000 &&
			n < (int)1000 * m_context.offset + (int)1000 * (m_context.end - m_context.start))
		{
			int subtime = n - (int)1000 * (m_context.offset - m_context.start);
			return ass_render_frame(AssEngine::Instance().ass_renderer, this->get(), subtime, NULL);
		}
		return NULL;
	}
};  // class FileHandler
