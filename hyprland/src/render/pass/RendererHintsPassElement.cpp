#include "RendererHintsPassElement.hpp"

CRendererHintsPassElement::CRendererHintsPassElement(const CRendererHintsPassElement::SData& data_) : m_data(data_) {
    ;
}

bool CRendererHintsPassElement::needsLiveBlur() {
    return false;
}

bool CRendererHintsPassElement::needsPrecomputeBlur() {
    return false;
}

bool CRendererHintsPassElement::undiscardable() {
    return true;
}

bool CRendererHintsPassElement::disableSimplification() {
    return m_data.renderModif && m_data.renderModif->enabled && !m_data.renderModif->modifs.empty();
}
