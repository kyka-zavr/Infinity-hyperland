#include "RendererHintsPassElement.hpp"

CRendererHintsPassElement::CRendererHintsPassElement(const CRendererHintsPassElement::SData& data_) : m_data(data_) {
    ;
}

bool CRendererHintsPassElement::needsLiveBlur(Render::CRenderContext& ctx) {
    return false;
}

bool CRendererHintsPassElement::needsPrecomputeBlur(Render::CRenderContext& ctx) {
    return false;
}

bool CRendererHintsPassElement::undiscardable(Render::CRenderContext& ctx) {
    return true;
}

bool CRendererHintsPassElement::disableSimplification(Render::CRenderContext& ctx) {
    return m_data.renderModif && m_data.renderModif->enabled && !m_data.renderModif->modifs.empty();
}
